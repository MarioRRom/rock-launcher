#include "rocklaunch/core/subprocess.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <stdexcept>
#include <thread>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace rocklaunch
{

namespace
{

constexpr auto kCancelPoll = std::chrono::milliseconds{ 50 };
constexpr auto kTerminateGrace = std::chrono::seconds{ 5 };

std::string Describe(const std::vector<std::string> &args)
{
    std::string command;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            command += " ";
        }
        command += args[i];
    }
    return command;
}

std::vector<char *> BuildArgv(const std::vector<std::string> &args)
{
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (const std::string &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

bool IsVariable(const std::string &entry, const std::string &key)
{
    return entry.size() > key.size() && entry.compare(0, key.size(), key) == 0
           && entry[key.size()] == '=';
}

// The strings own the bytes the pointers name, so the two have to share a
// lifetime: a vector<char*> returned on its own points into a destroyed local.
struct Environment
{
    std::vector<std::string> entries;
    std::vector<char *> pointers;

    explicit Environment(const std::vector<std::string> &overrides)
    {
        for (char **entry = environ; entry != nullptr && *entry != nullptr; ++entry) {
            entries.emplace_back(*entry);
        }

        for (const std::string &override : overrides) {
            const std::size_t separator = override.find('=');
            if (separator == std::string::npos) {
                continue;
            }
            const std::string key = override.substr(0, separator);
            const auto existing = std::find_if(entries.begin(), entries.end(),
                                               [&key](const std::string &entry) {
                                                   return IsVariable(entry, key);
                                               });
            if (existing != entries.end()) {
                *existing = override;
            } else {
                entries.push_back(override);
            }
        }

        pointers.reserve(entries.size() + 1);
        for (std::string &entry : entries) {
            pointers.push_back(entry.data());
        }
        pointers.push_back(nullptr);
    }
};

ExitInfo DidNotStart()
{
    ExitInfo result;
    result.started = false;
    result.reaped = false;
    return result;
}

ExitInfo NoStatus()
{
    ExitInfo result;
    result.reaped = false;
    return result;
}

ExitInfo FromStatus(int status)
{
    ExitInfo result;
    if (WIFEXITED(status)) {
        result.code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.signaled = true;
        result.signal = WTERMSIG(status);
        result.code = 128 + result.signal;
    }
    return result;
}

// A signal that lands mid-wait reports EINTR with the child still running, so
// treating that as a failure would abandon a live process nobody can reap.
pid_t Wait(pid_t pid, int &status, int flags)
{
    pid_t reaped;
    do {
        reaped = waitpid(pid, &status, flags);
    } while (reaped < 0 && errno == EINTR);
    return reaped;
}

ExitInfo WaitFor(pid_t pid, const std::function<bool()> &isCancelled)
{
    int status = 0;

    if (!isCancelled) {
        if (Wait(pid, status, 0) < 0) {
            return NoStatus();
        }
        return FromStatus(status);
    }

    bool terminating = false;
    bool killed = false;
    auto terminateSent = std::chrono::steady_clock::now();

    for (;;) {
        const pid_t reaped = Wait(pid, status, WNOHANG);
        if (reaped < 0) {
            return NoStatus();
        }
        if (reaped == pid) {
            return FromStatus(status);
        }

        bool cancel = false;
        try {
            cancel = isCancelled();
        } catch (...) {
            kill(pid, SIGKILL);
            int ignored = 0;
            Wait(pid, ignored, 0);
            throw;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!terminating && cancel) {
            kill(pid, SIGTERM);
            terminating = true;
            terminateSent = now;
        } else if (terminating && !killed && now - terminateSent >= kTerminateGrace) {
            kill(pid, SIGKILL);
            killed = true;
        }
        std::this_thread::sleep_for(kCancelPoll);
    }
}

} // namespace

ExitInfo RunSubprocess(const std::vector<std::string> &args,
                       const fs::path &workDir,
                       const std::vector<std::string> &environment,
                       const std::function<bool()> &isCancelled)
{
    if (args.empty()) {
        return DidNotStart();
    }

    // Built before fork: the child may only call async-signal-safe functions
    // until exec, and both of these allocate.
    std::vector<char *> argv = BuildArgv(args);
    const Environment env(environment);

    const pid_t pid = fork();
    if (pid < 0) {
        return DidNotStart();
    }

    if (pid == 0) {
        if (!workDir.empty() && chdir(workDir.c_str()) != 0) {
            _exit(127);
        }
        // Dispositions survive exec, so a parent that ignores SIGPIPE hands the
        // child EPIPE returns where it expects death by signal.
        signal(SIGPIPE, SIG_DFL);
        execvpe(argv[0], argv.data(), env.pointers.data());
        _exit(127);
    }

    return WaitFor(pid, isCancelled);
}

std::string FailureReason(const ExitInfo &exit)
{
    if (!exit.started) {
        return "did not start";
    }
    if (!exit.reaped) {
        return "no exit status";
    }
    if (exit.signaled) {
        return "killed by signal " + std::to_string(exit.signal);
    }
    if (exit.code != 0) {
        return "exit " + std::to_string(exit.code);
    }
    return {};
}

void ThrowIfFailed(const ExitInfo &exit, const std::vector<std::string> &args)
{
    const std::string reason = FailureReason(exit);
    if (!reason.empty()) {
        throw std::runtime_error("Command failed (" + reason + "): " + Describe(args));
    }
}

void RunSubprocessOrThrow(const std::vector<std::string> &args,
                          const fs::path &workDir,
                          const std::vector<std::string> &environment)
{
    ThrowIfFailed(RunSubprocess(args, workDir, environment), args);
}

} // namespace rocklaunch
