#include "profile_model.h"

#include "game_profile_model.h"

namespace
{

ProfileEntry EntryFrom(const rocklaunch::ProfileConfig &profile)
{
    ProfileEntry entry(QString::fromStdString(profile.id),
                       QString::fromStdString(profile.name));
    entry.runnerName = QString::fromStdString(profile.runnerName);
    entry.runnerSource = QString::fromStdString(profile.runnerSource);
    entry.installDir = QString::fromStdString(profile.installDir.string());
    return entry;
}

} // namespace

ProfileModel::ProfileModel(rocklaunch::ProfileManager *manager,
                           GameProfileModel *gameModel,
                           QObject *parent)
    : QObject(parent)
    , m_profiles(manager)
    , m_gameProfileModel(gameModel)
{
    if (m_gameProfileModel) {
        connect(m_gameProfileModel, &GameProfileModel::gameIdChanged, this, &ProfileModel::refresh);
    }
    refresh();
}

QList<ProfileEntry> ProfileModel::profiles() const
{
    return m_entries;
}

QString ProfileModel::currentProfile() const
{
    return m_currentProfile;
}

void ProfileModel::setCurrentProfile(const QString &id)
{
    if (m_currentProfile == id) {
        return;
    }
    m_currentProfile = id;
    emit currentProfileChanged();
    emit currentProfileNameChanged();
}

QString ProfileModel::currentProfileName() const
{
    const ProfileEntry *entry = entryFor(m_currentProfile);
    return entry && !entry->name.isEmpty() ? entry->name : m_currentProfile;
}

QString ProfileModel::freeProfileId() const
{
    return m_profiles ? QString::fromStdString(m_profiles->PreviewNextId()) : QString();
}

QString ProfileModel::saveProfile(const QString &id, const QString &name,
                                  const QString &runnerName, const QString &runnerSource)
{
    QString profileId = id;
    if (profileId.isEmpty()) {
        profileId = createProfile(name);
        if (profileId.isEmpty()) {
            return {};
        }
    } else if (!renameProfile(profileId, name)) {
        return {};
    }

    const ProfileEntry stored = profileEntry(profileId);
    if (!runnerName.isEmpty()
        && (runnerName != stored.runnerName || runnerSource != stored.runnerSource)) {
        setRunner(profileId, runnerName, runnerSource);
    }
    return profileId;
}

QString ProfileModel::createProfile(const QString &name)
{
    if (!m_profiles) {
        return {};
    }

    try {
        std::optional<rocklaunch::ProfileConfig> created =
            m_profiles->CreateProfile(name.toStdString());
        if (!created.has_value()) {
            return {};
        }

        QString newId = QString::fromStdString(created->id);
        refresh();
        setCurrentProfile(newId);
        return newId;
    } catch (const std::exception &exception) {
        emit failed(QString::fromUtf8(exception.what()));
        return {};
    }
}

bool ProfileModel::renameProfile(const QString &id, const QString &name)
{
    if (!m_profiles) {
        return false;
    }

    try {
        if (!m_profiles->SetName(id.toStdString(), name.toStdString())) {
            return false;
        }
        refresh();
        return true;
    } catch (const std::exception &exception) {
        emit failed(QString::fromUtf8(exception.what()));
        return false;
    }
}

bool ProfileModel::nameValid(const QString &name) const
{
    return rocklaunch::ProfileManager::NameValid(name.toStdString());
}

QString ProfileModel::profileName(const QString &id) const
{
    // No id fallback (unlike currentProfileName): the edit dialog saves this
    // text as the name, and an id is not a name.
    const ProfileEntry *entry = entryFor(id);
    return entry ? entry->name : QString();
}

ProfileEntry ProfileModel::profileEntry(const QString &id) const
{
    const ProfileEntry *entry = entryFor(id);
    return entry ? *entry : ProfileEntry();
}

bool ProfileModel::setRunner(const QString &id, const QString &name, const QString &source)
{
    if (!m_profiles) {
        return false;
    }

    try {
        if (!m_profiles->SetRunner(id.toStdString(), name.toStdString(),
                                   source.toStdString())) {
            emit failed(QStringLiteral("The runner %1 (%2) is not installed")
                            .arg(name, source));
            return false;
        }
        refresh();
        return true;
    } catch (const std::exception &exception) {
        emit failed(QString::fromUtf8(exception.what()));
        return false;
    }
}

bool ProfileModel::removeProfile(const QString &id)
{
    if (!m_profiles) {
        return false;
    }

    try {
        if (!m_profiles->DeleteProfile(id.toStdString())) {
            return false;
        }
        if (m_currentProfile == id) {
            setCurrentProfile(QString());
        }
        refresh();
        return true;
    } catch (const std::exception &exception) {
        emit failed(QString::fromUtf8(exception.what()));
        return false;
    }
}

void ProfileModel::refresh()
{
    if (!m_profiles) {
        if (!m_entries.isEmpty()) {
            m_entries.clear();
            emit profilesChanged();
        }
        return;
    }

    const std::string gameId =
        m_gameProfileModel ? m_gameProfileModel->gameId().toStdString() : std::string();

    QList<ProfileEntry> updated;
    for (const rocklaunch::ProfileConfig &profile : m_profiles->ListProfiles(gameId)) {
        updated.append(EntryFrom(profile));
    }

    if (m_entries != updated) {
        m_entries = updated;
        emit profilesChanged();
        emit currentProfileNameChanged();
    }

    const QString fallback = m_entries.isEmpty() ? QString() : m_entries.first().id;
    if (m_currentProfile.isEmpty() && !fallback.isEmpty()) {
        setCurrentProfile(fallback);
    } else if (!m_currentProfile.isEmpty() && !entryFor(m_currentProfile)) {
        setCurrentProfile(fallback);
    }
}

const ProfileEntry *ProfileModel::entryFor(const QString &id) const
{
    for (const ProfileEntry &entry : m_entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}
