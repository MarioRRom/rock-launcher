#pragma once

#include "rocklaunch/core/progress.h"
#include "rocklaunch/core/runners/runners.h"

#include <string>

namespace rocklaunch
{

// Unpacking a runner release into the runners dir, from URLs the cache holds.
namespace runner_install
{

struct Request
{
    fs::path targetDir;
    std::string assetName;
    std::string assetUrl;
    std::string sha512Name;
    std::string sha512Url;
};

// Removes scratch dirs of installs killed before they could unwind. A dir whose
// runner lock can be taken has no live owner.
void SweepAbandoned(const fs::path &runnersDir);

// Downloads both assets, verifies the tarball, and swaps the extracted directory
// into place. A second install of the same runner bounces on the lock.
void Run(const Request &request, ProgressCallback onProgress);

} // namespace runner_install
} // namespace rocklaunch
