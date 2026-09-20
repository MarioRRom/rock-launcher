#pragma once

#include <QObject>
#include <QProcess>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/runners/runner_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

class ProfileModel;

class LaunchController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit LaunchController(QObject *parent = nullptr);

    static LaunchController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        return s_instance;
    }

    static void setInstance(LaunchController *instance)
    {
        s_instance = instance;
    }

    void SetConfigStore(rocklaunch::ConfigStore *store);
    void SetProfileModel(ProfileModel *model);

    Q_INVOKABLE void launch();

signals:
    void launchError(const QString &message);

private:
    rocklaunch::ConfigStore *m_configStore = nullptr;
    ProfileModel *m_profileModel = nullptr;
    rocklaunch::RunnerManager m_runnerManager;
    rocklaunch::Rocksmith2014RemasteredProfile m_gameProfile;
    QProcess m_process;
    inline static LaunchController *s_instance = nullptr;
};
