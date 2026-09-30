// Core unit tests for the runners module. HOME and XDG_* point at argv[1], and
// installs use file:// URLs, so nothing touches the real system or the network.

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/runners/runner_assets.h"
#include "rocklaunch/core/runners/runners.h"
#include "rocklaunch/core/utils/checksum.h"
#include "rocklaunch/core/utils/file_lock.h"
#include "rocklaunch/core/utils/string_util.h"

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

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

template <typename Callable>
bool Throws(Callable callable)
{
    try {
        callable();
    } catch (const std::exception &) {
        return true;
    }
    return false;
}

template <typename Exception, typename Callable>
bool ThrowsAs(Callable callable)
{
    try {
        callable();
    } catch (const Exception &) {
        return true;
    } catch (const std::exception &) {
        return false;
    }
    return false;
}

fs::path DataDir()
{
    return rocklaunch::ConfigStore::DefaultDataDir();
}

fs::path CachePath()
{
    return DataDir() / "runner_releases.json";
}

fs::path RunnersDir()
{
    return DataDir() / "runners";
}

std::string ReadLog()
{
    std::ifstream file(rocklaunch::Logger().LogFile());
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

// Each scan test starts from an empty managed tree: Installed() reads the whole
// runners dir, so leftover folders from a previous test would show up as rows.
void ResetRunners()
{
    fs::remove_all(RunnersDir());
}

void WriteFile(const fs::path &path, const std::string &contents = "")
{
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << contents;
}

// A runner the scan accepts: kind comes from which executable the folder ships.
void MakeRunner(const fs::path &rootDir, const std::string &kind)
{
    fs::create_directories(rootDir);
    WriteFile(rootDir / (kind == "proton" ? "proton" : "wine"), "#!/bin/sh\n");
    if (kind == "proton") {
        WriteFile(rootDir / "dist" / "bin" / "wine");
    }
}

std::string IsoNow()
{
    const std::time_t time = std::time(nullptr);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time));
    return buffer;
}

// The cache the CLI would have written: two sources, resolved and raw assets.
nlohmann::json SampleCache()
{
    return {
        { "fetched_at", IsoNow() },
        { "releases",
          nlohmann::json::array({
              { { "source", "proton-ge-custom" },
                { "name", "GE-Proton11-7" },
                { "asset", "GE-Proton11-7-x86_64.tar.gz" },
                { "size", 1024 },
                { "url", "file:///dev/null" },
                { "sha512", "GE-Proton11-7-x86_64.sha512sum" },
                { "sha512_url", "file:///dev/null" },
                { "assets", nlohmann::json::array({
                                { { "name", "GE-Proton11-7-x86_64.tar.gz" },
                                  { "url", "file:///dev/null" },
                                  { "size", 1024 } },
                                { { "name", "GE-Proton11-7-arm64.tar.gz" },
                                  { "url", "file:///dev/null" },
                                  { "size", 999 } } }) } },
              { { "source", "proton-ge-custom" },
                { "name", "GE-Proton10-32" },
                { "asset", "GE-Proton10-32-x86_64.tar.gz" },
                { "size", 512 },
                { "url", "file:///dev/null" },
                { "sha512", "GE-Proton10-32-x86_64.sha512sum" },
                { "sha512_url", "file:///dev/null" },
                { "assets", nlohmann::json::array() } },
              { { "source", "Proton-CachyOS" },
                { "name", "GE-Proton11-7" },
                { "asset", "" },
                { "size", 0 },
                { "url", "" },
                { "sha512", "" },
                { "sha512_url", "" },
                { "assets", nlohmann::json::array() } },
          }) },
    };
}

void WriteCache(const nlohmann::json &cache)
{
    fs::create_directories(CachePath().parent_path());
    std::ofstream out(CachePath());
    out << cache.dump(4) << '\n';
}

const nlohmann::json *Row(const nlohmann::json &rows,
                          const std::string &source,
                          const std::string &name)
{
    for (const nlohmann::json &row : rows) {
        if (row.value("source", "") == source && row.value("name", "") == name) {
            return &row;
        }
    }
    return nullptr;
}

void TestFreshCacheIsReturnedVerbatim()
{
    const nlohmann::json written = SampleCache();
    WriteCache(written);

    const nlohmann::json read = rocklaunch::Runners::Releases();

    Check(read == written, "a fresh cache is returned as stored, no refetch");
    Check(read["releases"].size() == 3, "every cached release is present");
    Check(read["releases"][0]["assets"].size() == 2,
          "the raw assets array survives, so an explicit asset needs no network");
}

