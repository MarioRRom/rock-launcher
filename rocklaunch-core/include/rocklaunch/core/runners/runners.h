#pragma once

#include "rocklaunch/core/progress.h"

#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

constexpr const char *kRunnerKindProton = "proton";
constexpr const char *kRunnerKindWine = "wine";

// Sources of runners the launcher did not install. Managed runners carry the name
// of their repo instead.
constexpr const char *kSteamRunnerSource = "steam";
constexpr const char *kSystemRunnerSource = "system";
constexpr const char *kSystemRunnerName = "system-wine";

// A runner resolved from the (name, source) pair a profile stores. A name is only
// unique within its source, so core never resolves a bare one.
struct RunnerRef
{
    std::string name;
    std::string source;
    std::string kind;
    fs::path rootDir;
    fs::path executable;
};

// Releases available to install, runners present on this machine, and install and
// remove. Both listings are plain JSON rows sharing "source" and "name"; a key that
// cannot exist for a row is absent, never null.
namespace Runners
{

// GitHub "owner/repo" slugs the launcher installs from.
std::vector<std::string> Repos();

// One row per release of every repo. Regenerates the cache file when it is missing,
// unreadable, older than 24 h, or when force is set, all of which need the network.
//
//   source      repo name, also the install directory
//   name        release tag
//   asset       tarball for this host, "" when the release has none
//   size        that tarball's size in bytes
//   url         its download URL
//   sha512      the .sha512sum asset name, "" when the release ships none
//   sha512_url  its download URL
//   assets      every asset as {name, url, size}
nlohmann::json Releases(bool force = false);

// The cache exactly as stored, or an empty object. Never refreshes, so it never
// fails for network reasons.
nlohmann::json Cached();

// One row per runner on this machine, whoever installed it, with these keys added:
//
//   kind        "proton" or "wine", from the executable the folder ships
//   root        the runner directory
//   executable  the binary a launch runs
//   size        bytes on disk; walking the tree is slow, so withSize = false omits it
//
// A runner whose release the cache no longer lists is still reported.
nlohmann::json Installed(bool withSize = true);

// nullopt when the pair matches nothing installed, or is not a valid pair.
std::optional<RunnerRef> Find(const std::string &name, const std::string &source);

// Downloads, verifies and extracts into <runners>/<source>/<name>, replacing any
// previous install. Uses Cached(), so it spends no API request. fileName picks
// another file of the release than the cached choice; its own hash is then used.
void Install(const std::string &name,
             const std::string &source,
             const std::string &fileName = "",
             ProgressCallback onProgress = nullptr);

// Deletes an installed runner. Refuses Steam and system runners, and a runner that
// is being installed.
void Remove(const std::string &name, const std::string &source);

} // namespace Runners
} // namespace rocklaunch
