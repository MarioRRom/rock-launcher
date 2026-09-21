#pragma once

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/game_profile.h"
#include "rocklaunch/core/runners/runner.h"

#include <optional>
#include <string>
#include <vector>

namespace rocklaunch
{

// Command line and environment ready to run the game.
struct LaunchCommand
{
    std::vector<std::string> command;
    std::vector<std::string> environment;
    // Working directory for the launched process; the game's install directory.
    fs::path workingDirectory;
};

// Creates the prefix directory when it does not exist yet and applies the global
// prefix settings (Audio=alsa). For Proton runners the keys are written to
// <prefixDir>/pfx (the WINEPREFIX the proton script actually uses); Wine runners
// use <prefixDir> directly. Returns warnings for settings that could not be applied.
std::vector<std::string> EnsurePrefix(const fs::path &prefixDir, const Runner &runner);

// Commands that configure a fresh prefix (Audio=alsa registry keys) and the
// environment they need. Empty when no wine binary is available. EnsurePrefix()
// executes them synchronously for the CLI; the GUI runs them asynchronously so
// its event loop is not blocked.
std::vector<LaunchCommand> BuildPrefixCommands(const fs::path &prefixDir, const Runner &runner);

// Command that stops the wineserver of a prefix (wineserver -k), used to shut
// the prefix down cleanly when stopping a game. Nullopt when no wineserver
// executable can be located.
std::optional<LaunchCommand> BuildWineKillCommand(const fs::path &prefixDir, const Runner &runner);

// Builds the command line and environment to run profile's game with the given runner.
LaunchCommand BuildLaunchCommand(const ProfileConfig &profile,
                                 const Runner &runner,
                                 const IGameProfile &game);

// Runs the command, replacing the current process. Returns false when the process
// could not be started; on success the current process no longer exists.
bool ExecLaunchCommand(const LaunchCommand &command);

} // namespace rocklaunch
