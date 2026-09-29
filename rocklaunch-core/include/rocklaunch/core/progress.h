#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace rocklaunch
{

enum class ProgressStage
{
    Resolving,
    Downloading,
    Verifying,
    Extracting,
    Installing,
};

constexpr const char *StageName(ProgressStage stage)
{
    switch (stage) {
    case ProgressStage::Resolving:
        return "Resolving";
    case ProgressStage::Downloading:
        return "Downloading";
    case ProgressStage::Verifying:
        return "Verifying";
    case ProgressStage::Extracting:
        return "Extracting";
    case ProgressStage::Installing:
        return "Installing";
    }
    return "Working";
}

// totalBytes == 0 means the server sent no Content-Length.
struct Progress
{
    ProgressStage stage = ProgressStage::Downloading;
    std::string file;

    uint64_t bytesTransferred = 0;
    uint64_t totalBytes = 0;
    double percentage = 0.0;
    uint64_t bytesPerSecond = 0;
};

// Returning false cancels: the operation aborts and throws.
using ProgressCallback = std::function<bool(const Progress &)>;

} // namespace rocklaunch
