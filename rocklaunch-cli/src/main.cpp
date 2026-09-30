#include "rocklaunch/cli/cli_ui.h"
#include "rocklaunch/cli/progress_bar.h"
#include "rocklaunch/cli/runners/runner_commands.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/patches/patch_manager.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"
#include "rocklaunch/core/runners/runners.h"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{

// Profile commands

// Creates a profile from core business rules; the id is always the next free
// "<gameId>-<n>" and name is an optional tag. Only the outcome is printed here.
int CreateProfile(const std::string &name, rocklaunch::ProfileManager &profiles)
{
    std::optional<rocklaunch::ProfileConfig> created = profiles.CreateProfile(name);
    if (!created.has_value()) {
        return 1;
    }

    std::cout << "Created profile: " << created->id;
    if (!created->name.empty()) {
        std::cout << " \"" << created->name << '"';
    }
    std::cout << '\n';
    return 0;
}

// An empty name is valid: core clears the tag.
int RenameProfile(const std::string &profileId,
                  const std::string &name,
                  rocklaunch::ProfileManager &profiles)
{
    if (!profiles.SetName(profileId, name)) {
        return 1;
    }

    if (name.empty()) {
        std::cout << "Cleared the name of profile " << profileId << '\n';
    } else {
        std::cout << "Profile " << profileId << " is now named \"" << name << "\"\n";
    }
    return 0;
}

int ShowProfile(const std::string &profileId, rocklaunch::ProfileManager &profiles)
{
    std::optional<rocklaunch::ProfileConfig> maybeConfig = profiles.GetProfile(profileId);
    if (!maybeConfig.has_value()) {
        return 1;
    }

    const rocklaunch::ProfileConfig &config = *maybeConfig;
    std::cout << "Profile: " << config.id << '\n'
              << "Name: " << (config.name.empty() ? "not assigned" : config.name) << '\n'
              << "Game: " << config.gameId << '\n'
              << "Prefix path: " << config.prefixDir << '\n'
              << "Runner: "
              << (config.runnerName.empty() ? "not assigned"
                                            : config.runnerSource + "/" + config.runnerName)
              << '\n'
              << "Install path: ";
    if (config.installDir.empty()) {
        std::cout << "not assigned\n";
    } else {
        std::cout << config.installDir << '\n';
    }

    std::cout << "Applied patches: ";
    bool anyPatch = false;
    for (const auto &entry : config.patches) {
        if (entry.second.enabled) {
            if (anyPatch) {
                std::cout << ", ";
            }
            std::cout << entry.first;
            anyPatch = true;
        }
    }
    if (!anyPatch) {
        std::cout << "none";
    }
    std::cout << '\n';

    return 0;
}

int RemoveProfile(const std::string &profileId,
                  rocklaunch::ProfileManager &profiles,
                  bool force)
{
    // DeleteProfile checks existence itself, so a corrupt profile (whose JSON
    // cannot be read) is still removable from both the CLI and the GUI.
    if (!force && !ConfirmDestructive("Remove profile " + profileId + " and its prefix?")) {
        std::cout << "Aborted.\n";
        return 1;
    }

    if (!profiles.DeleteProfile(profileId)) {
        return 1;
    }

    std::cout << "Removed profile: " << profileId << '\n';
    return 0;
}

// Patch commands

int PatchListAll(const rocklaunch::PatchManager &patchManager)
{
    std::cout << Color(PadLeft("Patch", 26), "1") << Color(PadLeft("Game", 26), "1")
              << Color("Name", "1") << '\n';
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        rocklaunch::PatchPreset preset = patch->Preset();
        std::cout << Color(PadLeft(patch->Id(), 26), kPatchColor)
                  << PadLeft(preset.gameId, 26) << preset.name << '\n';
    }

    return 0;
}

int PatchList(const std::string &profileId,
              rocklaunch::ConfigStore &configStore,
              const rocklaunch::PatchManager &patchManager)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    std::cout << Color(PadLeft("Patch", 26), "1") << Color(PadLeft("Game", 26), "1")
              << Color(PadLeft("Status", 10), "1") << Color("Name", "1") << '\n';
    for (const rocklaunch::ILaunchPatch *patch : patchManager.List()) {
        if (patch->GameId() != config.gameId) {
            continue;
        }

        rocklaunch::PatchPreset preset = patch->Preset();
        bool enabled = patch->IsEnabled(config);
        std::string status = enabled ? "enabled" : "disabled";
        std::cout << Color(PadLeft(patch->Id(), 26), kPatchColor) << PadLeft(preset.gameId, 26)
                  << Color(PadLeft(status, 10), enabled ? "32" : kPatchColor)
                  << preset.name << '\n';
    }

    return 0;
}

int PatchAdd(const std::string &profileId,
             const std::string &patchId,
             rocklaunch::PatchManager &patchManager,
             bool force)
{
    std::string error;
    if (!patchManager.Enable(profileId, patchId, error, force)) {
        PrintError(error);
        return 1;
    }

    std::cout << "Enabled patch " << patchId << " on profile " << profileId << '\n';
    return 0;
}

