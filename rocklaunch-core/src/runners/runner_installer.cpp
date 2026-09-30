#include "rocklaunch/core/runners/runner_installer.h"

#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/subprocess.h"
#include "rocklaunch/core/utils/checksum.h"
#include "rocklaunch/core/utils/downloader.h"
#include "rocklaunch/core/utils/file_lock.h"
#include "rocklaunch/core/utils/path_util.h"

#include <fstream>
#include <functional>
#include <optional>
#include <stdexcept>

namespace rocklaunch
{

namespace runner_install
{

namespace
{

constexpr const char *kScratchPrefix = ".tmp-download-";

fs::path ScratchPath(const fs::path &targetDir)
{
    return targetDir.parent_path()
        / (kScratchPrefix + RequirePathComponent(targetDir.filename().string()));
}

bool IsScratchDir(const std::string &name)
{
    return name.rfind(kScratchPrefix, 0) == 0;
}

Progress MakeStep(ProgressStage stage, const std::string &file, double percentage)
{
    Progress progress;
    progress.stage = stage;
    progress.file = file;
    progress.percentage = percentage;
    return progress;
}

// Steps that measure no bytes announce themselves too; the opening tick is where
// a cancel is honoured.
void BeginStep(const ProgressCallback &onProgress, ProgressStage stage,
               const std::string &file)
{
    if (onProgress && !onProgress(MakeStep(stage, file, 0.0))) {
        throw Cancelled(std::string(StageName(stage)) + " cancelled");
    }
}

void EndStep(const ProgressCallback &onProgress, ProgressStage stage,
             const std::string &file)
{
    if (onProgress) {
        onProgress(MakeStep(stage, file, 100.0));
    }
}

// An unset predicate is what keeps RunSubprocess blocking in waitpid: installing
// one that can only answer false would wake every 50 ms for nothing.
void RunExtract(const std::vector<std::string> &args, const ProgressCallback &onProgress,
                ProgressStage stage, const std::string &file)
{
    const Progress step = MakeStep(stage, file, 0.0);
    bool cancelRequested = false;
    std::function<bool()> isCancelled;
    if (onProgress) {
        isCancelled = [&onProgress, &step, &cancelRequested] {
            cancelRequested = !onProgress(step);
            return cancelRequested;
        };
    }

    const ExitInfo result = RunSubprocess(args, {}, {}, isCancelled);
    if (cancelRequested) {
        throw Cancelled(std::string(StageName(stage)) + " cancelled");
    }
    ThrowIfFailed(result, args);
}

// The hash is the first token of the first line.
std::string ParseSha512SumFile(const fs::path &path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger().Error("RunnerInstall: cannot open file: " + path.string());
        throw std::runtime_error(
            "the sha512sum was not downloaded; check the URL in the release list");
    }

    std::string line;
    if (std::getline(file, line) && !line.empty()) {
        std::size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            return line.substr(0, spacePos);
        }
        return line;
    }
    Logger().Error("RunnerInstall: cannot parse sha512sum file: " + path.string());
    throw std::runtime_error("it was empty; the release should ship a '<hash>  <name>' line");
}

// A release tarball holds exactly one top-level directory, the runner. Taking
// whichever came first would install one of several at random.
fs::path OnlyDirectory(const fs::path &parent)
{
    std::optional<fs::path> found;
    for (const fs::directory_entry &entry : fs::directory_iterator(parent)) {
        if (!entry.is_directory()) {
            continue;
        }
        if (found.has_value()) {
            Logger().Error("RunnerInstall: extraction produced more than one directory: "
                           + entry.path().string());
            throw std::runtime_error(
                "a runner tarball holds exactly one top-level directory, so this one is "
                "not a plain runner release");
        }
        found = entry.path();
    }
    if (!found.has_value()) {
        Logger().Error("RunnerInstall: extraction produced no directory in " + parent.string());
        throw std::runtime_error(
            "a runner tarball holds one top-level directory, but the archive unpacked "
            "into loose files");
    }
    return *found;
}

} // anonymous namespace

