#pragma once

#include <QObject>
#include <QProcess>

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/runners/runner_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

class ProfileModel;

class LaunchController final : public QObject
{
    Q_OBJECT

public:
    explicit LaunchController(QObject *parent = nullptr);

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
};
