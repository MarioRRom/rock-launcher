// Core unit tests for the download layer. Progress and cancellation have no CLI
// surface, so these run as a core unit binary, like LaunchSession.
//
// NO NETWORK: libcurl speaks the FILE protocol, so a local file drives the same
// path as a release asset — real transfer-info callbacks, real byte counts,
// real cancellation — with no curl binary on PATH.
//
// ISOLATED: the test root is argv[1], and HOME / XDG_* point at it, exactly
// like the CLI test scripts.

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/utils/downloader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

namespace fs = std::filesystem;

int gChecks = 0;
int gFailures = 0;

void Check(bool condition, const std::string &what)
{
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::cerr << "FAIL: " << what << '\n';
    }
}

// A file big enough that libcurl reports it in many chunks rather than one.
constexpr std::size_t kPayloadBytes = 4u * 1024u * 1024u;

fs::path MakePayload(const fs::path &dir, const std::string &name,
                     std::size_t bytes)
{
    std::ofstream out(dir / name, std::ios::binary);
    for (std::size_t i = 0; i < bytes; ++i) {
        out.put(static_cast<char>('a' + (i % 26)));
    }
    out.close();
    return dir / name;
}

std::string FileUrl(const fs::path &path)
{
    return "file://" + path.string();
}

// The log message without its "YYYY-MM-DD HH:MM:SS [LEVEL] " prefix, so a
// search for digits cannot match the clock.
std::string ReadLogMessages()
{
    std::ifstream file(rocklaunch::Logger().LogFile());
    std::ostringstream out;
    out << file.rdbuf();
    const std::string line = out.str();
    const std::size_t start = line.find("] ");
    return start == std::string::npos ? line : line.substr(start + 2);
}

// Progress must move forward and end on a full bar, even though Fetch
// synthesises the closing tick rather than receiving it from libcurl.
void TestProgressReachesCompletion(const fs::path &dir)
{
    const fs::path source = MakePayload(dir, "payload.bin", kPayloadBytes);
    const fs::path dest = dir / "out" / "downloaded.bin";

    std::vector<rocklaunch::Progress> ticks;
    rocklaunch::Downloader::Fetch(FileUrl(source), dest,
        [&ticks](const rocklaunch::Progress &progress) {
            ticks.push_back(progress);
            return true;
        });

    Check(!ticks.empty(), "a transfer reports at least one tick");
    Check(fs::exists(dest), "the destination exists after a successful fetch");
    Check(fs::file_size(dest) == kPayloadBytes, "every byte arrived");

    bool allDownloading = true;
    bool sawPartial = false;
    bool monotonic = true;
    uint64_t previous = 0;
    for (const rocklaunch::Progress &tick : ticks) {
        if (tick.bytesTransferred > 0
            && tick.bytesTransferred < kPayloadBytes) {
            sawPartial = true;
        }
        if (tick.stage != rocklaunch::ProgressStage::Downloading) {
            allDownloading = false;
        }
        if (tick.bytesTransferred < previous) {
            monotonic = false;
        }
        previous = tick.bytesTransferred;
    }
    Check(sawPartial, "progress is reported incrementally, not just at the end");
    Check(allDownloading, "ticks are labelled Downloading");
    Check(monotonic, "byte counts never move backwards");

    const rocklaunch::Progress &last = ticks.back();
    Check(last.percentage == 100.0, "the last tick is 100 %");
    Check(last.totalBytes == kPayloadBytes, "the total matches the file size");
    Check(last.bytesTransferred == kPayloadBytes,
          "the closing tick reports the real byte count");
    Check(last.bytesPerSecond > 0, "a speed is reported");
    Check(last.file == "downloaded.bin",
          "the tick names the file so a consumer can tell transfers apart");
}

// A transfer with no known total must stay indeterminate rather than report a
// bogus 0 % that looks frozen, and must still land on 100 % at the end.
void TestUnknownTotalIsIndeterminate(const fs::path &dir)
{
    const fs::path source = MakePayload(dir, "small.bin", 1024);
    const fs::path dest = dir / "out" / "small.bin";

    std::vector<rocklaunch::Progress> ticks;
    rocklaunch::Downloader::Fetch(FileUrl(source), dest,
        [&ticks](const rocklaunch::Progress &progress) {
            ticks.push_back(progress);
            return true;
        });

    // Fetch reports the final size as the total, which is what rescues a bar
    // that had nothing to show while the transfer ran.
    Check(ticks.back().totalBytes == 1024,
          "the closing tick supplies the total for an indeterminate transfer");
    Check(ticks.back().percentage == 100.0,
          "an indeterminate transfer still ends on 100 %");
}