void TestCachedNeverRefreshes()
{
    fs::remove_all(CachePath());
    Check(!rocklaunch::Runners::Cached().contains("releases"),
          "no cache file means an empty listing, not a network request");

    nlohmann::json stale = SampleCache();
    stale["fetched_at"] = "2000-01-01T00:00:00Z";
    WriteCache(stale);

    const nlohmann::json cached = rocklaunch::Runners::Cached();
    Check(cached == stale, "Cached() returns a stale file as stored instead of refetching");

    std::ofstream broken(CachePath());
    broken << "{ not json";
    broken.close();
    Check(!rocklaunch::Runners::Cached().contains("releases"),
          "an unreadable cache yields an empty listing rather than throwing");
}

void TestInstalledReportsAllThreeOrigins(const fs::path &root)
{
    ResetRunners();
    WriteCache(SampleCache());
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton10-32", "wine");
    MakeRunner(root / "home" / ".steam" / "steam" / "compatibilitytools.d"
               / "Steam-Proton 9.0", "proton");

    const nlohmann::json rows = rocklaunch::Runners::Installed();

    const nlohmann::json *managed = Row(rows, "proton-ge-custom", "GE-Proton11-7");
    Check(managed != nullptr, "a managed runner is listed with its repo as source");
    if (managed != nullptr) {
        Check(managed->value("kind", "") == "proton",
              "kind comes from the executable the folder ships");
        Check(managed->value("root", "")
                  == (RunnersDir() / "proton-ge-custom" / "GE-Proton11-7").string(),
              "root is the runner directory itself");
        Check(rocklaunch::EndsWith(managed->value("executable", ""), "proton"),
              "executable is the binary the launch runs");
        Check(managed->contains("size"), "an installed row carries a measured size");
    }

    const nlohmann::json *wine = Row(rows, "proton-ge-custom", "GE-Proton10-32");
    Check(wine != nullptr && wine->value("kind", "") == "wine",
          "a folder shipping wine is categorised as wine");

    const nlohmann::json *steam = Row(rows, "steam", "Steam-Proton 9.0");
    Check(steam != nullptr, "a Steam compatibility tool is listed with source steam");
}

void TestInstalledIncludesRunnersMissingFromTheCache()
{
    ResetRunners();
    WriteCache(SampleCache());
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton9-20", "proton");

    const nlohmann::json rows = rocklaunch::Runners::Installed();

    Check(Row(rows, "proton-ge-custom", "GE-Proton9-20") != nullptr,
          "an installed runner the cache does not list still appears");
    Check(Row(rows, "proton-ge-custom", "GE-Proton11-7") == nullptr,
          "a cached release that is not installed is not reported as installed");
}

void TestScanSkipsNonRunners()
{
    ResetRunners();
    WriteCache(SampleCache());
    fs::create_directories(RunnersDir() / "proton-ge-custom" / "not-a-runner");
    fs::create_directories(RunnersDir() / "proton-ge-custom" / ".locks");

    for (const nlohmann::json &row : rocklaunch::Runners::Installed()) {
        Check(row.value("name", "") != "not-a-runner",
              "a folder with no proton or wine executable is not a runner");
        Check(row.value("name", "") != ".locks",
              "the lock dir is not a runner");
    }
}

void TestInstalledIgnoresTheFlatLayout()
{
    ResetRunners();
    MakeRunner(RunnersDir() / "GE-Proton9-20", "proton");
    WriteFile(RunnersDir() / "GE-Proton9-20" / "files" / "bin" / "wine");
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");

    const nlohmann::json rows = rocklaunch::Runners::Installed(false);

    for (const nlohmann::json &row : rows) {
        Check(row.value("source", "") != "GE-Proton9-20",
              "a runner directly under runners/ is not scanned as a source");
    }
    Check(Row(rows, "proton-ge-custom", "GE-Proton11-7") != nullptr,
          "a runner in its source directory is still listed");
}

void TestInstalledSizeIsOptional()
{
    ResetRunners();
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");

    Check(rocklaunch::Runners::Installed()[0].contains("size"), "size is measured by default");
    Check(!rocklaunch::Runners::Installed(false)[0].contains("size"),
          "size is absent, not null, when it was not asked for");
}

void TestFindRefusesNamesThatEscape()
{
    ResetRunners();
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");

    Check(!rocklaunch::Runners::Find("../proton-ge-custom/GE-Proton11-7", "x").has_value(),
          "a traversing name is not found");
    Check(!rocklaunch::Runners::Find("GE-Proton11-7", "..").has_value(),
          "a traversing source is not found");
    Check(!rocklaunch::Runners::Find("GE-Proton11-7", "").has_value(),
          "a name with no source is not found");
}

