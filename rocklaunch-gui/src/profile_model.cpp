#include "profile_model.h"

#include "game_profile_model.h"

#include <utility>

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

QString ProfileModel::createProfile(const QString &name)
{
    if (!m_profiles) {
        return {};
    }

    std::optional<rocklaunch::ProfileConfig> created =
        m_profiles->CreateProfile(name.toStdString());
    if (!created.has_value()) {
        return {};
    }

    QString newId = QString::fromStdString(created->id);
    refresh();
    setCurrentProfile(newId);
    return newId;
}

bool ProfileModel::renameProfile(const QString &id, const QString &name)
{
    if (!m_profiles || !m_profiles->SetName(id.toStdString(), name.toStdString())) {
        return false;
    }
    refresh();
    return true;
}

bool ProfileModel::nameValid(const QString &name) const
{
    return rocklaunch::ProfileManager::NameValid(name.toStdString());
}

QString ProfileModel::profileName(const QString &id) const
{
    const ProfileEntry *entry = entryFor(id);
    return entry ? entry->name : QString();
}

bool ProfileModel::removeProfile(const QString &id)
{
    if (!m_profiles) {
        return false;
    }

    bool removed = m_profiles->DeleteProfile(id.toStdString());
    if (removed) {
        if (m_currentProfile == id) {
            setCurrentProfile(QString());
        }
        refresh();
    }
    return removed;
}

void ProfileModel::refresh()
{
    if (!m_profiles || !m_gameProfileModel) {
        if (!m_entries.isEmpty()) {
            m_entries.clear();
            emit profilesChanged();
        }
        return;
    }

    QList<ProfileEntry> updated;
    for (const rocklaunch::ProfileConfig &profile :
         m_profiles->ListProfiles(m_gameProfileModel->gameId().toStdString())) {
        updated.append(ProfileEntry(QString::fromStdString(profile.id),
                                    QString::fromStdString(profile.name)));
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
