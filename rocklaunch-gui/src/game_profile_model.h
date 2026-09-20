#pragma once

#include <QObject>
#include <QString>

class GameProfileModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString gameId READ gameId WRITE setGameId NOTIFY gameIdChanged)

public:
    explicit GameProfileModel(QObject *parent = nullptr);

    QString gameId() const;
    void setGameId(const QString &id);

signals:
    void gameIdChanged();

private:
    QString m_gameId;
};