void TestInstallWithoutACacheNamesNoCliCommand()
{
    ResetRunners();
    fs::remove(CachePath());
    fs::remove(rocklaunch::Logger().LogFile());

    std::string message;
    try {
        rocklaunch::Runners::Install("GE-Proton11-7", "proton-ge-custom");
    } catch (const std::exception &error) {
        message = error.what();
    }

    const std::string logged = ReadLog();
    Check(logged.find("no release named 'proton-ge-custom/GE-Proton11-7'")
              != std::string::npos,
          "the log names the release that was not in the cache: " + logged);
    Check(message.find("rocklaunch-cli") == std::string::npos
              && message.find("runner install") == std::string::npos,
          "core does not name a CLI invocation in its errors");
    Check(message.find("0 releases") != std::string::npos,
          "the throw says how much the cache holds: " + message);
    Check(message.find("no release named") == std::string::npos,
          "the throw does not repeat the log line: " + message);
    Check(logged.find("0 releases") == std::string::npos,
          "the log does not repeat the throw: " + logged);
}

void TestResolveRunnerFollowsTheProfilePair()
{
    ResetRunners();
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");

    rocklaunch::ProfileConfig profile;
    profile.runnerName = "GE-Proton11-7";
    profile.runnerSource = "proton-ge-custom";
    Check(rocklaunch::ResolveRunner(profile).kind == rocklaunch::kRunnerKindProton,
          "a profile's pair resolves to its installed runner");

    profile.runnerName = "gone";
    Check(Throws([&] { rocklaunch::ResolveRunner(profile); }),
          "a pair naming nothing installed is reported, not guessed");
}

void TestFindResolvesEachOrigin(const fs::path &root)
{
    ResetRunners();
    WriteCache(SampleCache());
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");
    MakeRunner(RunnersDir() / "Proton-CachyOS" / "GE-Proton11-7", "proton");
    MakeRunner(root / "home" / ".steam" / "steam" / "compatibilitytools.d"
               / "Steam-Proton 9.0", "proton");

    const auto managed =
        rocklaunch::Runners::Find("GE-Proton11-7", "proton-ge-custom");
    const auto cachyos = rocklaunch::Runners::Find("GE-Proton11-7", "Proton-CachyOS");
    Check(managed.has_value() && cachyos.has_value(),
          "the same name in two sources resolves to both runners");
    Check(managed.has_value() && cachyos.has_value()
              && managed->rootDir != cachyos->rootDir,
          "each pair resolves to its own directory");
    Check(!rocklaunch::Runners::Find("GE-Proton11-7", "").has_value(),
          "a bare name is not a valid pair, so core never picks a source");
    Check(!rocklaunch::Runners::Find("", "proton-ge-custom").has_value(),
          "an empty name resolves to nothing");
    Check(!rocklaunch::Runners::Find("system-wine", "system").has_value(),
          "with no wine on PATH the system runner does not exist");
    Check(rocklaunch::Runners::Find("Steam-Proton 9.0", "steam").has_value(),
          "a Steam runner is found by its own source");
    Check(rocklaunch::Runners::Find("Nope", "proton-ge-custom").has_value() == false,
          "a name with no folder resolves to nothing");
}

// Both listings read the table, so a row Installed scans has to resolve through Find
// to the very directory it was scanned at: a source declared for one and not the other lands here.
void TestEveryScannedRowResolvesBack(const fs::path &root)
{
    ResetRunners();
    WriteCache(SampleCache());
    MakeRunner(RunnersDir() / "proton-ge-custom" / "GE-Proton11-7", "proton");
    MakeRunner(RunnersDir() / "Proton-CachyOS" / "GE-Proton10-32", "wine");
    MakeRunner(root / "home" / ".steam" / "steam" / "compatibilitytools.d"
               / "Steam-Proton 9.0", "proton");

    for (const nlohmann::json &row : rocklaunch::Runners::Installed()) {
        const std::string pair = row.value("source", "") + "/" + row.value("name", "");
        const auto resolved =
            rocklaunch::Runners::Find(row.value("name", ""), row.value("source", ""));
        Check(resolved.has_value(), pair + " is resolvable as the pair Installed reported");
        Check(!resolved.has_value() || resolved->rootDir == fs::path(row.value("root", "")),
              pair + " resolves to the directory it was scanned at");
    }
}