void SweepAbandoned(const fs::path &runnersDir)
{
    Logger logger;
    std::error_code iterationError;
    for (fs::directory_iterator it(runnersDir, iterationError), end;
         it != end; it.increment(iterationError)) {
        if (!fs::is_directory(it->path(), iterationError)) {
            continue;
        }

        const fs::path sourceDir = it->path();
        for (fs::directory_iterator scan(sourceDir, iterationError), sourceEnd;
             scan != sourceEnd; scan.increment(iterationError)) {
            if (!IsScratchDir(scan->path().filename().string())) {
                continue;
            }

            const std::string name = scan->path().filename().string();
            const fs::path target = sourceDir / name.substr(std::string(kScratchPrefix).size());

            // Held across the removal so a racing install keeps its fresh scratch
            // dir. The lock file is never unlinked: that would split the lock in two.
            PathLock probe(target);
            if (!probe.Acquired()) {
                continue;
            }

            logger.Debug("RunnerInstall: removing abandoned " + scan->path().string());
            fs::remove_all(scan->path(), iterationError);
            if (iterationError) {
                logger.Warn("RunnerInstall: cannot remove abandoned scratch "
                            + scan->path().string() + ": " + iterationError.message());
            }
        }
    }
}

void Run(const Request &request, ProgressCallback onProgress)
{
    Logger logger;
    const fs::path &targetDir = request.targetDir;
    const std::string name = targetDir.filename().string();

    // Both names come from the release API and become path components below: a separator
    // would place the download outside the scratch dir, and the hash check is downstream.
    if (!IsPathComponent(request.assetName)) {
        Logger().Error("RunnerInstall: release asset name is not a plain file name: '"
                       + request.assetName + "'");
        throw std::runtime_error(
            "a name with a path separator would unpack outside the scratch dir");
    }
    if (!IsPathComponent(request.sha512Name)) {
        Logger().Error("RunnerInstall: release hash name is not a plain file name: '"
                       + request.sha512Name + "'");
        throw std::runtime_error(
            "a name with a path separator would unpack outside the scratch dir");
    }

    // The install ends in remove_all + rename on the target, so two overlapping
    // installs of one runner would destroy each other.
    PathLock lock(targetDir);
    if (!lock.Acquired()) {
        logger.Warn("RunnerInstall: " + name + " is already being installed");
        throw std::runtime_error(
            "another process holds " + PathLock::LockPath(targetDir).string()
            + "\n  wait for it to finish, or remove the lock if no install is running");
    }

    // Beside the target so the final rename stays on one filesystem.
    const fs::path scratchDir = ScratchPath(targetDir);
    fs::remove_all(scratchDir);
    fs::create_directories(scratchDir);

    logger.Info("RunnerInstall: installing " + name);

    try {
        const fs::path tarballPath = scratchDir / request.assetName;
        const fs::path sha512Path = scratchDir / request.sha512Name;
        Downloader::Fetch(request.assetUrl, tarballPath, onProgress);
        Downloader::Fetch(request.sha512Url, sha512Path, onProgress);

        BeginStep(onProgress, ProgressStage::Verifying, request.assetName);
        const std::string expectedHash = ParseSha512SumFile(sha512Path);
        const std::string actualHash = HashFile(tarballPath);
        if (actualHash != expectedHash) {
            logger.Error("RunnerInstall: SHA-512 mismatch for " + request.assetName);
            throw std::runtime_error(
                "SHA-512 mismatch for " + request.assetName
                + "\n  expected: " + expectedHash
                + "\n  got:      " + actualHash
                + "\n  the download is corrupt or the release was rebuilt; nothing installed");
        }
        EndStep(onProgress, ProgressStage::Verifying, request.assetName);

        BeginStep(onProgress, ProgressStage::Extracting, request.assetName);
        const fs::path extractDir = scratchDir / "extracted";
        fs::create_directories(extractDir);
        RunExtract({ "tar", "-xf", tarballPath.string(), "-C", extractDir.string() },
                   onProgress, ProgressStage::Extracting, request.assetName);
        const fs::path extracted = OnlyDirectory(extractDir);
        EndStep(onProgress, ProgressStage::Extracting, request.assetName);

        BeginStep(onProgress, ProgressStage::Installing, name);
        fs::remove_all(targetDir);
        fs::rename(extracted, targetDir);
        fs::remove_all(scratchDir);
        EndStep(onProgress, ProgressStage::Installing, name);

        logger.Info("RunnerInstall: installed " + name + " to " + targetDir.string());
    } catch (...) {
        fs::remove_all(scratchDir);
        throw;
    }
}

} // namespace runner_install
} // namespace rocklaunch
