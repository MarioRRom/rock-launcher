#include "game_profile_model.h"
#include "launch_controller.h"
#include "profile_model.h"
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

    GameProfileModel gameProfileModel;
    gameProfileModel.setGameId(QString::fromStdString(rs2014.Id()));

    ProfileModel profileModel;
    profileModel.SetProfileManager(&profileManager);
    profileModel.SetGameProfileModel(&gameProfileModel);

    LaunchController launchController;
    launchController.SetProfileManager(&profileManager);
    launchController.SetProfileModel(&profileModel);

    Theme theme;
    Theme::setInstance(&theme);

    // Backend test hook until the theme selector lands; ROCKLAUNCH_THEME
    // overrides the flavor at startup (unknown values → mocha).
    const QString themeOverride = qEnvironmentVariable("ROCKLAUNCH_THEME");
    if (!themeOverride.isEmpty()) {
        theme.setFlavor(themeOverride);
    }

    qmlRegisterSingletonInstance("RockLaunch.Gui", 1, 0, "GameProfileModel", &gameProfileModel);
    qmlRegisterSingletonInstance("RockLaunch.Gui", 1, 0, "ProfileModel", &profileModel);
    qmlRegisterSingletonInstance("RockLaunch.Gui", 1, 0, "LaunchController", &launchController);
    qmlRegisterSingletonInstance("RockLaunch.Gui", 1, 0, "Theme", &theme);

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