void TestRemoveOnlyTouchesManagedRunners(const fs::path &root)
{
    ResetRunners();
    WriteCache(SampleCache());
    const fs::path managed = RunnersDir() / "proton-ge-custom" / "GE-Proton11-7";
    const fs::path steam = root / "home" / ".steam" / "steam" / "compatibilitytools.d"
        / "Steam-Proton 9.0";
    MakeRunner(managed, "proton");
    MakeRunner(steam, "proton");

    const fs::path outside = root / "outside" / "keepme";
    MakeRunner(outside, "proton");

    Check(Throws([&] { rocklaunch::Runners::Remove("Steam-Proton 9.0", "steam"); }),
          "Remove refuses a Steam runner");
    Check(Throws([&] { rocklaunch::Runners::Remove("system-wine", "system"); }),
          "Remove refuses the system wine entry");
    Check(Throws([&] { rocklaunch::Runners::Remove("../outside/keepme", "proton-ge-custom"); }),
          "a name that would escape the runners dir is refused");
    Check(fs::exists(steam), "the Steam runner survives the refused removals");
    Check(fs::exists(outside), "the directory outside runners/ is untouched");

    rocklaunch::Runners::Remove("GE-Proton11-7", "proton-ge-custom");
    Check(!fs::exists(managed), "a managed runner is removed");

    Check(Throws([&] { rocklaunch::Runners::Remove("GE-Proton11-7", "proton-ge-custom"); }),
          "removing an absent runner reports it instead of succeeding");
}

// An install renames onto the target, so a remove ignoring the lock could delete
// the directory the install just produced.
void TestRemoveRefusesALockedRunner()
{
    ResetRunners();
    WriteCache(SampleCache());
    const fs::path managed = RunnersDir() / "proton-ge-custom" / "GE-Proton11-7";
    MakeRunner(managed, "proton");

    {
        rocklaunch::PathLock held(managed);
        Check(held.Acquired(), "the test holds the runner lock");

        Check(Throws([&] { rocklaunch::Runners::Remove("GE-Proton11-7", "proton-ge-custom"); }),
              "a remove of a runner being installed is refused");
    }

    rocklaunch::Runners::Remove("GE-Proton11-7", "proton-ge-custom");
    Check(!fs::exists(managed), "the remove succeeds once the lock is released");
}

void TestInstallUsesTheCacheWithoutRefetching()
{
    ResetRunners();
    nlohmann::json cache = SampleCache();
    cache["releases"][0]["asset"] = "GE-Proton11-7-x86_64.tar.gz";
    cache["releases"][0]["size"] = 0;
    cache["releases"][0]["url"] = "";
    WriteCache(cache);

    // url is empty on purpose: the failure under test is the missing asset, not
    // the download, and the manual assetName override is what recovers it.
    Check(Throws([&] { rocklaunch::Runners::Install("GE-Proton11-7", "proton-ge-custom"); }),
          "installing a release with no asset for this host is refused");
    Check(Throws([&] {
        rocklaunch::Runners::Install("GE-Proton11-7", "proton-ge-custom", "nope.tar.gz");
    }), "an asset the release does not ship is refused");
    Check(Throws([&] { rocklaunch::Runners::Install("Absent", "proton-ge-custom"); }),
          "installing a release the cache does not list is refused");
    Check(Throws([&] { rocklaunch::Runners::Install("GE-Proton11-7", "../escape"); }),
          "a source that would escape the runners dir is refused");
}

// A real tarball and its hash, served over file:// like a release asset, wired into
// the first cache row. The files are named after folderName so the sha512 pairing rule,
// which matches real release naming, applies as it would in production.
// Returns false when tar is unavailable.
bool PrepareLocalRelease(const fs::path &root,
                         const std::string &assetsDir,
                         const std::string &folderName,
                         const std::string &releaseName,
                         bool correctHash,
                         nlohmann::json &cache)
{
    const fs::path payload = root / assetsDir / "payload" / folderName;
    fs::remove_all(payload);
    fs::create_directories(payload / "dist" / "bin");
    WriteFile(payload / "proton", "#!/bin/sh\n");
    WriteFile(payload / "dist" / "bin" / "wine");

    const fs::path assets = root / assetsDir;
    fs::create_directories(assets);
    const fs::path tarball = assets / (folderName + ".tar.gz");
    if (std::system(("tar -czf " + tarball.string() + " -C "
                     + payload.parent_path().string() + " " + folderName + " 2>/dev/null")
                        .c_str())
        != 0) {
        return false;
    }

    const fs::path sum = assets / (folderName + ".sha512sum");
    std::ofstream out(sum);
    out << (correctHash ? rocklaunch::HashFile(tarball.string())
                        : std::string(128, '0'))
        << "  " << tarball.filename().string() << '\n';
    out.close();

    nlohmann::json &release = cache["releases"][0];
    release["name"] = releaseName;
    release["asset"] = tarball.filename().string();
    release["url"] = "file://" + tarball.string();
    release["sha512"] = sum.filename().string();
    release["sha512_url"] = "file://" + sum.string();
    release["assets"] = nlohmann::json::array({
        { { "name", tarball.filename().string() },
          { "url", "file://" + tarball.string() },
          { "size", 1 } },
        { { "name", sum.filename().string() },
          { "url", "file://" + sum.string() },
          { "size", 1 } },
    });
    return true;
}

