#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace rocklaunch
{

namespace fs = std::filesystem;

struct ExitInfo
{
    bool signaled = false;
    int code = 0;         // WEXITSTATUS, or 128 + signal when signaled
    int signal = 0;       // terminating signal number when signaled
    bool started = true;  // false = empty args or fork failed
    bool reaped = true;   // false = no status collected, so code and signal say nothing
};

// Never throws. environment overrides the inherited environment rather than
// replacing it; a true isCancelled sends SIGTERM, then SIGKILL after 5 s.
//
// Only the direct child is signalled, so a command that forks — an `sh -c`
// wrapper — leaves its own children running when cancelled. A predicate that
// throws kills the child before the exception propagates.
[[nodiscard]] ExitInfo RunSubprocess(const std::vector<std::string> &args,
                                     const fs::path &workDir = {},
                                     const std::vector<std::string> &environment = {},
                                     const std::function<bool()> &isCancelled = {});

// The message names the command and is printed verbatim by the CLI.
void RunSubprocessOrThrow(const std::vector<std::string> &args,
                          const fs::path &workDir = {},
                          const std::vector<std::string> &environment = {});

// The report RunSubprocessOrThrow produces, for a caller holding the ExitInfo.
void ThrowIfFailed(const ExitInfo &exit, const std::vector<std::string> &args);

// "" when the child ran and exited 0, otherwise "did not start",
// "killed by signal N" or "exit N" — for a caller that adds its own context.
std::string FailureReason(const ExitInfo &exit);

} // namespace rocklaunch
