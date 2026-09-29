#pragma once

#include "rocklaunch/core/utils/downloader.h"

#include <optional>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace runner_assets
{

// The tarball of a release for the host architecture, nullopt when there is none or
// the choice is ambiguous.
std::optional<AssetInfo> SelectTarball(const std::vector<AssetInfo> &assets);

// The .sha512sum name paired with a tarball by the whole stem the repos share
// ("X-x86_64.tar.gz" and "X-x86_64.sha512sum"). Empty when it is not a tarball.
std::string Sha512NameFor(const std::string &tarballName);

// The asset named Sha512NameFor(tarballName), if the release ships it.
std::optional<AssetInfo> Sha512For(const std::vector<AssetInfo> &assets,
                                   const std::string &tarballName);

// The release's .sha512sum, disambiguated by host architecture and then by pairing
// with tarballName (skipped when empty). nullopt when there is no usable hash.
std::optional<AssetInfo> SelectSha512(const std::vector<AssetInfo> &assets,
                                      const std::string &tarballName = "");

} // namespace runner_assets
} // namespace rocklaunch