void TestInstallFromLocalAssets(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "assets", "GE-Proton11-7-x86_64", "GE-Proton11-7",
                             true, cache)) {
        std::cerr << "SKIP: tar unavailable, install round trip not exercised\n";
        return;
    }
    WriteCache(cache);

    rocklaunch::Runners::Install("GE-Proton11-7", "proton-ge-custom");

    const fs::path target = RunnersDir() / "proton-ge-custom" / "GE-Proton11-7";
    Check(fs::exists(target / "proton"), "the tarball is extracted into the target");
    Check(fs::exists(target / "dist" / "bin" / "wine"),
          "the payload keeps its layout, so the launch finds its wine");
    Check(!fs::exists(RunnersDir() / "proton-ge-custom" / ".tmp-download-GE-Proton11-7"),
          "the scratch dir is gone after a successful install");

    const nlohmann::json *row = Row(rocklaunch::Runners::Installed(),
                                    "proton-ge-custom", "GE-Proton11-7");
    Check(row != nullptr, "the freshly installed runner is listed as installed");
}

// A file named by hand may be a different build than the cache picked, and the cached
// hash is that build's: the install re-derives the hash from the file, so the download
// is still verified instead of failing against the wrong digest.
void TestInstallOfANamedFileVerifiesThatFilesHash(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "namedfile-assets", "RunnerV3-x86_64", "RunnerV3",
                             true, cache)) {
        std::cerr << "SKIP: tar unavailable, named-file install not exercised\n";
        return;
    }

    // A second real build, listed in assets so the override can name it, but not the
    // one the cache points at. Its hash differs, so verifying one against the other
    // is exactly what this test would catch.
    const fs::path assets = root / "namedfile-assets";
    const fs::path v3Payload = assets / "payload" / "RunnerV3-x86_64_v3";
    fs::create_directories(v3Payload / "dist" / "bin");
    WriteFile(v3Payload / "proton", "#!/bin/sh\nv3\n");
    WriteFile(v3Payload / "dist" / "bin" / "wine", "v3\n");
    const fs::path v3Tarball = assets / "RunnerV3-x86_64_v3.tar.gz";
    if (std::system(("tar -czf " + v3Tarball.string() + " -C "
                     + (assets / "payload").string() + " RunnerV3-x86_64_v3 2>/dev/null")
                        .c_str())
        != 0) {
        std::cerr << "SKIP: tar unavailable, named-file install not exercised\n";
        return;
    }
    const fs::path v3Sum = assets / "RunnerV3-x86_64_v3.sha512sum";
    {
        std::ofstream out(v3Sum);
        out << rocklaunch::HashFile(v3Tarball.string()) << "  RunnerV3-x86_64_v3.tar.gz\n";
    }

    nlohmann::json &release = cache["releases"][0];
    release["assets"].push_back({
        { "name", v3Tarball.filename().string() },
        { "url", "file://" + v3Tarball.string() },
        { "size", 1 },
    });
    release["assets"].push_back({
        { "name", v3Sum.filename().string() },
        { "url", "file://" + v3Sum.string() },
        { "size", 1 },
    });
    WriteCache(cache);

    rocklaunch::Runners::Install("RunnerV3", "proton-ge-custom", v3Tarball.filename().string());

    const fs::path target = RunnersDir() / "proton-ge-custom" / "RunnerV3";
    std::string proton;
    {
        std::ifstream in(target / "proton");
        proton.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    Check(proton == "#!/bin/sh\nv3\n",
          "the named file is the one installed, verified against its own hash");
}

