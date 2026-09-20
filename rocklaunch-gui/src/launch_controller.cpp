#include "launch_controller.h"

#include "profile_model.h"

#include <rocklaunch/core/launch.h>

#include <QDebug>

LaunchController::LaunchController(QObject *parent)
    : QObject(parent)
    , m_runnerManager(rocklaunch::RunnerManager::CreateDefault())
{
}

void LaunchController::SetConfigStore(rocklaunch::ConfigStore *store)
{
    m_configStore = store;
}

void LaunchController::SetProfileModel(ProfileModel *model)
{
    m_profileModel = model;
}

void LaunchController::launch()
{
    if (!m_configStore || !m_profileModel) {
        emit launchError("Controller not initialized");
        return;
    }

    QString profileId = m_profileModel->currentProfile();
    if (profileId.isEmpty()) {
        emit launchError("No profile selected");
        return;
    }

    if (!m_configStore->ProfileExists(profileId.toStdString())) {
        emit launchError("Profile not found: " + profileId);
        return;
    }

    rocklaunch::ProfileConfig config = m_configStore->LoadProfile(profileId.toStdString());

    if (config.gameId != m_gameProfile.Id()) {
        emit launchError("Profile is for a different game");
        return;
    }

    if (config.installDir.empty()) {
        emit launchError("Profile has no install path. Set game path first.");
        return;
    }

    if (config.runnerId.empty()) {
        emit launchError("Profile has no runner. Select a runner first.");
        return;
    }

    std::optional<rocklaunch::Runner> runner = m_runnerManager.Find(config.runnerId);
    if (!runner.has_value()) {
        emit launchError("Runner not found: " + QString::fromStdString(config.runnerId));
        return;
    }

    rocklaunch::LaunchCommand launchCmd = rocklaunch::BuildLaunchCommand(config, *runner, m_gameProfile);

    std::vector<std::string> warnings = rocklaunch::EnsurePrefix(config.prefixDir, *runner);
    for (const std::string &warning : warnings) {
        qWarning() << "Warning:" << QString::fromStdString(warning);
    }

    QString program = QString::fromStdString(launchCmd.command.front());
    QStringList arguments;
    for (size_t i = 1; i < launchCmd.command.size(); ++i) {
        arguments.append(QString::fromStdString(launchCmd.command[i]));
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const std::string &variable : launchCmd.environment) {
        std::size_t separator = variable.find('=');
        if (separator != std::string::npos) {
            QString key = QString::fromStdString(variable.substr(0, separator));
            QString value = QString::fromStdString(variable.substr(separator + 1));
            env.insert(key, value);
        }
    }

    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(QString::fromStdString(launchCmd.workingDirectory.string()));
    m_process.start(program, arguments);
}