int PatchRemove(const std::string &profileId,
                const std::string &patchId,
                rocklaunch::PatchManager &patchManager,
                bool force)
{
    std::string error;
    if (!patchManager.Disable(profileId, patchId, error, force)) {
        PrintError(error);
        return 1;
    }

    std::cout << "Disabled patch " << patchId << " on profile " << profileId << '\n';
    return 0;
}

int PatchStatus(const std::string &profileId,
                const std::string &patchId,
                rocklaunch::ConfigStore &configStore,
                const rocklaunch::PatchManager &patchManager)
{
    if (!configStore.ProfileExists(profileId)) {
        PrintError("Profile not found: " + profileId);
        return 1;
    }

    const rocklaunch::ILaunchPatch *patch = patchManager.Find(patchId);
    if (patch == nullptr) {
        PrintError("Unknown patch: " + patchId);
        return 1;
    }

    rocklaunch::ProfileConfig config = configStore.LoadProfile(profileId);
    rocklaunch::PatchPreset preset = patch->Preset();
    std::cout << "Patch: " << patch->Id() << '\n'
              << "Name: " << preset.name << '\n'
              << "Game: " << preset.gameId << '\n'
              << "Status: " << (patch->IsEnabled(config) ? "enabled" : "disabled") << '\n'
              << "Reversible: " << (preset.reversible ? "yes" : "no") << '\n'
              << "Install-level: " << (preset.installLevel ? "yes" : "no") << '\n'
              << "Description: " << preset.description << '\n';
    for (const rocklaunch::PatchOperation &operation : preset.operations) {
        std::cout << "  " << rocklaunch::PatchOperationTypeName(operation.type) << ' '
                  << operation.target << " - " << operation.detail << '\n';
    }

    return 0;
}

// Path assignment

int SetPath(const std::string &profileId,
            const std::string &path,
            rocklaunch::ProfileManager &profiles)
{
    std::optional<rocklaunch::fs::path> installDir = profiles.SetInstallPath(profileId, path);
    if (!installDir.has_value()) {
        return 1;
    }

    std::cout << "Saved profile " << profileId << ": " << *installDir << '\n';
    return 0;
}

int ListProfiles(rocklaunch::ProfileManager &profiles, const std::string &gameId)
{
    std::vector<rocklaunch::ProfileConfig> profileList = profiles.ListProfiles(gameId);
    if (profileList.empty()) {
        if (gameId.empty()) {
            std::cout << "No profiles are configured.\n";
        } else {
            std::cout << "No profiles for game " << gameId << ".\n";
        }
        return 0;
    }

    for (const rocklaunch::ProfileConfig &profile : profileList) {
        std::cout << Color(profile.id, kProfileColor);
        if (!profile.name.empty()) {
            std::cout << " \"" << profile.name << '"';
        }
        std::cout << '\n';
    }

    return 0;
}

// Launch

int LaunchProfile(const std::string &profileId,
                  rocklaunch::ProfileManager &profiles,
                  const rocklaunch::Rocksmith2014RemasteredProfile &gameProfile)
{
    // Pre-flight checks are core business rules shared with the GUI.
    rocklaunch::ProfileValidation validation = profiles.ValidateProfile(profileId);
    if (!validation.isValid) {
        return 1;
    }

    rocklaunch::ProfileConfig config = profiles.GetProfile(profileId).value();

    const rocklaunch::RunnerRef runner = rocklaunch::ResolveRunner(config);
    rocklaunch::LaunchCommand launch =
        rocklaunch::BuildLaunchCommand(config, runner, gameProfile);

    // Prefix creation also reports its warnings through the core logger.
    rocklaunch::EnsurePrefix(config.prefixDir, runner);

    if (!rocklaunch::ExecLaunchCommand(launch)) {
        PrintError("Failed to start '" + launch.command.front() + "': "
                    + std::strerror(errno));
        return 1;
    }

    return 0;
}

} // namespace

// Entry point