void TestInstallRejectsABadHash(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "badhash-assets", "RunnerX", "RunnerX", false, cache)) {
        std::cerr << "SKIP: tar unavailable, hash check not exercised\n";
        return;
    }
    WriteCache(cache);

    Check(Throws([&] { rocklaunch::Runners::Install("RunnerX", "proton-ge-custom"); }),
          "a sha512 mismatch fails the install");
    Check(!fs::exists(RunnersDir() / "proton-ge-custom" / "RunnerX"),
          "a failed install leaves no runner behind");
    Check(!fs::exists(RunnersDir() / "proton-ge-custom" / ".tmp-download-RunnerX"),
          "a failed install cleans its scratch dir");
}

// The asset names become path components under the scratch dir, and the hash check
// runs after the write, so "../../" would put attacker-chosen bytes outside the tree.
void TestInstallRefusesAssetNamesThatEscape(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "escape-assets", "RunnerY", "RunnerY", true, cache)) {
        std::cerr << "SKIP: tar unavailable, asset name escape not exercised\n";
        return;
    }

    nlohmann::json &release = cache["releases"][0];
    const std::string tarball = release["asset"];
    const std::string sum = release["sha512"];

    release["asset"] = "../../escaped.tar.gz";
    WriteCache(cache);
    Check(Throws([&] { rocklaunch::Runners::Install("RunnerY", "proton-ge-custom"); }),
          "a tarball asset name escaping the scratch dir is refused");
    Check(!fs::exists(RunnersDir() / "escaped.tar.gz"),
          "the tarball download never left the scratch dir");

    release["asset"] = tarball;
    release["sha512"] = "../../escaped.sha512sum";
    WriteCache(cache);
    Check(Throws([&] { rocklaunch::Runners::Install("RunnerY", "proton-ge-custom"); }),
          "a hash asset name escaping the scratch dir is refused");
    Check(!fs::exists(RunnersDir() / "escaped.sha512sum"),
          "the hash download never left the scratch dir");

    Check(!fs::exists(RunnersDir() / "proton-ge-custom" / "RunnerY"),
          "a refused asset name installs nothing");
}

// Cancelling is only honoured at a stage boundary, and Verifying/Extracting are
// the two that measure nothing, so a cancel there has no other chance to land.
void TestInstallCancelsAtEveryStage(const fs::path &root)
{
    const rocklaunch::ProgressStage stages[] = {
        rocklaunch::ProgressStage::Downloading,
        rocklaunch::ProgressStage::Verifying,
        rocklaunch::ProgressStage::Extracting,
        rocklaunch::ProgressStage::Installing,
    };

    for (const rocklaunch::ProgressStage stage : stages) {
        ResetRunners();

        nlohmann::json cache = SampleCache();
        if (!PrepareLocalRelease(root, "cancel-assets", "RunnerY-x86_64", "RunnerY",
                                 true, cache)) {
            std::cerr << "SKIP: tar unavailable, stage cancels not exercised\n";
            return;
        }
        WriteCache(cache);

        const std::string stageName = rocklaunch::StageName(stage);
        const rocklaunch::ProgressCallback cancel = [stageName](
                                                        const rocklaunch::Progress &progress) {
            return rocklaunch::StageName(progress.stage) != stageName;
        };

        Check(ThrowsAs<rocklaunch::Cancelled>([&] {
                  rocklaunch::Runners::Install("RunnerY", "proton-ge-custom", "", cancel);
              }),
              "a cancel during " + stageName + " throws Cancelled, not a plain error");
        Check(!fs::exists(RunnersDir() / "proton-ge-custom" / "RunnerY"),
              "a cancelled install leaves no runner behind, cancelling " + stageName);
        Check(!fs::exists(RunnersDir() / "proton-ge-custom" / ".tmp-download-RunnerY"),
              "a cancelled install cleans its scratch dir, cancelling " + stageName);
    }
}

// The held lock stands in for an install already in flight, which is the only way
// the install path is ever occupied.
void TestInstallRefusesALockedRunner(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "lock-assets", "RunnerZ-x86_64", "RunnerZ", true, cache)) {
        std::cerr << "SKIP: tar unavailable, install lock not exercised\n";
        return;
    }
    WriteCache(cache);

    const fs::path target = RunnersDir() / "proton-ge-custom" / "RunnerZ";
    {
        rocklaunch::PathLock held(target);
        Check(held.Acquired(), "the test holds the runner lock");

        Check(Throws([&] { rocklaunch::Runners::Install("RunnerZ", "proton-ge-custom"); }),
              "a second install of a locked runner is refused");
        Check(!fs::exists(target), "the refused install wrote nothing");
    }

    rocklaunch::Runners::Install("RunnerZ", "proton-ge-custom");
    Check(fs::exists(target / "proton"), "the install succeeds once the lock is released");
}

