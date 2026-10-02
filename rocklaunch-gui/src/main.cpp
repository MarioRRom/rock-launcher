#include "game_profile_model.h"
#include "launch_controller.h"
#include "profile_model.h"
#include "runners/runner_jobs.h"
#include "runners/runner_model.h"
#include "utils/dialog_controller.h"
#include "utils/theme.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

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

    GameProfileModel gameProfileModel(nullptr);
    gameProfileModel.setGameId(QString::fromStdString(rs2014.Id()));
    GameProfileModel::setInstance(&gameProfileModel);

    ProfileModel profileModel(&profileManager, &gameProfileModel, nullptr);
    ProfileModel::setInstance(&profileModel);

    LaunchController launchController(&profileManager, &profileModel, &rs2014, nullptr);
    LaunchController::setInstance(&launchController);

    RunnerJobs runnerJobs(nullptr);
    RunnerJobs::setInstance(&runnerJobs);

    RunnerModel runnerModel(&runnerJobs, nullptr);
    RunnerModel::setInstance(&runnerModel);

    // RunnerModel connects above, so the first scan may only start after it or
    // its rows would arrive with nobody listening.
    runnerJobs.refresh(false);

    Theme theme(nullptr);
    Theme::setInstance(&theme);

    DialogController dialogController(nullptr);
    DialogController::setInstance(&dialogController);

    QObject::connect(&profileModel, &ProfileModel::failed, &dialogController, &DialogController::showError);
    QObject::connect(
        &runnerJobs,
        &RunnerJobs::errorChanged,
        &dialogController,
        [&dialogController, &runnerJobs]() {
            if (!runnerJobs.error().isEmpty()) {
                dialogController.showError(runnerJobs.error());
            }
        }
    );

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
