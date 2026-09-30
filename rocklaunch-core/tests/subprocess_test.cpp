// Core unit tests for the subprocess primitives.
//
// NO NETWORK, NO WINE: every case is a local binary, so the exit-code contract
// is checked without a runner or a game.
//
// ISOLATED: the test root is argv[1], and HOME / XDG_* point at it, exactly
// like the CLI test scripts.

#include "rocklaunch/core/subprocess.h"

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <csignal>
#include <unistd.h>

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

std::string Throws(const std::function<void()> &action)
{
    try {
        action();
    } catch (const std::exception &error) {
        return error.what();
    }
    return {};
}

std::string ReadFile(const fs::path &path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

void TestRunSubprocessNeverThrows()
{
    Check(rocklaunch::ExitInfo{}.started && rocklaunch::ExitInfo{}.reaped,
          "the default ExitInfo reads as a collected process, which FromStatus relies on");

    const rocklaunch::ExitInfo empty = rocklaunch::RunSubprocess({});
    Check(!empty.started, "empty args report a process that never started");
    Check(empty.code == 0 && !empty.signaled, "a process that never started carries no exit code");

    const rocklaunch::ExitInfo clean = rocklaunch::RunSubprocess({ "/bin/sh", "-c", "exit 0" });
    Check(clean.started && clean.code == 0 && !clean.signaled,
          "a child that exits 0 reports started with code 0");

    const rocklaunch::ExitInfo failed = rocklaunch::RunSubprocess({ "/bin/sh", "-c", "exit 3" });
    Check(failed.started && failed.code == 3 && !failed.signaled,
          "a non-zero exit is reported as a code, not a signal");

    // execvpe searching PATH is how every runner is launched, so a missing
    // binary has to land as 127 rather than started = false.
    const rocklaunch::ExitInfo missing = rocklaunch::RunSubprocess({ "rocklaunch-no-such-binary" });
    Check(missing.started && missing.code == 127,
          "a binary that cannot be exec'd is a 127 exit, not a spawn failure");
}

void TestSignalIsReported()
{
    const rocklaunch::ExitInfo killed = rocklaunch::RunSubprocess(
        { "/bin/sh", "-c", "kill -TERM $$; sleep 5" });
    Check(killed.signaled && killed.signal == SIGTERM, "a signalled child names the signal");
    Check(killed.code == 128 + SIGTERM, "a signalled child carries 128 + signal as its code");
}

void TestWorkDirAndEnvironment(const fs::path &testRoot)
{
    const fs::path root = testRoot / "cwd";
    fs::create_directories(root);

    const rocklaunch::ExitInfo exit = rocklaunch::RunSubprocess(
        { "/bin/sh", "-c", "pwd > where.txt" }, root, {}, nullptr);
    Check(exit.started && exit.code == 0, "a workDir runs the child there");

    std::string reported = ReadFile(root / "where.txt");
    while (!reported.empty() && std::isspace(static_cast<unsigned char>(reported.back()))) {
        reported.pop_back();
    }
    Check(!reported.empty() && fs::canonical(reported) == fs::canonical(root),
          "the child's working directory is the workDir, got: " + reported);

    const rocklaunch::ExitInfo env = rocklaunch::RunSubprocess(
        { "/bin/sh", "-c", "env > vars.txt" }, root,
        { "ROCKLAUNCH_SUBPROCESS_TEST=present" });
    Check(env.started && env.code == 0, "an environment override is accepted");

    const std::string environment = ReadFile(root / "vars.txt");
    Check(environment.find("ROCKLAUNCH_SUBPROCESS_TEST=present") != std::string::npos,
          "the child sees the override");
    Check(environment.find("\nPATH=") != std::string::npos,
          "the rest of the environment is inherited, not replaced");
}

void TestFailFastWrapper()
{
    Check(Throws([] { rocklaunch::RunSubprocessOrThrow({}); }).find("Command failed")
              != std::string::npos,
          "the wrapper reports a process that never started");

    const std::string failed = Throws([] {
        rocklaunch::RunSubprocessOrThrow({ "/bin/sh", "-c", "exit 4" });
    });
    Check(failed.find("Command failed (exit 4)") != std::string::npos,
          "a non-zero exit keeps the (reason) wording: " + failed);
    Check(failed.find("/bin/sh -c exit 4") != std::string::npos,
          "the message names the whole command: " + failed);

    const std::string killed = Throws([] {
        rocklaunch::RunSubprocessOrThrow({ "/bin/sh", "-c", "kill -TERM $$; sleep 5" });
    });
    Check(killed.find("Command failed (killed by signal " + std::to_string(SIGTERM) + ")")
              != std::string::npos,
          "a signalled child names the signal: " + killed);

    const std::string absent = Throws([] { rocklaunch::RunSubprocessOrThrow({}); });
    Check(absent.find("Command failed (did not start)") != std::string::npos,
          "a process that never started uses the same shape: " + absent);

    const rocklaunch::ExitInfo clean{ false, 0, 0, true };
    Check(rocklaunch::FailureReason(clean).empty(), "a clean exit has no failure reason");
    Check(rocklaunch::FailureReason({ false, 0, 0, false }) == "did not start",
          "a process that never started is distinguishable from a failing one");

    rocklaunch::ExitInfo uncollected;
    uncollected.reaped = false;
    Check(rocklaunch::FailureReason(uncollected) == "no exit status",
          "a child whose status was lost is not reported as one that never started");

    Check(rocklaunch::FailureReason(rocklaunch::RunSubprocess(
              { "/bin/sh", "-c", "exit 6" })) == "exit 6",
          "FailureReason agrees with what the wrapper reports");

    Check(Throws([] {
        rocklaunch::RunSubprocessOrThrow({ "/bin/sh", "-c", "exit 0" });
    }).empty(), "a clean exit does not throw");
}

void TestCancellationStopsTheChild()
{
    std::atomic<int> polls{ 0 };
    const auto isCancelled = [&polls] {
        // True only after a few polls, so the child is running when it flips.
        return polls.fetch_add(1) >= 2;
    };

    const rocklaunch::ExitInfo exit =
        rocklaunch::RunSubprocess({ "/bin/sh", "-c", "exec sleep 30" }, {}, {}, isCancelled);
    Check(exit.signaled, "a cancelled child is reported as signalled: signal "
                             + std::to_string(exit.signal));
    Check(exit.code == 128 + SIGTERM, "cancellation escalates to SIGTERM first");
    Check(polls.load() >= 3, "the predicate is polled while the child runs, not once at the end");

    const rocklaunch::ExitInfo uncancelled =
        rocklaunch::RunSubprocess({ "/bin/sh", "-c", "exit 7" }, {}, {}, [] { return false; });
    Check(uncancelled.started && uncancelled.code == 7 && !uncancelled.signaled,
          "an untriggered predicate leaves the child's own exit status untouched");
}

// SA_RESTART is off on purpose: without the retry, a signal landing mid-wait
// makes waitpid report a spawn failure and abandons a live child.
void TestSignalDoesNotLookLikeAFailure()
{
    struct sigaction interrupt {};
    interrupt.sa_handler = [](int) {};
    sigemptyset(&interrupt.sa_mask);
    interrupt.sa_flags = 0;
    sigaction(SIGALRM, &interrupt, nullptr);

    const rocklaunch::ExitInfo exit = [&] {
        ualarm(50000, 0);
        const rocklaunch::ExitInfo result =
            rocklaunch::RunSubprocess({ "/bin/sh", "-c", "exec sleep 1" });
        ualarm(0, 0);
        return result;
    }();

    Check(exit.started, "an interrupted wait still reports the child as started");
    Check(!exit.signaled && exit.code == 0,
          "the child was reaped, not orphaned: signal " + std::to_string(exit.signal)
              + " code " + std::to_string(exit.code));
}

} // namespace

int main(int argc, char *argv[])
{
    const fs::path testRoot = argc > 1
        ? fs::path(argv[1])
        : fs::path("/tmp/rocklaunch-subprocess-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);

    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);

    TestRunSubprocessNeverThrows();
    TestSignalIsReported();
    TestWorkDirAndEnvironment(testRoot);
    TestFailFastWrapper();
    TestCancellationStopsTheChild();
    TestSignalDoesNotLookLikeAFailure();

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
