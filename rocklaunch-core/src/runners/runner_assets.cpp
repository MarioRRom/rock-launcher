#include "rocklaunch/core/runners/runner_assets.h"

#include "rocklaunch/core/utils/string_util.h"

#include <sys/utsname.h>
#include <vector>

namespace rocklaunch
{

namespace runner_assets
{

namespace
{

// In order of preference when a build ships in several compression formats.
const std::vector<std::string> kTarballExtensions = {
    ".tar.gz", ".tar.xz", ".tar.bz2", ".tar.zst", ".tgz",
};

std::string DetectHostArch()
{
    struct utsname uts;
    if (::uname(&uts) == 0) {
        return uts.machine;
    }
    return "x86_64";
}

// Position in kTarballExtensions, or its size when the name is not a tarball.
std::size_t TarballRank(const std::string &nameLower)
{
    for (std::size_t rank = 0; rank < kTarballExtensions.size(); ++rank) {
        if (EndsWith(nameLower, kTarballExtensions[rank])) {
            return rank;
        }
    }
    return kTarballExtensions.size();
}

// Whole-token arch match: "x86_64" must not match "x86_64_v3".
bool MatchesArch(const std::string &nameLower, const std::string &archLower)
{
    std::size_t pos = nameLower.find(archLower);
    while (pos != std::string::npos) {
        std::size_t after = pos + archLower.size();
        bool boundary = after >= nameLower.size()
            || !(std::isalnum(static_cast<unsigned char>(nameLower[after]))
                 || nameLower[after] == '_');
        if (boundary) {
            return true;
        }
        pos = nameLower.find(archLower, pos + 1);
    }
    return false;
}

// True when the name explicitly targets a different architecture.
bool IsForOtherArch(const std::string &nameLower, const std::string &hostArch)
{
    static const std::vector<std::string> kKnownArches = {
        "x86_64", "aarch64", "arm64", "amd64", "i386", "i686", "armv7l", "armv8l",
    };
    for (const std::string &arch : kKnownArches) {
        if (arch != hostArch && MatchesArch(nameLower, arch)) {
            return true;
        }
    }
    return false;
}

template <typename Predicate>
std::vector<AssetInfo> Filter(const std::vector<AssetInfo> &assets, Predicate keep)
{
    std::vector<AssetInfo> kept;
    for (const AssetInfo &asset : assets) {
        if (keep(ToLower(asset.name))) {
            kept.push_back(asset);
        }
    }
    return kept;
}

bool IsTarballName(const std::string &nameLower)
{
    return TarballRank(nameLower) < kTarballExtensions.size()
        && nameLower.find("sha512") == std::string::npos;
}

} // anonymous namespace

std::optional<AssetInfo> SelectTarball(const std::vector<AssetInfo> &assets)
{
    std::vector<AssetInfo> candidates = Filter(assets, IsTarballName);
    if (candidates.size() == 1) {
        return candidates[0];
    }

    const std::string hostArch = ToLower(DetectHostArch());

    std::vector<AssetInfo> matched = Filter(assets, [&](const std::string &name) {
        return IsTarballName(name) && MatchesArch(name, hostArch);
    });
    if (matched.size() == 1) {
        return matched[0];
    }

    // No arch token matched: drop other architectures, then prefer by format.
    std::vector<AssetInfo> filtered = Filter(assets, [&](const std::string &name) {
        return IsTarballName(name) && !IsForOtherArch(name, hostArch);
    });
    if (filtered.size() == 1) {
        return filtered[0];
    }
    if (filtered.size() > 1) {
        std::stable_sort(filtered.begin(), filtered.end(),
                         [](const AssetInfo &left, const AssetInfo &right) {
                             return TarballRank(ToLower(left.name))
                                 < TarballRank(ToLower(right.name));
                         });
        return filtered[0];
    }

    return std::nullopt;
}

std::string Sha512NameFor(const std::string &tarballName)
{
    const std::string lower = ToLower(tarballName);
    for (const std::string &extension : kTarballExtensions) {
        if (EndsWith(lower, extension)) {
            return tarballName.substr(0, tarballName.size() - extension.size())
                + ".sha512sum";
        }
    }
    return "";
}

std::optional<AssetInfo> Sha512For(const std::vector<AssetInfo> &assets,
                                   const std::string &tarballName)
{
    const std::string wanted = ToLower(Sha512NameFor(tarballName));
    if (wanted.empty()) {
        return std::nullopt;
    }

    // Case-insensitive: the repos are not consistent about asset-name case.
    const std::vector<AssetInfo> paired = Filter(assets, [&](const std::string &name) {
        return name == wanted;
    });
    return paired.size() == 1 ? std::optional<AssetInfo>(paired[0]) : std::nullopt;
}

std::optional<AssetInfo> SelectSha512(const std::vector<AssetInfo> &assets,
                                      const std::string &tarballName)
{
    auto isSha512 = [](const std::string &nameLower) {
        return EndsWith(nameLower, ".sha512sum");
    };

    std::vector<AssetInfo> candidates = Filter(assets, isSha512);
    if (candidates.size() == 1) {
        return candidates[0];
    }
    if (candidates.size() > 1) {
        const std::string hostArch = ToLower(DetectHostArch());
        std::vector<AssetInfo> matched = Filter(assets, [&](const std::string &name) {
            return isSha512(name) && MatchesArch(name, hostArch);
        });
        if (matched.size() == 1) {
            return matched[0];
        }

        // None for this arch: fall back to the hash covering the tarball.
        const std::optional<AssetInfo> paired = Sha512For(assets, tarballName);
        if (paired.has_value()) {
            return paired;
        }
    }

    return std::nullopt;
}

} // namespace runner_assets
} // namespace rocklaunch
