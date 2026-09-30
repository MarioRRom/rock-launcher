// Core unit tests for the file-backed logger. Logger has no CLI surface.
//
// NO NETWORK, NO WINE: the whole file is filesystem concerns.
//
// ISOLATED: the test root is argv[1], and HOME / XDG_* point at it, exactly
// like the CLI test scripts.

#include "rocklaunch/core/logger.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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

std::string ReadAll(const fs::path &path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void TestWritesTheMessage(const fs::path &root)
{
    const rocklaunch::Logger logger(root / "ok");
    logger.Info("an info line");
    logger.Warn("a warn line");
    logger.Error("an error line");

    const std::string contents = ReadAll(logger.LogFile());
    Check(contents.find("an info line") != std::string::npos, "an info line reaches the logfile");
    Check(contents.find("a warn line") != std::string::npos, "a warn line reaches the logfile");
    Check(contents.find("an error line") != std::string::npos, "an error line reaches the logfile");
    Check(contents.find("[INFO]") != std::string::npos, "the level is recorded next to the message");

    logger.Debug("a debug line");
    Check(ReadAll(logger.LogFile()).find("a debug line") == std::string::npos,
          "a debug line is never persisted");
}

// The logfile path is a directory, so the ofstream append cannot open it.
// create_directories on the parent still succeeds, isolating the write path.
void TestUnwritableLogfileDoesNotThrow(const fs::path &root)
{
    const fs::path logDir = root / "blocked";
    fs::create_directories(logDir / "rocklaunch.log");

    const rocklaunch::Logger logger(logDir);
    Check(!Throws([&] { logger.Info("info into a blocked logfile"); }), "info survives a blocked logfile");
    Check(!Throws([&] { logger.Warn("warn into a blocked logfile"); }), "warn survives a blocked logfile");
    Check(!Throws([&] { logger.Error("error into a blocked logfile"); }), "error survives a blocked logfile");
    Check(!Throws([&] { logger.Debug("debug into a blocked logfile"); }), "debug survives a blocked logfile");
    Check(!fs::is_regular_file(logger.LogFile()), "nothing was created over the directory in the way");
}

// A regular file where the log dir should be, so create_directories fails. The
// constructor runs on the first log call of every component, so it must survive.
void TestUncreatableLogDirDoesNotThrow(const fs::path &root)
{
    const fs::path blocker = root / "not-a-dir";
    std::ofstream(blocker) << "a regular file\n";

    Check(!Throws([&] { const rocklaunch::Logger logger(blocker / "logs"); }),
          "the constructor survives a log dir it cannot create");
}

// The install path: the runner is on disk and the success line is logged from
// inside the try that reports failures.
void TestTheSuccessPathSurvivesABrokenLogfile(const fs::path &root)
{
    const fs::path logDir = root / "blocked-success";
    fs::create_directories(logDir / "rocklaunch.log");

    bool installed = false;
    const fs::path target = root / "installed-runner";
    fs::create_directories(target);

    try {
        const rocklaunch::Logger logger(logDir);
        logger.Info("RunnerInstall: installed something to " + target.string());
        installed = true;
    } catch (const std::exception &) {
        // The caller sees this as a failed install with the runner in place.
    }

    Check(installed, "logging a completed install does not report it as failed");
    Check(fs::is_directory(target), "the installed runner is still there");
}

} // namespace

int main(int argc, char **argv)
{
    const fs::path testRoot = argc > 1
        ? fs::path(argv[1])
        : fs::path("/tmp/rocklaunch-logger-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);

    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);

    TestWritesTheMessage(testRoot);
    TestUnwritableLogfileDoesNotThrow(testRoot);
    TestUncreatableLogDirDoesNotThrow(testRoot);
    TestTheSuccessPathSurvivesABrokenLogfile(testRoot);

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
