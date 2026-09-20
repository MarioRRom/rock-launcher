#include "game_profile_model.h"

GameProfileModel::GameProfileModel(QObject *parent)
    : QObject(parent)
{
}

QString GameProfileModel::gameId() const
{
    return m_gameId;
}

void GameProfileModel::setGameId(const QString &id)
{
    if (m_gameId == id) {
        return;
    }
    m_gameId = id;
    emit gameIdChanged();
}