// Which tick the cancel lands on is libcurl's business, so the assertion is
// that the transfer stops well short of a full one.
void TestCallbackCancelsTransfer(const fs::path &dir)
{
    const fs::path source = MakePayload(dir, "cancel-me.bin", kPayloadBytes);

    int fullTicks = 0;
    rocklaunch::Downloader::Fetch(FileUrl(source), dir / "out" / "baseline.bin",
        [&fullTicks](const rocklaunch::Progress &) {
            ++fullTicks;
            return true;
        });

    const fs::path dest = dir / "out" / "cancelled.bin";
    int calls = 0;
    bool threw = false;
    std::string message;
    try {
        rocklaunch::Downloader::Fetch(FileUrl(source), dest,
            [&calls](const rocklaunch::Progress &) {
                ++calls;
                return false;
            });
    } catch (const std::runtime_error &error) {
        threw = true;
        message = error.what();
    }

    Check(threw, "cancelling a transfer throws");
    Check(calls > 0, "the reporter was called before the cancel");
    Check(calls < fullTicks,
          "a cancelled transfer stops far short of a full one");
    Check(message.find("cancelled") != std::string::npos,
          "the error says it was cancelled, not that it failed");
    Check(!fs::exists(dest),
          "a cancelled transfer leaves no destination file");
    Check(!fs::exists(dest.string() + ".tmp"),
          "a cancelled transfer cleans up its temp file");
}

// An interrupted download must not leave a truncated file under the real name
// for a later run to trust.
void TestDestinationIsAtomic(const fs::path &dir)
{
    const fs::path source = MakePayload(dir, "atomic.bin", 64 * 1024);
    const fs::path dest = dir / "out" / "atomic.bin";

    // A stale temp file from an earlier run must not survive the fetch.
    fs::create_directories(dest.parent_path());
    {
        std::ofstream stale(dest.string() + ".tmp");
        stale << "leftover garbage";
    }

    rocklaunch::Downloader::Fetch(FileUrl(source), dest);

    Check(fs::file_size(dest) == 64 * 1024, "the destination is the real file");
    Check(!fs::exists(dest.string() + ".tmp"),
          "no temp file is left behind on success");
}

// No callback is the common case (the CDLC patch uses it) and must not crash
// or change the outcome.
void TestNoCallbackIsFine(const fs::path &dir)
{
    const fs::path source = MakePayload(dir, "silent.bin", 4096);
    const fs::path dest = dir / "out" / "silent.bin";

    rocklaunch::Downloader::Fetch(FileUrl(source), dest);

    Check(fs::file_size(dest) == 4096, "fetching without a callback works");
}

// A missing source is the file:// analogue of an HTTP error, reported as
// CURLE_FILE_COULDNT_READ_FILE (37) — not CURLE_READ_ERROR (26), which means a
// read callback failed.
void TestMissingSourceReportsReadFailure(const fs::path &dir)
{
    const fs::path missing = dir / "does-not-exist.bin";
    const fs::path dest = dir / "out" / "never-written.bin";

    // The logfile appends, so an earlier test's lines would pollute the search.
    fs::remove(rocklaunch::Logger().LogFile());

    bool threw = false;
    std::string message;
    try {
        rocklaunch::Downloader::Fetch(FileUrl(missing), dest);
    } catch (const std::runtime_error &error) {
        threw = true;
        message = error.what();
    }
    std::string logged = ReadLogMessages();

    Check(threw, "fetching a missing source throws");
    Check(logged.find("Cannot read file") != std::string::npos,
          "the log names the real cause instead of a bare curl number: " + logged);
    Check(logged.find("37") == std::string::npos,
          "the log does not leak the bare curl number: " + logged);
    Check(message.find("never-written.bin") != std::string::npos,
          "the throw says the partial file was cleaned up: " + message);
    Check(!fs::exists(dest), "a failed transfer writes no destination");
    Check(!fs::exists(dest.string() + ".tmp"),
          "a failed transfer cleans up its temp file");
}

// The labels are what a UI shows, so they are pinned.
void TestStageNames()
{
    Check(std::string(rocklaunch::StageName(rocklaunch::ProgressStage::Downloading))
              == "Downloading", "Downloading label");
    Check(std::string(rocklaunch::StageName(rocklaunch::ProgressStage::Verifying))
              == "Verifying", "Verifying label");
    Check(std::string(rocklaunch::StageName(rocklaunch::ProgressStage::Extracting))
              == "Extracting", "Extracting label");
    Check(std::string(rocklaunch::StageName(rocklaunch::ProgressStage::Installing))
              == "Installing", "Installing label");
    Check(std::string(rocklaunch::StageName(rocklaunch::ProgressStage::Resolving))
              == "Resolving", "Resolving label");
}

} // namespace

int main(int argc, char **argv)
{
    const fs::path testRoot = argc > 1
        ? fs::path(argv[1])
        : fs::path("/tmp/rocklaunch-downloader-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);
    fs::create_directories(testRoot / "config", error);
    fs::create_directories(testRoot / "work", error);

    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);
    setenv("XDG_CONFIG_HOME", (testRoot / "config").string().c_str(), 1);

    const fs::path work = testRoot / "work";

    TestProgressReachesCompletion(work);
    TestUnknownTotalIsIndeterminate(work);
    TestCallbackCancelsTransfer(work);
    TestDestinationIsAtomic(work);
    TestNoCallbackIsFine(work);
    TestMissingSourceReportsReadFailure(work);
    TestStageNames();

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
