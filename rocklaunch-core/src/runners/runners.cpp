#include "rocklaunch/core/runners/runners.h"

#include "rocklaunch/core/runners/runners_internal.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/runners/runner_assets.h"
#include "rocklaunch/core/runners/runner_installer.h"
#include "rocklaunch/core/utils/file_lock.h"

#include <stdexcept>
#include <utility>

namespace rocklaunch
{
namespace Runners
{

namespace
{

struct Asset
{
    std::string name;
    std::string url;

    bool Usable() const { return !name.empty() && !url.empty(); }
};

const nlohmann::json *FindRelease(const nlohmann::json &cache, const std::string &source,
                                  const std::string &name)
{
    const auto releases = cache.find("releases");
    if (releases == cache.end() || !releases->is_array()) {
        return nullptr;
    }
    for (const nlohmann::json &release : *releases) {
        if (detail::Text(release, "source") == source && detail::Text(release, "name") == name) {
            return &release;
        }
    }
    return nullptr;
}

Asset FindAsset(const nlohmann::json &release, const std::string &assetName)
{
    const auto assets = release.find("assets");
    if (assets == release.end() || !assets->is_array()) {
        return {};
    }
    for (const nlohmann::json &asset : *assets) {
        if (detail::Text(asset, "name") == assetName) {
            return { assetName, detail::Text(asset, "url") };
        }
    }
    return {};
}

std::string AssetNames(const nlohmann::json &release)
{
    std::string names;
    if (!release.contains("assets") || !release["assets"].is_array()) {
        return "(none listed)";
    }
    for (const nlohmann::json &asset : release["assets"]) {
        const std::string name = detail::Text(asset, "name");
        if (name.empty()) {
            continue;
        }
        if (!names.empty()) {
            names += ", ";
        }
        names += name;
    }
    return names.empty() ? std::string("(none listed)") : names;
}

} // anonymous namespace

void Install(const std::string &name, const std::string &source,
             const std::string &fileName, ProgressCallback onProgress)
{
    const fs::path targetDir = detail::RunnerDir(source, name);
    const std::string qualified = detail::Qualified(source, name);

    // Never Releases(): an install works from the local list and spends no request.
    const nlohmann::json cache = Cached();
    const nlohmann::json *release = FindRelease(cache, source, name);
    if (release == nullptr) {
        const bool listed = cache.contains("releases") && cache["releases"].is_array();
        Logger().Error("Runners: no release named '" + qualified + "' in the cache");
        throw std::runtime_error(
            "the cache holds " + std::to_string(listed ? cache["releases"].size() : 0)
            + " releases\n  refresh the release list, then search for the name");
    }

    // A named file may be another build than the cached choice, and the cached hash
    // covers that choice only, so the hash is looked up for the file actually chosen.
    Asset tarball;
    Asset sha512;
    if (fileName.empty()) {
        tarball = { detail::Text(*release, "asset"), detail::Text(*release, "url") };
        sha512 = { detail::Text(*release, "sha512"), detail::Text(*release, "sha512_url") };
    } else {
        tarball = FindAsset(*release, fileName);
        sha512 = FindAsset(*release, runner_assets::Sha512NameFor(fileName));
        if (!tarball.Usable()) {
            Logger().Error("Runners: release " + qualified + " has no file named '"
                           + fileName + "'");
            throw std::runtime_error("this release offers: " + AssetNames(*release));
        }
    }

    if (!tarball.Usable()) {
        Logger().Error("Runners: no installable file for " + qualified);
        throw std::runtime_error(
            "this release offers: " + AssetNames(*release)
            + "\n  install with an explicit --file to override the choice");
    }
    if (!sha512.Usable()) {
        Logger().Error("Runners: no sha512sum for " + tarball.name + " in " + qualified
                       + "; cannot verify download integrity");
        throw std::runtime_error(
            "the release must ship " + runner_assets::Sha512NameFor(tarball.name)
            + " next to it; it offers: " + AssetNames(*release));
    }

    runner_install::SweepAbandoned(detail::RunnersDir());
    runner_install::Run({ targetDir, tarball.name, tarball.url, sha512.name, sha512.url },
                        std::move(onProgress));
}

void Remove(const std::string &name, const std::string &source)
{
    const std::string qualified = detail::Qualified(source, name);
    if (detail::IsExternal(source)) {
        Logger().Error("Runners: runner " + qualified
                       + " does not belong to this launcher");
        throw std::runtime_error(
            "it was found outside " + detail::RunnersDir().string()
            + ", which is the only tree this launcher removes from");
    }

    const fs::path runnerDir = detail::RunnerDir(source, name);
    std::error_code error;
    if (!fs::is_directory(runnerDir, error)) {
        Logger().Error("Runners: runner not installed: " + qualified);
        throw std::runtime_error(
            "no directory at " + runnerDir.string()
            + "\n  the list of installed runners names them as source/name");
    }

    // A symlink under runners/ could point anywhere; only what resolves inside goes.
    const fs::path canonical = fs::canonical(runnerDir, error);
    if (error) {
        Logger().Error("Runners: cannot resolve the path of " + qualified);
        throw std::runtime_error(runnerDir.string() + ": " + error.message());
    }
    const fs::path base = fs::canonical(detail::RunnersDir(), error);
    if (error) {
        Logger().Error("Runners: cannot resolve the runners dir");
        throw std::runtime_error(detail::RunnersDir().string() + ": " + error.message());
    }
    if (canonical.lexically_relative(base).native().rfind("..", 0) == 0) {
        Logger().Error("Runners: refusing to remove outside the runners dir: " + qualified);
        throw std::runtime_error(
            runnerDir.string() + " resolves to " + canonical.string()
            + ", which is not under " + base.string());
    }

    // An install removes the target and renames onto it; an unlocked remove could
    // delete the directory that install just produced.
    PathLock lock(runnerDir);
    if (!lock.Acquired()) {
        Logger().Error("Runners: runner " + qualified + " is being installed right now");
        throw std::runtime_error(
            "held at " + PathLock::LockPath(runnerDir).string()
            + "\n  retry once the install finishes");
    }

    fs::remove_all(runnerDir, error);
    if (error) {
        Logger().Error("Runners: failed to remove runner: " + qualified);
        throw std::runtime_error(runnerDir.string() + ": " + error.message());
    }

    Logger().Info("Runners: removed " + qualified);
}

} // namespace Runners
} // namespace rocklaunch
