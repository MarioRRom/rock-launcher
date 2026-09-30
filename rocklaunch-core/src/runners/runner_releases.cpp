#include "rocklaunch/core/runners/runners.h"

#include "rocklaunch/core/runners/runners_internal.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/runners/runner_assets.h"
#include "rocklaunch/core/utils/atomic_file.h"
#include "rocklaunch/core/utils/downloader.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace rocklaunch
{
namespace Runners
{

namespace
{

constexpr int kReleaseTtlHours = 24;
constexpr int kReleasesPerRepo = 100;
constexpr const char *kCacheFileName = "runner_releases.json";
constexpr const char *kTimestampFormat = "%Y-%m-%dT%H:%M:%SZ";

const std::vector<std::string> kRepos = {
    "GloriousEggroll/proton-ge-custom",
    "CachyOS/Proton-CachyOS",
};

fs::path CachePath()
{
    return ConfigStore::DefaultDataDir() / kCacheFileName;
}

// The source is the repo name: it names the cache rows and the install directory.
std::string SourceFromRepo(const std::string &repo)
{
    const std::size_t slash = repo.find_last_of('/');
    return slash == std::string::npos ? repo : repo.substr(slash + 1);
}

std::string CurrentTimestamp()
{
    const std::time_t now = std::time(nullptr);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), kTimestampFormat, std::gmtime(&now));
    return buffer;
}

// A missing or unreadable cache reads as empty, which is stale.
nlohmann::json ReadCache()
{
    std::ifstream file(CachePath());
    if (!file.is_open()) {
        return nlohmann::json::object();
    }

    const nlohmann::json cache = nlohmann::json::parse(file, nullptr, false);
    if (cache.is_object()) {
        return cache;
    }

    // Odd but survivable: the caller refreshes over it. Silently reading a corrupt
    // cache as "no cache" is what makes a bad refresh look like a clean first run.
    Logger().Warn("Runners: release cache is not a JSON object, reading it as empty: "
                  + CachePath().string());
    return nlohmann::json::object();
}

bool IsStale(const nlohmann::json &cache)
{
    if (!cache.contains("releases") || !cache["releases"].is_array()) {
        return true;
    }

    std::tm fetched = {};
    std::istringstream stamp(detail::Text(cache, "fetched_at"));
    stamp >> std::get_time(&fetched, kTimestampFormat);
    if (stamp.fail()) {
        return true;
    }

    // The stamp is written with gmtime, so it has to be read back as UTC.
    const auto age = std::chrono::system_clock::now()
        - std::chrono::system_clock::from_time_t(timegm(&fetched));
    return std::chrono::duration_cast<std::chrono::hours>(age).count() >= kReleaseTtlHours;
}

nlohmann::json ReleaseRow(const std::string &source, const ReleaseInfo &release)
{
    const std::optional<AssetInfo> tarball = runner_assets::SelectTarball(release.assets);
    const std::optional<AssetInfo> sha512 = runner_assets::SelectSha512(
        release.assets, tarball.has_value() ? tarball->name : "");

    nlohmann::json assets = nlohmann::json::array();
    for (const AssetInfo &asset : release.assets) {
        assets.push_back({
            { "name", asset.name },
            { "url", asset.downloadUrl },
            { "size", asset.size },
        });
    }

    return {
        { "source", source },
        { "name", release.tag },
        { "asset", tarball.has_value() ? tarball->name : "" },
        { "size", tarball.has_value() ? tarball->size : 0 },
        { "url", tarball.has_value() ? tarball->downloadUrl : "" },
        { "sha512", sha512.has_value() ? sha512->name : "" },
        { "sha512_url", sha512.has_value() ? sha512->downloadUrl : "" },
        { "assets", assets },
    };
}

void WriteCache(const nlohmann::json &cache)
{
    const fs::path path = CachePath();
    fs::create_directories(path.parent_path());
    WriteFileAtomic(path, cache.dump(4) + "\n",
                    "Runners: cache", "the releases list was not updated");
}

nlohmann::json RefreshCache()
{
    Logger logger;
    logger.Debug("Runners: refreshing the release cache");

    nlohmann::json releases = nlohmann::json::array();
    for (const std::string &repo : kRepos) {
        const std::string source = SourceFromRepo(repo);
        for (const ReleaseInfo &release : Downloader::ListReleases(repo, kReleasesPerRepo)) {
            releases.push_back(ReleaseRow(source, release));
        }
    }

    // Every repo answering empty means the listing failed. Writing it would pass
    // for a fresh cache and hide every runner for the next 24 h.
    if (releases.empty()) {
        std::string asked;
        for (const std::string &repo : kRepos) {
            if (!asked.empty()) {
                asked += ", ";
            }
            asked += repo;
        }
        Logger().Error("Runners: release listing came back empty from all "
                       + std::to_string(kRepos.size()) + " repos");
        throw std::runtime_error(
            "asked GitHub for: " + asked
            + "\n  the existing cache is left untouched, so try again later");
    }

    const nlohmann::json cache = {
        { "fetched_at", CurrentTimestamp() },
        { "releases", releases },
    };
    WriteCache(cache);
    logger.Info("Runners: cached " + std::to_string(releases.size()) + " releases from "
                + std::to_string(kRepos.size()) + " repos");
    return cache;
}

} // anonymous namespace

std::vector<std::string> Repos()
{
    return kRepos;
}

nlohmann::json Releases(bool force)
{
    if (!force) {
        nlohmann::json cache = ReadCache();
        if (!IsStale(cache)) {
            return cache;
        }
    }
    return RefreshCache();
}

nlohmann::json Cached()
{
    return ReadCache();
}

} // namespace Runners
} // namespace rocklaunch