// The leftover scratch dir stands in for an install killed on a signal, whose
// lock died with it and left nothing to say it is gone.
void TestInstallSweepsAbandonedScratch(const fs::path &root)
{
    ResetRunners();

    nlohmann::json cache = SampleCache();
    if (!PrepareLocalRelease(root, "sweep-assets", "RunnerW-x86_64", "RunnerW", true, cache)) {
        std::cerr << "SKIP: tar unavailable, scratch sweep not exercised\n";
        return;
    }
    WriteCache(cache);

    const fs::path scratch = RunnersDir() / "proton-ge-custom" / ".tmp-download-RunnerW";
    fs::create_directories(scratch / "extracted" / "junk");
    WriteFile(scratch / "half-downloaded.tar.gz", "partial");

    rocklaunch::Runners::Install("RunnerW", "proton-ge-custom");

    Check(!fs::exists(scratch), "the abandoned scratch dir is gone after the install");
    Check(fs::exists(RunnersDir() / "proton-ge-custom" / "RunnerW" / "proton"),
          "the install produced the runner anyway");
}

void TestAssetSelection()
{
    const std::vector<rocklaunch::AssetInfo> assets = {
        { "GE-Proton11-7-x86_64.tar.gz", "u1", 1 },
        { "GE-Proton11-7-x86_64.sha512sum", "u2", 2 },
        { "GE-Proton11-7-aarch64.tar.gz", "u3", 3 },
        { "GE-Proton11-7-source.tar.gz", "u4", 4 },
    };

    const auto tarball = rocklaunch::runner_assets::SelectTarball(assets);
    Check(tarball.has_value() && tarball->name == "GE-Proton11-7-x86_64.tar.gz",
          "the host-arch tarball is picked, and never the sha512sum");

    const auto hash = rocklaunch::runner_assets::SelectSha512(assets);
    Check(hash.has_value() && hash->name == "GE-Proton11-7-x86_64.sha512sum",
          "the sha512sum asset is found");

    const std::vector<rocklaunch::AssetInfo> noHash = {
        { "Runner-1.0-x86_64.tar.gz", "u1", 1 },
    };
    Check(rocklaunch::runner_assets::SelectSha512(noHash).has_value() == false,
          "a release without a hash reports none");
    Check(rocklaunch::runner_assets::SelectTarball(noHash).has_value(),
          "a single-arch release still resolves to a tarball");
}

// Several hashes and none for the host arch: the hash paired with the tarball is the
// only unambiguous answer left.
void TestSha512FallsBackToPairingWithTheTarball()
{
    const std::vector<rocklaunch::AssetInfo> assets = {
        { "Runner-1.0-x86_64.tar.gz", "u1", 1 },
        { "Runner-1.0-aarch64.tar.gz", "u2", 1 },
        { "Runner-1.0-x86_64_v3.tar.gz", "u3", 1 },
        { "Runner-1.0-x86_64.sha512sum", "h1", 1 },
        { "Runner-1.0-aarch64.sha512sum", "h2", 1 },
        { "Runner-1.0-x86_64_v3.sha512sum", "h3", 1 },
    };

    Check(rocklaunch::runner_assets::Sha512NameFor("Runner-1.0-x86_64.tar.gz")
              == "Runner-1.0-x86_64.sha512sum",
          "the hash name is the tarball stem plus the hash extension");
    Check(rocklaunch::runner_assets::Sha512NameFor("Runner-1.0.tar.gz")
              == "Runner-1.0.sha512sum",
          "a single-arch name pairs the same way");
    Check(rocklaunch::runner_assets::Sha512NameFor("Runner-1.0.sha512sum").empty(),
          "a name that is not a tarball has no paired hash");
    Check(rocklaunch::runner_assets::Sha512NameFor("").empty(),
          "an empty tarball name has no paired hash");

    const auto paired = rocklaunch::runner_assets::Sha512For(assets, "Runner-1.0-x86_64.tar.gz");
    Check(paired.has_value() && paired->name == "Runner-1.0-x86_64.sha512sum",
          "the hash with the matching stem is the pair");

    // v3 is a whole different arch token, so it must not be taken for the v3-less
    // tarball, nor the other way round.
    const auto v3 = rocklaunch::runner_assets::Sha512For(assets, "Runner-1.0-x86_64_v3.tar.gz");
    Check(v3.has_value() && v3->name == "Runner-1.0-x86_64_v3.sha512sum",
          "the v3 tarball pairs with the v3 hash, not the shorter stem");
    Check(rocklaunch::runner_assets::Sha512For(assets, "Runner-1.0-riscv64.tar.gz")
              .has_value() == false,
          "a tarball the release ships no hash for reports none");
}

