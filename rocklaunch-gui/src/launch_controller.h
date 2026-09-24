#pragma once

#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/launch_session.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/runners/runner_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include <memory>
#include <optional>
#include <vector>

class ProfileModel;

// Drives a launch from core's LaunchSession state machine. The GUI keeps its
// event loop running: prefix commands and the game itself are QProcesses, and
// every session transition is triggered by a Qt signal. QML only reads
// launchState and calls launch()/stop().
class LaunchController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int launchState READ launchState NOTIFY launchStateChanged)
    Q_PROPERTY(QString statusDetail READ statusDetail NOTIFY launchStateChanged)

public:
    explicit LaunchController(QObject *parent);

    static LaunchController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(LaunchController *instance)
    {
        s_instance = instance;
    }

    void SetProfileManager(rocklaunch::ProfileManager *manager);
    void SetProfileModel(ProfileModel *model);

    // Core SessionState as int so QML can compare without exposing the enum
    // (Idle=0, PreparingPrefix=1, Starting=2, Running=3, Finished=4, Error=5).
    int launchState() const;
    // Last state detail, e.g. InterpretExit diagnostics on Finished.
    QString statusDetail() const;

    // Launches the current profile, or stops the game while one is running.
    Q_INVOKABLE void launch();
    // SIGTERM, then wineserver -k, escalating to SIGKILL after kStopGrace.
    Q_INVOKABLE void stop();

signals:
    void launchStateChanged();
    void launchError(const QString &message);

private:
    void StartLaunch();
    void RunNextPrefixCommand();
    void StartGame();

    rocklaunch::ProfileManager *m_profiles = nullptr;
    ProfileModel *m_profileModel = nullptr;
    rocklaunch::RunnerManager m_runnerManager;
    rocklaunch::Rocksmith2014RemasteredProfile m_gameProfile;
    rocklaunch::LaunchSession m_session;

    // Prefix setup commands (BuildPrefixCommands) run one QProcess at a time;
    // the game itself lives inside the QtProcessHandle owned by the session.
    QProcess m_prefixProcess;
    std::vector<rocklaunch::LaunchCommand> m_prefixCommands;
    QProcess m_wineKillProcess;
    QTimer m_stopTimer;

    rocklaunch::ProfileConfig m_pendingProfile;
    std::optional<rocklaunch::Runner> m_pendingRunner;
    QString m_statusDetail;
    inline static LaunchController *s_instance = nullptr;
};
