#include "rocklaunch/core/profile_manager.h"

#include "rocklaunch/core/game_profile.h"
#include "rocklaunch/core/logger.h"
#include "rocklaunch/core/manual_source.h"
#include "rocklaunch/core/runners/runners.h"

#include <exception>
#include <stdexcept>

namespace rocklaunch
{

namespace
{

// The only source of profile ids: the first unused "<gameId>-<n>", so two
// profiles can never share one.
std::string NextDefaultProfileId(const std::string &gameId, const ConfigStore &store)
{
    for (int index = 1;; ++index) {
        std::string candidate = gameId + "-" + std::to_string(index);
        if (!store.ProfileExists(candidate)) {
            return candidate;
        }
    }
}

// Reports why a name was refused. Separate from NameValid() because a form calls
// that one on every keystroke and must not write to the log.
void LogInvalidName(const std::string &name)
{
    Logger logger;
    logger.Error("ProfileManager: Profile names may not contain quotes or control "
                 "characters (got: " + name + ")");
}

} // namespace

ProfileManager::ProfileManager(ConfigStore &store, const IGameProfile &gameProfile)
    : m_store(store)
    , m_gameProfile(gameProfile)
{
    if (m_gameProfile.Id().empty()) {
        Logger logger;
        logger.Error("ProfileManager: no game profile selected; refusing to create profiles");
        throw std::invalid_argument("A game profile with an id is required");
    }
}

std::vector<ProfileConfig> ProfileManager::ListProfiles(const std::string &gameId) const
{
    std::vector<ProfileConfig> profiles;
    for (const std::string &id : m_store.ListProfileIds()) {
        // One corrupt or unreadable profile must not break the whole list (the
        // GUI refreshes it routinely); it is skipped and reported instead.
        try {
            ProfileConfig profile = m_store.LoadProfile(id);
            if (gameId.empty() || profile.gameId == gameId) {
                profiles.push_back(profile);
            }
        } catch (const std::exception &error) {
            Logger logger;
            logger.Warn("ProfileManager: skipping unreadable profile " + id + ": "
                        + error.what());
        }
    }
    return profiles;
}

std::optional<ProfileConfig> ProfileManager::GetProfile(const std::string &id) const
{
    // An invalid id can never exist; check before the store would throw.
    if (!m_store.ProfileIdValid(id) || !m_store.ProfileExists(id)) {
        Logger logger;
        logger.Error("ProfileManager: Profile not found: " + id);
        return std::nullopt;
    }

    // A corrupt profile JSON must degrade to a logged error, not an exception;
    // the GUI reads profiles routinely and cannot rely on a catch-all handler.
    try {
        return m_store.LoadProfile(id);
    } catch (const std::exception &error) {
        Logger logger;
        logger.Error("ProfileManager: Profile " + id + " could not be read: " + error.what());
        return std::nullopt;
    }
}

bool ProfileManager::NameValid(const std::string &name)
{
    // A name is only ever shown as `<id> "<name>"`, so quotes and control
    // characters are rejected instead of escaped. UTF-8 passes through untouched.
    for (char character : name) {
        unsigned char byte = static_cast<unsigned char>(character);
        if (byte < 0x20 || byte == 0x7f || character == '"') {
            return false;
        }
    }
    return true;
}

std::string ProfileManager::PreviewNextId() const
{
    return NextDefaultProfileId(m_gameProfile.Id(), m_store);
}

std::optional<ProfileConfig> ProfileManager::CreateProfile(const std::string &name)
{
    if (!NameValid(name)) {
        LogInvalidName(name);
        return std::nullopt;
    }

    ProfileConfig config;
    config.id = PreviewNextId();
    config.name = name;
    config.gameId = m_gameProfile.Id();
    config.prefixDir = m_store.DataDir() / "prefixes" / config.id;
    m_store.SaveProfile(config);

    return config;
}

bool ProfileManager::SetName(const std::string &id, const std::string &name)
{
    std::optional<ProfileConfig> profile = GetProfile(id);
    if (!profile.has_value()) {
        return false;
    }

    if (!NameValid(name)) {
        LogInvalidName(name);
        return false;
    }

    profile->name = name;
    m_store.SaveProfile(*profile);

    return true;
}

bool ProfileManager::DeleteProfile(const std::string &id)
{
    if (!m_store.ProfileIdValid(id) || !m_store.ProfileExists(id)) {
        Logger logger;
        logger.Error("ProfileManager: Profile not found: " + id);
        return false;
    }

    return m_store.DeleteProfile(id);
}

std::optional<fs::path> ProfileManager::SetInstallPath(const std::string &id, const fs::path &path)
{
    std::optional<ProfileConfig> profile = GetProfile(id);
    if (!profile.has_value()) {
        return std::nullopt;
    }

    ManualSource source(path);
    std::optional<fs::path> installDir = source.Locate(m_gameProfile);
    if (!installDir.has_value()) {
        Logger logger;
        logger.Error("ProfileManager: Invalid installation for game " + m_gameProfile.Id()
                     + ": " + path.string()
                     + "\nExpected the game's executable and data directory.");
        return std::nullopt;
    }

    std::optional<std::string> conflictingProfile =
        m_store.ProfileUsingInstallDir(*installDir, id);
    if (conflictingProfile.has_value()) {
        Logger logger;
        logger.Error("ProfileManager: This game installation is already used by profile: "
                     + *conflictingProfile);
        return std::nullopt;
    }

    // A profile is bound to one game; never point it at another game's installation.
    if (profile->gameId != m_gameProfile.Id()) {
        Logger logger;
        logger.Error("ProfileManager: Profile " + id + " is for game " + profile->gameId
                     + ", not " + m_gameProfile.Id());
        return std::nullopt;
    }

    profile->installDir = *installDir;
    m_store.SaveProfile(*profile);

    return installDir;
}

bool ProfileManager::SetRunner(const std::string &id, const std::string &name,
                               const std::string &source)
{
    std::optional<ProfileConfig> profile = GetProfile(id);
    if (!profile.has_value()) {
        return false;
    }

    if (!Runners::Find(name, source).has_value()) {
        Logger logger;
        logger.Error("ProfileManager: Runner not installed: " + source + "/" + name);
        return false;
    }

    profile->runnerName = name;
    profile->runnerSource = source;
    m_store.SaveProfile(*profile);

    return true;
}

ProfileValidation ProfileManager::ValidateProfile(const std::string &id) const
{
    std::optional<ProfileConfig> profile = GetProfile(id);
    if (!profile.has_value()) {
        return ProfileValidation{false, {ValidationIssue{ValidationIssue::Severity::Error,
                                                         "Profile not found: " + id}}};
    }

    ProfileValidation validation;
    validation.isValid = true;

    auto addError = [&validation](const std::string &message) {
        validation.isValid = false;
        Logger logger;
        logger.Error("ProfileManager: " + message);
        validation.issues.emplace_back(ValidationIssue{ValidationIssue::Severity::Error, message});
    };

    // First failing check wins, mirroring the sequential launch pre-flight.
    if (profile->gameId != m_gameProfile.Id()) {
        addError("Profile " + id + " is for game " + profile->gameId
                 + ", which this build does not support.");
    } else if (profile->installDir.empty()) {
        addError("Profile " + id + " has no install path. "
                 "Use set-path <profile_id> <path> first.");
    } else if (profile->runnerName.empty() || profile->runnerSource.empty()) {
        addError("Profile " + id + " has no runner. "
                 "Use runner set <profile_id> <source>/<name> first.");
    } else if (!Runners::Find(profile->runnerName, profile->runnerSource).has_value()) {
        addError("Runner not installed: " + profile->runnerSource + "/" + profile->runnerName);
    }

    return validation;
}

} // namespace rocklaunch