#pragma once

#include "rocklaunch/core/progress.h"

#include <filesystem>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

struct AssetInfo
{
    std::string name;
    std::string downloadUrl;
    uint64_t size;
};

struct ReleaseInfo
{
    std::string version;
    std::string tag;
    std::vector<AssetInfo> assets;
};

// HTTP download and GitHub release utilities, built on libcurl.
namespace Downloader
{

// Download url to destPath through destPath + ".tmp", renamed on success.
// onProgress is optional; returning false cancels.
// Throws std::runtime_error on HTTP error, network failure or cancellation.
void Fetch(const std::string &url,
           const fs::path &destPath,
           ProgressCallback onProgress = nullptr);

// List releases from a GitHub repo (e.g. "GloriousEggroll/proton-ge-custom").
// Returns up to count releases, newest first.
std::vector<ReleaseInfo> ListReleases(const std::string &repo,
                                      int count = 30);

} // namespace Downloader
} // namespace rocklaunch
