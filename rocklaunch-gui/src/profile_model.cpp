#include "profile_model.h"

#include "game_profile_model.h"

ProfileModel::ProfileModel(QObject *parent)
    : QObject(parent)
{
}

void ProfileModel::SetProfileManager(rocklaunch::ProfileManager *manager)
{
    if (m_profiles == manager) {
        return;
    }
    m_profiles = manager;
    refresh();
}

void ProfileModel::SetGameProfileModel(GameProfileModel *model)
{
    if (m_gameProfileModel == model) {
        return;
    }
    if (m_gameProfileModel) {
        disconnect(m_gameProfileModel, nullptr, this, nullptr);
    }
    m_gameProfileModel = model;
    if (m_gameProfileModel) {
        connect(m_gameProfileModel, &GameProfileModel::gameIdChanged,
                this, &ProfileModel::refresh);
    }
    refresh();
}

QStringList ProfileModel::profiles() const
{
    return m_profileIds;
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
}

QString ProfileModel::createProfile()
{
    if (!m_profiles || !m_gameProfileModel) {
        return {};
    }

    // Core picks the next free "<gameId>-<n>" id and binds the default prefix.
    std::optional<rocklaunch::ProfileConfig> created =
        m_profiles->CreateDefaultProfile("");
    if (!created.has_value()) {
        return {};
    }

    QString id = QString::fromStdString(created->id);
    refresh();
    m_currentProfile = id;
    emit currentProfileChanged();
    return id;
}

bool ProfileModel::removeProfile(const QString &id)
{
    if (!m_profiles) {
        return false;
    }

    bool removed = m_profiles->DeleteProfile(id.toStdString());
    if (removed) {
        if (m_currentProfile == id) {
            m_currentProfile.clear();
            emit currentProfileChanged();
        }
        refresh();
    }
    return removed;
}

void ProfileModel::refresh()
{
    if (!m_profiles || !m_gameProfileModel) {
        if (!m_profileIds.isEmpty()) {
            m_profileIds.clear();
            emit profilesChanged();
        }
        return;
    }

    std::string gameId = m_gameProfileModel->gameId().toStdString();
    // Core lists only profiles bound to the selected game and skips corrupt
    // profile files with a logged warning instead of throwing.
    QStringList updated;
    for (const rocklaunch::ProfileConfig &profile : m_profiles->ListProfiles(gameId)) {
        updated.append(QString::fromStdString(profile.id));
    }

    if (m_profileIds != updated) {
        m_profileIds = updated;
        emit profilesChanged();
    }

    if (m_currentProfile.isEmpty() && !m_profileIds.isEmpty()) {
        m_currentProfile = m_profileIds.first();
        emit currentProfileChanged();
    } else if (!m_currentProfile.isEmpty() && !m_profileIds.contains(m_currentProfile)) {
        m_currentProfile = m_profileIds.isEmpty() ? QString() : m_profileIds.first();
        emit currentProfileChanged();
    }
}
