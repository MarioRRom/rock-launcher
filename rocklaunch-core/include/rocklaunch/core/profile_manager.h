#pragma once

#include "rocklaunch/core/config_store.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rocklaunch
{

class IGameProfile;
class RunnerManager;

// A single problem found while validating a profile before use (launch, GUI forms...).
struct ValidationIssue
{
    enum class Severity
    {
        Warning,
        Error,
    };

    Severity severity;
    std::string message;
};

// Result of the pre-flight checks. Callers only need isValid for the exit code
// and may render issues themselves; core also reports every issue through Logger.
struct ProfileValidation
{
    bool isValid = false;
    std::vector<ValidationIssue> issues;
};

// Business rules for profiles: naming, defaults, game path assignment, runner
// assignment and pre-flight validation. Both the CLI and the GUI share these
// rules instead of re-implementing them.
class ProfileManager final
{
public:
    // Fails fast (logged + invalid_argument) when the game profile has no id, so
    // names like "<empty>-1" or profiles without a "Game:" can never be created.
    // The gameProfile reference must outlive this manager.
    ProfileManager(ConfigStore &store, const IGameProfile &gameProfile);

    // Lists profiles, optionally restricted to those bound to gameId. Empty gameId
    // lists profiles of every game; non-empty returns only matching games. Because
    // the game binding is stored inside each profile JSON, all files are read and
    // then filtered in core — the GUI never re-implements this comparison.
    std::vector<ProfileConfig> ListProfiles(const std::string &gameId = "") const;
    // nullopt when the id is invalid or no such profile exists (both logged).
    std::optional<ProfileConfig> GetProfile(const std::string &id) const;

    // Pure check for a display tag: UTF-8 is fine, but quotes and control
    // characters would break the `<id> "<name>"` rendering of a listing. Forms
    // call it to reject input before saving; core logs the reason when it does.
    static bool NameValid(const std::string &name);

    // The id the next created profile will take. A hint for a form, not a
    // reservation: CreateProfile picks the same id when it runs, so a profile
    // created in between moves it forward.
    std::string PreviewNextId() const;

    // Creates a profile bound to the managed game under the next free
    // "<gameId>-<n>" id, so ids can never collide and are never the user's to
    // pick. name is an optional display tag; an empty tag leaves the profile
    // identified by its id alone. Returns nullopt (logged) on an invalid name.
    std::optional<ProfileConfig> CreateProfile(const std::string &name = "");

    // Sets the display tag of a profile; an empty name clears it. Returns false
    // (logged) when the profile does not exist or the name is not valid.
    bool SetName(const std::string &id, const std::string &name);

    bool DeleteProfile(const std::string &id);

    // Validates the installation through the game profile, rejects install dirs
    // already claimed by another profile and profiles bound to another game.
    // Returns the resolved install dir, or nullopt (logged) on any failure.
    std::optional<fs::path> SetInstallPath(const std::string &id, const fs::path &path);

    bool SetRunner(const std::string &id, const std::string &runnerId,
                   const RunnerManager &runners);

    ProfileValidation ValidateProfile(const std::string &id,
                                      const RunnerManager &runners) const;

private:
    ConfigStore &m_store;
    const IGameProfile &m_gameProfile;
};

} // namespace rocklaunch