// The pairing fallback only runs when the arch rule cannot pick, so the hashes here
// carry no arch token.
void TestSelectSha512AgreesWithThePairing()
{
    const std::vector<rocklaunch::AssetInfo> assets = {
        { "Runner-1.0.tar.gz", "u1", 1 },
        { "Runner-1.0-v3.tar.gz", "u2", 1 },
        { "Runner-1.0.sha512sum", "h1", 1 },
        { "Runner-1.0-v3.sha512sum", "h2", 1 },
    };

    const auto paired = rocklaunch::runner_assets::SelectSha512(assets, "Runner-1.0-v3.tar.gz");
    Check(paired.has_value() && paired->name == "Runner-1.0-v3.sha512sum",
          "with no arch match the hash that pairs with the tarball is used");
    Check(rocklaunch::runner_assets::SelectSha512(assets, "Runner-1.0-riscv64.tar.gz")
              .has_value() == false,
          "no arch match and no pair leaves the release without a hash");
}

// A PATH holding only the tools the install path shells out to: the host's wine must
// stay out of Installed()/Find(), while RunSubprocess needs tar and `tar -czf` needs gzip.
fs::path MakeToolPath(const fs::path &root)
{
    const fs::path tools = root / "tool-bin";
    fs::create_directories(tools);

    const char *hostPath = std::getenv("PATH");
    if (hostPath == nullptr) {
        return tools;
    }

    std::istringstream paths(hostPath);
    std::string directory;
    std::vector<fs::path> hosts;
    while (std::getline(paths, directory, ':')) {
        for (const char *tool : { "tar", "gzip" }) {
            const fs::path candidate = fs::path(directory) / tool;
            std::error_code error;
            if (fs::is_regular_file(candidate, error)) {
                hosts.push_back(fs::canonical(candidate, error));
            }
        }
    }

    for (const fs::path &host : hosts) {
        const fs::path link = tools / host.filename();
        std::error_code error;
        if (fs::exists(fs::symlink_status(link, error)) && !error) {
            continue;
        }
        fs::create_symlink(host, link, error);
        if (error) {
            fs::copy_file(host, link, fs::copy_options::overwrite_existing, error);
        }
    }

    return tools;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    const fs::path testRoot = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "runners-test";
    fs::remove_all(testRoot);
    fs::create_directories(testRoot / "home");
    fs::create_directories(testRoot / "data");

    const fs::path toolPath = MakeToolPath(testRoot);

    setenv("HOME", (testRoot / "home").c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").c_str(), 1);
    setenv("PATH", toolPath.c_str(), 1);

    TestFreshCacheIsReturnedVerbatim();
    TestCachedNeverRefreshes();
    TestInstalledReportsAllThreeOrigins(testRoot);
    TestInstalledIncludesRunnersMissingFromTheCache();
    TestScanSkipsNonRunners();
    TestInstalledIgnoresTheFlatLayout();
    TestInstalledSizeIsOptional();
    TestFindRefusesNamesThatEscape();
    TestInstallWithoutACacheNamesNoCliCommand();
    TestResolveRunnerFollowsTheProfilePair();
    TestFindResolvesEachOrigin(testRoot);
    TestEveryScannedRowResolvesBack(testRoot);
    TestRemoveOnlyTouchesManagedRunners(testRoot);
    TestInstallUsesTheCacheWithoutRefetching();
    TestInstallFromLocalAssets(testRoot);
    TestInstallOfANamedFileVerifiesThatFilesHash(testRoot);
    TestInstallRejectsABadHash(testRoot);
    TestInstallRefusesAssetNamesThatEscape(testRoot);
    TestInstallCancelsAtEveryStage(testRoot);
    TestInstallRefusesALockedRunner(testRoot);
    TestInstallSweepsAbandonedScratch(testRoot);
    TestRemoveRefusesALockedRunner();
    TestAssetSelection();
    TestSha512FallsBackToPairingWithTheTarball();
    TestSelectSha512AgreesWithThePairing();

    std::cout << (gFailures == 0 ? "PASS" : "FAIL") << ": " << gChecks - gFailures << '/'
              << gChecks << " checks\n";
    return gFailures == 0 ? 0 : 1;
}
