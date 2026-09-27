#include "game_profile_model.h"
#include "launch_controller.h"
#include "profile_model.h"
#include "runners/runner_model.h"
#include "utils/dialog_controller.h"
#include "utils/theme.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"
#include "rocklaunch/core/runners/runner_manager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("RockLauncher");
    app.setOrganizationName("RockLaunch");

    rocklaunch::ConfigStore configStore;
    rocklaunch::Rocksmith2014RemasteredProfile rs2014;
    rocklaunch::ProfileManager profileManager(configStore, rs2014);
    // One runner discovery for the whole app: LaunchController validates and
    // launches with it, RunnerModel lists it.
    rocklaunch::RunnerManager runnerManager(rocklaunch::RunnerManager::CreateDefault());

    GameProfileModel gameProfileModel(nullptr);
    gameProfileModel.setGameId(QString::fromStdString(rs2014.Id()));
    GameProfileModel::setInstance(&gameProfileModel);

    ProfileModel profileModel(&profileManager, &gameProfileModel, nullptr);
    ProfileModel::setInstance(&profileModel);

    LaunchController launchController(&profileManager, &profileModel, &runnerManager, nullptr);
    LaunchController::setInstance(&launchController);

    // Wired ahead of the runners page rework; the UI still uses a placeholder.
    RunnerModel runnerModel(&runnerManager, nullptr);
    RunnerModel::setInstance(&runnerModel);

    Theme theme(nullptr);
    Theme::setInstance(&theme);

    DialogController dialogController(nullptr);
    DialogController::setInstance(&dialogController);

    // Backend test hook until the theme selector lands; ROCKLAUNCH_THEME
    // overrides the flavor at startup (unknown values → mocha).
    const QString themeOverride = qEnvironmentVariable("ROCKLAUNCH_THEME");
    if (!themeOverride.isEmpty()) {
        theme.setFlavor(themeOverride);
    }

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.loadFromModule("RockLaunch.Gui", "Main");

    return app.exec();
}
