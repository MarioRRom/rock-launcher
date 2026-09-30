#include "rocklaunch/core/utils/downloader.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/progress.h"

#include <curl/curl.h>

#include <chrono>
#include <cstddef>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace rocklaunch
{
namespace Downloader
{

namespace
{

constexpr long kConnectTimeoutSeconds = 30;

// Under 1 KB/s for a minute counts as dead, not slow.
constexpr long kLowSpeedLimitBytes = 1024;
constexpr long kLowSpeedTimeSeconds = 60;

// GitHub's API returns 403 without a User-Agent, and libcurl sends none.
constexpr const char *kUserAgent = "RockLaunch/0.1 (libcurl)";

void EnsureCurlInitialized()
{
    static const CURLcode init = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (init != CURLE_OK) {
        Logger().Error("Downloader: curl_global_init failed: "
                       + std::string(curl_easy_strerror(init)));
        throw std::runtime_error("libcurl could not start; no transfer can run");
    }
}

struct CurlDeleter
{
    void operator()(CURL *handle) const { curl_easy_cleanup(handle); }
};

struct SlistDeleter
{
    void operator()(curl_slist *list) const { curl_slist_free_all(list); }
};

// An empty destPath means "buffer the body in memory".
struct Transfer
{
    std::string url;
    fs::path destPath;
    std::vector<std::string> headers;
    std::string displayName;
    bool acceptEncoding = false;
    ProgressStage stage = ProgressStage::Downloading;
};

struct WriteSink
{
    std::ofstream *file = nullptr;
    std::string *body = nullptr;
    uint64_t bytesWritten = 0;
};

struct ProgressContext
{
    const ProgressCallback *onProgress = nullptr;
    std::string file;
    ProgressStage stage = ProgressStage::Downloading;
    std::chrono::steady_clock::time_point started;
    bool cancelled = false;
};

std::size_t WriteCallback(char *data, std::size_t size,
                          std::size_t count, void *clientp)
{
    const std::size_t bytes = size * count;
    WriteSink *sink = static_cast<WriteSink *>(clientp);
    sink->bytesWritten += bytes;

    if (sink->file != nullptr) {
        sink->file->write(data, static_cast<std::streamsize>(bytes));
        if (!*sink->file) {
            return 0; // Any short count is CURLE_WRITE_ERROR.
        }
    } else if (sink->body != nullptr) {
        sink->body->append(data, bytes);
    }

    return bytes;
}

// libcurl reports an unknown length as a negative count.
uint64_t ToBytes(curl_off_t value)
{
    return value > 0 ? static_cast<uint64_t>(value) : 0;
}

Progress MakeProgress(const ProgressContext &ctx, uint64_t transferred,
                      uint64_t total)
{
    Progress progress;
    progress.stage = ctx.stage;
    progress.file = ctx.file;
    progress.bytesTransferred = transferred;
    progress.totalBytes = total;
    progress.percentage = total > 0
        ? 100.0 * static_cast<double>(transferred) / static_cast<double>(total)
        : 0.0;

    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - ctx.started).count();
    if (seconds > 0.0) {
        progress.bytesPerSecond = static_cast<uint64_t>(
            static_cast<double>(transferred) / seconds);
    }

    return progress;
}

int TransferInfoCallback(void *clientp, curl_off_t downloadTotal,
                         curl_off_t downloaded, curl_off_t, curl_off_t)
{
    ProgressContext *ctx = static_cast<ProgressContext *>(clientp);
    if (ctx == nullptr || ctx->onProgress == nullptr || !*ctx->onProgress) {
        return 0;
    }

    if (!(*ctx->onProgress)(MakeProgress(*ctx, ToBytes(downloaded),
                                        ToBytes(downloadTotal)))) {
        ctx->cancelled = true;
        return 1; // CURLE_ABORTED_BY_CALLBACK
    }
    return 0;
}

bool IsGitHubApi(const std::string &url)
{
    return url.find("api.github.com") != std::string::npos;
}

std::string DescribeError(CURLcode code, long responseCode,
                          const std::string &url, const char *errorBuffer)
{
    switch (code) {
    case CURLE_FILE_COULDNT_READ_FILE:
        return "Cannot read file: " + url;
    case CURLE_COULDNT_RESOLVE_HOST:
        return "Cannot resolve host: " + url;
    case CURLE_COULDNT_CONNECT:
        return "Network unreachable: " + url;
    case CURLE_OPERATION_TIMEDOUT:
        return "Transfer timed out: " + url;
    case CURLE_HTTP_RETURNED_ERROR:
        // 403 is the hourly quota, 429 the burst limit; they clear at different times.
        if ((responseCode == 403 || responseCode == 429) && IsGitHubApi(url)) {
            if (responseCode == 429) {
                return "GitHub secondary rate limit (429). "
                       "Too many requests in a short window; retry shortly.";
            }
            return "GitHub API rate limit exceeded (403). "
                   "Unauthenticated requests are limited to 60 per hour.";
        }
        return "HTTP " + std::to_string(responseCode) + " for " + url;
    default:
        break;
    }

    std::string message = curl_easy_strerror(code);
    if (errorBuffer != nullptr && errorBuffer[0] != '\0'
        && std::string(errorBuffer) != message) {
        message += " (" + std::string(errorBuffer) + ")";
    }
    return message + ": " + url;
}

// A completed transfer reports 100 % whatever Content-Length claimed.
void ReportCompleted(const ProgressCallback &onProgress,
                     const ProgressContext &ctx, uint64_t bytes)
{
    if (onProgress) {
        onProgress(MakeProgress(ctx, bytes, bytes));
    }
}

std::string Perform(const Transfer &transfer, const ProgressCallback &onProgress)
{
    Logger logger;
    EnsureCurlInitialized();

    const bool toFile = !transfer.destPath.empty();
    fs::path tempPath;
    std::ofstream out;
    if (toFile) {
        if (transfer.destPath.has_parent_path()) {
            fs::create_directories(transfer.destPath.parent_path());
        }
        tempPath = transfer.destPath.string() + ".tmp";
        out.open(tempPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            Logger().Error("Downloader: cannot write " + tempPath.string());
            throw std::runtime_error(
                "check free space and write permission on "
                + transfer.destPath.parent_path().string());
        }
    }

    std::unique_ptr<CURL, CurlDeleter> handle(curl_easy_init());
    if (!handle) {
        Logger().Error("Downloader: cannot create libcurl handle");
        throw std::runtime_error("libcurl ran out of memory before the transfer started");
    }

    std::string body;
    WriteSink sink;
    sink.file = toFile ? &out : nullptr;
    sink.body = toFile ? nullptr : &body;

    ProgressContext ctx;
    ctx.onProgress = &onProgress;
    ctx.file = transfer.displayName;
    ctx.stage = transfer.stage;
    ctx.started = std::chrono::steady_clock::now();

    char errorBuffer[CURL_ERROR_SIZE] = {};

    curl_slist *rawHeaders = nullptr;
    for (const std::string &header : transfer.headers) {
        rawHeaders = curl_slist_append(rawHeaders, header.c_str());
    }
    std::unique_ptr<curl_slist, SlistDeleter> headers(rawHeaders);

    CURL *easy = handle.get();
    curl_easy_setopt(easy, CURLOPT_URL, transfer.url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 10L);
    // Redirects may only stay on https, so a hostile server cannot bounce the
    // transfer onto file:// and read the local disk.
    curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    // Like curl -f, but the status stays readable, which tells 403 from 404.
    curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, kUserAgent);
    // Required when called from a worker thread.
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, kLowSpeedLimitBytes);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, kLowSpeedTimeSeconds);
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, TransferInfoCallback);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, &ctx);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &sink);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, errorBuffer);
    // Only for the JSON API: on release assets a negotiated encoding would make
    // the total describe the compressed size while the counter tracks decoded bytes.
    if (transfer.acceptEncoding) {
        curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
    }

    const CURLcode result = curl_easy_perform(easy);

    long responseCode = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &responseCode);

    out.close();
    std::error_code ec;

    if (result != CURLE_OK) {
        if (toFile) {
            fs::remove(tempPath, ec);
        }
        // Checked before the log so a cancel the user asked for does not land as
        // an ERROR on stderr and in the logfile.
        if (ctx.cancelled) {
            Logger().Info("Downloader: cancelled " + transfer.url);
            throw Cancelled("the download was cancelled");
        }
        Logger().Error("Downloader: "
                       + DescribeError(result, responseCode, transfer.url, errorBuffer));
        throw std::runtime_error(
            toFile ? "the partial file at " + transfer.destPath.string() + " was removed"
                   : "no file was written");
    }

    if (toFile) {
        fs::rename(tempPath, transfer.destPath, ec);
        if (ec) {
            const std::string reason = ec.message();
            fs::remove(tempPath, ec);
            Logger().Error("Downloader: cannot rename " + tempPath.string() + " to "
                           + transfer.destPath.string() + ": " + reason);
            throw std::runtime_error(
                "the bytes arrived, but the temporary file could not be moved into place");
        }
        ReportCompleted(onProgress, ctx, sink.bytesWritten);
    }

    return body;
}

