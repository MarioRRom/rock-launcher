#include "rocklaunch/core/launch.h"

#include "rocklaunch/core/launch_context.h"
#include "rocklaunch/core/logger.h"

#include <cstdlib>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

namespace rocklaunch
{

namespace
{

int RunProcess(const std::vector<std::string> &command, const std::vector<std::string> &environment)
{
    pid_t child = fork();
    if (child == -1) {
        return -1;
    }

    if (child == 0) {
        for (const std::string &variable : environment) {
            std::size_t separator = variable.find('=');
            if (separator != std::string::npos) {
                setenv(variable.substr(0, separator).c_str(),
                       variable.substr(separator + 1).c_str(), 1);
            }
        }

        std::vector<char *> argv;
        argv.reserve(command.size() + 1);
        for (const std::string &argument : command) {
            argv.push_back(const_cast<char *>(argument.c_str()));
        }
        argv.push_back(nullptr);

        execvp(command.front().c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    if (waitpid(child, &status, 0) == -1) {
        return -1;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// Proton builds ship binaries under files/ (GE-Proton) or dist/ (stock Steam
// Proton). Returns the first existing candidate for binary, or the dist/
// fallback path.
fs::path ProtonBinaryFor(const Runner &runner, const char *binary)
{
    for (const fs::path &layout : { "files", "dist" }) {
        fs::path candidate = runner.rootDir / layout / "bin" / binary;
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) {
            return candidate;
        }
    }

    return runner.rootDir / "dist" / "bin" / binary;
}

// The wine binary used to configure a prefix. GE-Proton ships under files/;
// stock Steam Proton uses dist/.
fs::path WineBinaryFor(const Runner &runner)
{
    if (runner.type == RunnerType::Wine) {
        return runner.executable;
    }

    return ProtonBinaryFor(runner, "wine");
}

// The directory a runner actually uses as WINEPREFIX inside prefixDir. The
// proton script uses <prefixDir>/pfx; Wine runners use the directory directly.
fs::path WinePrefixFor(const fs::path &prefixDir, const Runner &runner)
{
    return runner.type == RunnerType::Proton ? prefixDir / "pfx" : prefixDir;
}

} // namespace

std::vector<std::string> EnsurePrefix(const fs::path &prefixDir, const Runner &runner)
{
    Logger logger;
    std::vector<std::string> warnings;
    std::error_code error;

    // The proton script uses <prefixDir>/pfx as the real WINEPREFIX; prefixDir
    // itself is only the STEAM_COMPAT_DATA_PATH container. Wine runners use the
    // directory directly.
    fs::path winePrefix = WinePrefixFor(prefixDir, runner);

    fs::create_directories(winePrefix, error);
    if (error) {
        logger.Error("Launch: unable to create prefix at " + winePrefix.string());
        throw std::runtime_error("Unable to create prefix");
    }

    std::vector<LaunchCommand> commands = BuildPrefixCommands(prefixDir, runner);
    if (commands.empty()) {
        fs::path wine = WineBinaryFor(runner);
        logger.Warn("Launch: wine binary not found at " + wine.string());
        warnings.emplace_back("Audio=alsa skipped — wine not found");
        return warnings;
    }

    for (const LaunchCommand &command : commands) {
        int result = RunProcess(command.command, command.environment);
        if (result != 0) {
            logger.Warn("Launch: Audio=alsa reg add failed (exit "
                        + std::to_string(result) + ")");
            warnings.emplace_back("Audio=alsa failed (exit "
                                  + std::to_string(result) + ")");
            break;
        }
    }

    return warnings;
}

std::vector<LaunchCommand> BuildPrefixCommands(const fs::path &prefixDir, const Runner &runner)
{
    fs::path wine = WineBinaryFor(runner);
    std::error_code error;
    if (!fs::is_regular_file(wine, error)) {
        return {};
    }

    // Audio=alsa: makes the game enumerate audio devices via ALSA.
    // Written to both 64-bit and 32-bit registry views for the 32-bit game.
    const std::vector<std::string> keys = { "HKCU\\Software\\Wine\\Drivers",
                                            "HKCU\\Software\\Wow6432Node\\Wine\\Drivers" };
    std::vector<LaunchCommand> commands;
    commands.reserve(keys.size());
    for (const std::string &key : keys) {
        LaunchCommand command;
        command.command = { wine.string(), "reg", "add", key,
                            "/v", "Audio", "/d", "alsa", "/f" };
        command.environment = { "WINEPREFIX=" + WinePrefixFor(prefixDir, runner).string() };
        commands.push_back(std::move(command));
    }
    return commands;
}

std::optional<LaunchCommand> BuildWineKillCommand(const fs::path &prefixDir, const Runner &runner)
{
    // wineserver sits next to wine for Wine runners; Proton builds ship it under
    // files/ or dist/, the same layouts WineBinaryFor() probes.
    std::vector<fs::path> candidates;
    if (runner.type == RunnerType::Wine) {
        if (!runner.executable.empty()) {
            candidates.push_back(runner.executable.parent_path() / "wineserver");
        }
    } else {
        fs::path candidate = ProtonBinaryFor(runner, "wineserver");
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) {
            candidates.push_back(candidate);
        }
    }

    for (const fs::path &candidate : candidates) {
        std::error_code error;
        if (!fs::is_regular_file(candidate, error)) {
            continue;
        }

        LaunchCommand command;
        command.command = { candidate.string(), "-k" };
        command.environment = { "WINEPREFIX=" + WinePrefixFor(prefixDir, runner).string() };
        return command;
    }

    return std::nullopt;
}

LaunchCommand BuildLaunchCommand(const ProfileConfig &profile,
                                 const Runner &runner,
                                 const IGameProfile &game)
{
    LaunchCommand launch;
    fs::path executable = game.Executable(profile.installDir);
    if (runner.type == RunnerType::Proton) {
        launch.command = { runner.executable.string(), "run", executable.string() };
    } else {
        launch.command = { runner.executable.string(), executable.string() };
    }

    LaunchContext context;
    context.installDir = profile.installDir;
    context.prefixDir = profile.prefixDir;
    context.runnerId = runner.id;

    // Games expect to be launched from their install directory; Rocksmith reads
    // and writes Rocksmith.ini relative to the working directory.
    launch.workingDirectory = profile.installDir;

    if (runner.type == RunnerType::Wine) {
        launch.environment.emplace_back("WINEPREFIX=" + profile.prefixDir.string());
    } else {
        // Proton runs the game inside the prefix; point it at a real directory even without Steam.
        launch.environment.emplace_back("STEAM_COMPAT_DATA_PATH=" + profile.prefixDir.string());
        launch.environment.emplace_back("STEAM_COMPAT_CLIENT_INSTALL_PATH="
                                        + profile.prefixDir.parent_path().string());
    }
    // Universal audio buffer adjustment; harmless when PipeWire is not in use.
    launch.environment.emplace_back("PIPEWIRE_LATENCY=256/48000");

    for (const std::string &variable : game.RequiredEnv(context)) {
        launch.environment.emplace_back(variable);
    }

    return launch;
}

bool ExecLaunchCommand(const LaunchCommand &command)
{
    for (const std::string &variable : command.environment) {
        std::size_t separator = variable.find('=');
        if (separator != std::string::npos) {
            setenv(variable.substr(0, separator).c_str(),
                   variable.substr(separator + 1).c_str(), 1);
        }
    }

    if (!command.workingDirectory.empty()
        && chdir(command.workingDirectory.c_str()) == -1) {
        return false;
    }

    std::vector<char *> argv;
    argv.reserve(command.command.size() + 1);
    for (const std::string &argument : command.command) {
        argv.push_back(const_cast<char *>(argument.c_str()));
    }
    argv.push_back(nullptr);

    execvp(command.command.front().c_str(), argv.data());
    return false;
}

} // namespace rocklaunch
