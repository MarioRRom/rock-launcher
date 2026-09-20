#pragma once

#include "rocklaunch/core/config_store.h"

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class GameProfileModel;

class ProfileModel final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QString currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY currentProfileChanged)

public:
    explicit ProfileModel(QObject *parent = nullptr);

    static ProfileModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        return s_instance;
    }

    static void setInstance(ProfileModel *instance)
    {
        s_instance = instance;
    }

    void SetConfigStore(rocklaunch::ConfigStore *store);
    void SetGameProfileModel(GameProfileModel *model);

    QStringList profiles() const;
    QString currentProfile() const;
    void setCurrentProfile(const QString &id);

    Q_INVOKABLE QString createProfile();
    Q_INVOKABLE bool removeProfile(const QString &id);
    Q_INVOKABLE void refresh();

signals:
    void profilesChanged();
    void currentProfileChanged();

private:
    QString nextDefaultId() const;

    rocklaunch::ConfigStore *m_store = nullptr;
    GameProfileModel *m_gameProfileModel = nullptr;
    QStringList m_profiles;
    QString m_currentProfile;
    inline static ProfileModel *s_instance = nullptr;
};