int main(int argc, char *argv[])
{
    try {
        rocklaunch::ConfigStore configStore;
        rocklaunch::Rocksmith2014RemasteredProfile profile;
        rocklaunch::ProfileManager profiles(configStore, profile);
        rocklaunch::PatchManager patchManager =
            rocklaunch::PatchManager::CreateDefault(configStore);

        if (argc == 1) {
            PrintUsage();
            return 0;
        }

        std::string_view argument(argv[1]);
        if (argument == "--help" || argument == "-h") {
            PrintUsage();
            return 0;
        }

        if (argument == "--version") {
            std::cout << "rocklaunch-cli " << ROCKLAUNCH_VERSION << '\n';
            return 0;
        }

        if (argument == "set-path" && argc == 4) {
            return SetPath(argv[2], argv[3], profiles);
        }

        if (argument == "runner" && argc == 3 && std::string_view(argv[2]) == "-u") {
            nlohmann::json releases;
            try {
                releases = rocklaunch::Runners::Releases(true);
            } catch (const std::exception &error) {
                PrintError(std::string(error.what()));
                return 1;
            }
            std::cout << "Updated the releases list (" << releases["releases"].size()
                      << " releases).\n";
            return 0;
        }

        if (argument == "runner" && argc == 3 && std::string_view(argv[2]) == "list") {
            return RunnerList();
        }

        if (argument == "runner" && argc == 5 && std::string_view(argv[2]) == "set") {
            return RunnerSet(argv[3], argv[4]);
        }

        if (argument == "runner" && argc >= 3 && std::string_view(argv[2]) == "search") {
            bool refresh = false;
            std::string query;
            for (int i = 3; i < argc; ++i) {
                const std::string_view arg = argv[i];
                if (arg == "-u") {
                    refresh = true;
                } else if (arg.rfind('-', 0) == 0) {
                    PrintUsageError("unknown option '" + std::string(arg) + "'",
                                    "runner", "search");
                    return 1;
                } else {
                    if (!query.empty()) {
                        query += ' ';
                    }
                    query += argv[i];
                }
            }
            return RunnerSearch(query, refresh);
        }

        if (argument == "runner" && argc >= 4 && std::string_view(argv[2]) == "install") {
            std::string runnerToken;
            std::string fileName;
            bool force = false;
            for (int i = 3; i < argc; ++i) {
                const std::string_view arg = argv[i];
                if (IsForceFlag(arg)) {
                    force = true;
                } else if (runnerToken.empty()) {
                    runnerToken = arg;
                } else if (fileName.empty()) {
                    fileName = arg;
                } else {
                    PrintUsageError("unexpected argument '" + std::string(arg) + "'",
                                    "runner", "install");
                    return 1;
                }
            }
            if (runnerToken.empty()) {
                PrintUsageError("missing runner", "runner", "install");
                return 1;
            }
            return RunnerInstall(runnerToken, fileName, force);
        }

        if (argument == "runner" && argc >= 4 && std::string_view(argv[2]) == "remove") {
            bool force = false;
            std::string runnerToken;
            for (int i = 3; i < argc; ++i) {
                const std::string_view arg = argv[i];
                if (IsForceFlag(arg)) {
                    force = true;
                } else if (runnerToken.empty()) {
                    runnerToken = arg;
                } else {
                    PrintUsageError("unexpected argument '" + std::string(arg) + "'",
                                    "runner", "remove");
                    return 1;
                }
            }
            if (runnerToken.empty()) {
                PrintUsageError("missing runner", "runner", "remove");
                return 1;
            }
            return RunnerRemove(runnerToken, force);
        }

        if (argument == "launch" && argc == 3) {
            return LaunchProfile(argv[2], profiles, profile);
        }

        if (argument == "patch" && argc == 3 && std::string_view(argv[2]) == "list") {
            return PatchListAll(patchManager);
        }

        if (argument == "patch" && argc == 4 && std::string_view(argv[2]) == "list") {
            return PatchList(argv[3], configStore, patchManager);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "add") {
            return PatchAdd(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "add"
            && IsForceFlag(std::string_view(argv[3]))) {
            return PatchAdd(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "remove") {
            return PatchRemove(argv[3], argv[4], patchManager, false);
        }

        if (argument == "patch" && argc == 6 && std::string_view(argv[2]) == "remove"
            && IsForceFlag(std::string_view(argv[3]))) {
            return PatchRemove(argv[4], argv[5], patchManager, true);
        }

        if (argument == "patch" && argc == 5 && std::string_view(argv[2]) == "status") {
            return PatchStatus(argv[3], argv[4], configStore, patchManager);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "list") {
            return ListProfiles(profiles, "");
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "list") {
            return ListProfiles(profiles, argv[3]);
        }

        if (argument == "profile" && argc == 3 && std::string_view(argv[2]) == "new") {
            return CreateProfile("", profiles);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "new") {
            return CreateProfile(argv[3], profiles);
        }

        if (argument == "profile" && argc == 5 && std::string_view(argv[2]) == "rename") {
            return RenameProfile(argv[3], argv[4], profiles);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "show") {
            return ShowProfile(argv[3], profiles);
        }

        if (argument == "profile" && argc == 4 && std::string_view(argv[2]) == "remove") {
            return RemoveProfile(argv[3], profiles, false);
        }

        if (argument == "profile" && argc == 5 && std::string_view(argv[2]) == "remove"
            && IsForceFlag(std::string_view(argv[3]))) {
            return RemoveProfile(argv[4], profiles, true);
        }

        bool knownTopLevel = argument == "profile" || argument == "runner"
            || argument == "patch" || argument == "launch"
            || argument == "set-path";
        if (knownTopLevel) {
            std::string subcommand = argc >= 3 ? argv[2] : "";
            if (!subcommand.empty() && !IsKnownSubcommand(argument, argv[2])) {
                PrintUsageError("unrecognized subcommand '" + subcommand + "'",
                                std::string(argument));
            } else {
                PrintUsageError("invalid arguments for '" + std::string(argument) + "'",
                                std::string(argument), subcommand);
            }
            return 1;
        }

        PrintUsageError("unrecognized subcommand '" + std::string(argument) + "'",
                        std::string(argument));
        return 1;
    } catch (const std::exception &error) {
        PrintError("rocklaunch-cli: " + std::string(error.what()));
        return 1;
    }
}