std::string CurlGet(const std::string &url)
{
    Transfer transfer;
    transfer.url = url;
    transfer.displayName = "GitHub API";
    transfer.stage = ProgressStage::Resolving;
    transfer.acceptEncoding = true;
    transfer.headers.push_back("Accept: application/vnd.github+json");

    return Perform(transfer, nullptr);
}

ReleaseInfo ParseRelease(const nlohmann::json &rel)
{
    ReleaseInfo info;
    info.version = rel.value("tag_name", "");
    info.tag = rel.value("tag_name", "");

    if (rel.contains("assets") && rel["assets"].is_array()) {
        for (const auto &a : rel["assets"]) {
            AssetInfo asset;
            asset.name = a.value("name", "");
            asset.downloadUrl = a.value("browser_download_url", "");
            asset.size = a.value("size", 0);
            info.assets.push_back(std::move(asset));
        }
    }

    return info;
}

} // anonymous namespace

void Fetch(const std::string &url, const fs::path &destPath,
           ProgressCallback onProgress)
{
    Logger logger;
    logger.Info("Downloader: fetching " + url);

    Transfer transfer;
    transfer.url = url;
    transfer.destPath = destPath;
    transfer.displayName = destPath.filename().string();
    transfer.stage = ProgressStage::Downloading;

    Perform(transfer, onProgress);

    if (!fs::exists(destPath) || fs::file_size(destPath) == 0) {
        Logger().Error("Downloader: empty file at " + destPath.string());
        throw std::runtime_error(
            "the transfer finished but produced no bytes, so nothing was installed");
    }

    logger.Info("Downloader: fetched " + std::to_string(fs::file_size(destPath))
                + " bytes to " + destPath.filename().string());
}

std::vector<ReleaseInfo> ListReleases(const std::string &repo, int count)
{
    Logger logger;
    logger.Info("Downloader: listing releases for " + repo);

    std::string url = "https://api.github.com/repos/" + repo
                    + "/releases?per_page=" + std::to_string(count);

    std::string body = CurlGet(url);

    auto json = nlohmann::json::parse(body);
    if (!json.is_array()) {
        Logger().Error("Downloader: GitHub API returned non-array for " + repo);
        throw std::runtime_error(
            "asked " + url + "\n  the answer was " + std::to_string(body.size())
            + " bytes of "
            + (json.is_object() ? "an object"
                                : json.is_string() ? "a string" : "another type")
            + ", not a release list");
    }

    std::vector<ReleaseInfo> releases;
    releases.reserve(json.size());

    for (const auto &rel : json) {
        releases.push_back(ParseRelease(rel));
    }

    logger.Info("Downloader: found " + std::to_string(releases.size())
                + " releases for " + repo);

    return releases;
}

} // namespace Downloader
} // namespace rocklaunch
