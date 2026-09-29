// Core unit tests for the path lock and the path-component sanitizer.
//
// NO NETWORK, NO WINE: both are pure filesystem concerns, and the lock is
// POSIX-only, so the whole file is synchronous and single-process except for
// the one test that needs a second process.
//
// ISOLATED: the test root is argv[1], and HOME / XDG_* point at it, exactly
// like the CLI test scripts.

#include "rocklaunch/core/utils/file_lock.h"
#include "rocklaunch/core/utils/path_util.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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

// Two locks on the same resource cannot coexist, which is what makes a second
// install bounce instead of racing the first.
void TestSecondAcquireFails(const fs::path &root)
{
    const fs::path resource = root / "bounce";

    rocklaunch::PathLock first(resource);
    Check(first.Acquired(), "the first lock on a free resource is acquired");

    // Contention is not an error: the lock is simply left unheld, so reaching
    // this line at all is part of what is being asserted.
    rocklaunch::PathLock second(resource);
    Check(!second.Acquired(), "a second lock on the same resource is refused");
}

// Ten different runners, ten locks, none of them waiting on each other.
void TestDifferentResourcesDoNotCollide(const fs::path &root)
{
    std::vector<std::unique_ptr<rocklaunch::PathLock>> locks;
    for (int i = 0; i < 10; ++i) {
        const fs::path resource = root / ("runner-" + std::to_string(i));
        locks.push_back(std::make_unique<rocklaunch::PathLock>(resource));
    }

    bool allHeld = true;
    for (const auto &lock : locks) {
        allHeld = allHeld && lock->Acquired();
    }
    Check(allHeld, "locks on different resources all hold at once");
}

// The property the whole design rests on: a lock is owned by the kernel, so a
// dead holder cannot keep it. Forks a child that takes the lock and exits
// without unwinding, standing in for an install killed by a signal.
void TestLockSurvivesHolderDeath(const fs::path &root)
{
    const fs::path resource = root / "orphan";

    const pid_t child = fork();
    if (child == 0) {
        rocklaunch::PathLock held(resource);
        // _exit so no destructor runs: the kernel has to do the releasing.
        ::_exit(held.Acquired() ? 0 : 1);
    }

    int status = 0;
    Check(waitpid(child, &status, 0) == child, "the child holder is reaped");
    Check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the child took the lock before exiting");

    rocklaunch::PathLock afterDeath(resource);
    Check(afterDeath.Acquired(),
          "the lock is free once its holder's process is gone");
}

// Releasing on scope exit is what lets a later install of the same tag through.
void TestReleaseOnDestruction(const fs::path &root)
{
    const fs::path resource = root / "released";

    {
        rocklaunch::PathLock held(resource);
        Check(held.Acquired(), "the lock is held inside its scope");
    }

    rocklaunch::PathLock reacquired(resource);
    Check(reacquired.Acquired(), "the lock is free once the holder is destroyed");
}

void TestLockPathLayout(const fs::path &root)
{
    const fs::path resource = root / "sub" / "GE-Proton11-7";
    const fs::path expected = root / "sub" / ".locks" / "GE-Proton11-7.lock";
    Check(rocklaunch::PathLock::LockPath(resource) == expected,
          "the lock file sits in .locks beside the resource");
    Check(rocklaunch::PathLock::LockPath("bare") == fs::path(".locks") / "bare.lock",
          "a resource with no directory still yields a usable lock path");
}

void TestRequirePathComponent()
{
    using rocklaunch::RequirePathComponent;

    Check(RequirePathComponent("GE-Proton11-7") == "GE-Proton11-7",
          "a real runner tag passes through unchanged");

    for (const std::string name : {"", ".", "..", "../../etc", "/etc/passwd", "a/b", ".hidden"}) {
        bool refused = false;
        try {
            RequirePathComponent(name);
        } catch (const std::runtime_error &) {
            refused = true;
        }
        Check(refused, "'" + name + "' is refused");
    }
}

} // anonymous namespace

int main(int argc, char **argv)
{
    const fs::path testRoot = argc > 1
        ? fs::path(argv[1])
        : fs::path("/tmp/rocklaunch-file-lock-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);

    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);

    const fs::path root = testRoot / "data";
    fs::create_directories(root, error);

    TestSecondAcquireFails(root);
    TestDifferentResourcesDoNotCollide(root);
    TestLockSurvivesHolderDeath(root);
    TestReleaseOnDestruction(root);
    TestLockPathLayout(root);
    TestRequirePathComponent();

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
