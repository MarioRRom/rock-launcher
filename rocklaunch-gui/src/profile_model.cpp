#include "profile_model.h"

#include "game_profile_model.h"

ProfileModel::ProfileModel(QObject *parent)
    : QObject(parent)
{
}

void ProfileModel::SetConfigStore(rocklaunch::ConfigStore *store)
{
    m_store = store;
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
    return m_profiles;
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
    if (!m_store || !m_gameProfileModel) {
        return {};
    }

    QString id = nextDefaultId();
    std::string stdId = id.toStdString();

    rocklaunch::ProfileConfig config;
    config.id = stdId;
    config.gameId = m_gameProfileModel->gameId().toStdString();
    config.prefixDir = rocklaunch::ConfigStore::DefaultDataDir() / "prefixes" / stdId;
    m_store->SaveProfile(config);

    refresh();
    m_currentProfile = id;
    emit currentProfileChanged();
    return id;
}

bool ProfileModel::removeProfile(const QString &id)
{
    if (!m_store) {
        return false;
    }

    bool removed = m_store->DeleteProfile(id.toStdString());
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
    if (!m_store || !m_gameProfileModel) {
        if (!m_profiles.isEmpty()) {
            m_profiles.clear();
            emit profilesChanged();
        }
        return;
    }

    std::string gameId = m_gameProfileModel->gameId().toStdString();
    QStringList updated;
    for (const auto &id : m_store->ListProfileIds()) {
        rocklaunch::ProfileConfig config = m_store->LoadProfile(id);
        if (config.gameId == gameId) {
            updated.append(QString::fromStdString(id));
        }
    }

    if (m_profiles != updated) {
        m_profiles = updated;
        emit profilesChanged();
    }

    if (m_currentProfile.isEmpty() && !m_profiles.isEmpty()) {
        m_currentProfile = m_profiles.first();
        emit currentProfileChanged();
    } else if (!m_currentProfile.isEmpty() && !m_profiles.contains(m_currentProfile)) {
        m_currentProfile = m_profiles.isEmpty() ? QString() : m_profiles.first();
        emit currentProfileChanged();
    }
}

QString ProfileModel::nextDefaultId() const
{
    if (!m_store || !m_gameProfileModel) {
        return {};
    }

    std::string gameId = m_gameProfileModel->gameId().toStdString();
    for (int i = 1;; ++i) {
        std::string candidate = gameId + "-" + std::to_string(i);
        if (!m_store->ProfileExists(candidate)) {
            return QString::fromStdString(candidate);
        }
    }
}
