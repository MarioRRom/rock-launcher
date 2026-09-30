// Core unit tests for the atomic text write. WriteFileAtomic has no CLI surface.
//
// NO NETWORK, NO WINE: the whole file is filesystem concerns.
//
// ISOLATED: the test root is argv[1], and HOME / XDG_* point at it, exactly
// like the CLI test scripts.

#include "rocklaunch/core/utils/atomic_file.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
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

void CheckEq(const std::string &actual, const std::string &expected, const std::string &what)
{
    ++gChecks;
    if (actual != expected) {
        ++gFailures;
        std::cerr << "FAIL: " << what << "\n  expected: " << expected << "\n  actual:   "
                  << actual << '\n';
    }
}

std::string What(const std::function<void()> &body)
{
    try {
        body();
    } catch (const std::exception &error) {
        return error.what();
    }
    return {};
}

void TestWritesTheTextAndLeavesNoTemp(const fs::path &root)
{
    const fs::path dest = root / "written.txt";
    rocklaunch::WriteFileAtomic(dest, "line one\nline two\n", "T: ok", "nothing was lost");

    std::ifstream input(dest);
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    CheckEq(contents, "line one\nline two\n", "the text arrives intact");
    Check(!fs::exists(dest.string() + ".tmp"), "no temp file survives a successful write");
}

void TestOverwritesAnExistingFile(const fs::path &root)
{
    const fs::path dest = root / "replaced.txt";
    std::ofstream(dest) << "old contents that are longer\n";

    rocklaunch::WriteFileAtomic(dest, "new\n", "T: replace", "nothing was lost");
    std::ifstream input(dest);
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    CheckEq(contents, "new\n", "the rename replaces the old file wholesale");
}

void TestEmptyTextIsStillAFile(const fs::path &root)
{
    const fs::path dest = root / "empty.txt";
    rocklaunch::WriteFileAtomic(dest, "", "T: empty", "nothing was lost");
    Check(fs::is_regular_file(dest) && fs::file_size(dest) == 0, "an empty write produces an empty file");
}

// A directory where the temp file goes, so the ofstream cannot open it.
void TestUnopenableTempReportsTheConsequence(const fs::path &root)
{
    const fs::path dest = root / "blocked.txt";
    fs::create_directories(dest.string() + ".tmp");

    const std::string message = What([&] {
        rocklaunch::WriteFileAtomic(dest, "text", "T: blocked", "the note was not saved");
    });

    CheckEq(message,
            "the note was not saved: cannot write " + dest.string()
                + "\n  check free space and permissions on " + root.string(),
            "an unopenable temp leads with what the user lost, then the reason");
}

// A non-empty directory as the destination, so the rename cannot succeed. The
// errno text is system-dependent, so only the fixed part is asserted.
void TestFailedRenameRemovesTheTemp(const fs::path &root)
{
    const fs::path dest = root / "occupied.txt";
    fs::create_directories(dest);
    std::ofstream(dest / "occupant") << "makes the directory non-empty\n";

    const std::string message = What([&] {
        rocklaunch::WriteFileAtomic(dest, "text", "T: occupied", "the note was not lost");
    });

    Check(message.rfind("the note was not lost: cannot replace " + dest.string() + "\n  ", 0) == 0,
          "a failed rename leads with the consequence and names the destination");
    Check(!fs::exists(dest.string() + ".tmp"), "a failed rename does not leave the temp behind");
    Check(fs::is_directory(dest) && fs::is_regular_file(dest / "occupant"),
          "a failed rename leaves the destination untouched");
}

} // namespace

int main(int argc, char **argv)
{
    const fs::path testRoot = argc > 1
        ? fs::path(argv[1])
        : fs::path("/tmp/rocklaunch-atomic-file-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);

    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);

    TestWritesTheTextAndLeavesNoTemp(testRoot);
    TestOverwritesAnExistingFile(testRoot);
    TestEmptyTextIsStillAFile(testRoot);
    TestUnopenableTempReportsTheConsequence(testRoot);
    TestFailedRenameRemovesTheTemp(testRoot);

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
