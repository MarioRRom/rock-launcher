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

// QML-visible mirror of core's SessionState, so QML compares against
// LaunchState.Running instead of a bare integer. A namespace, not a gadget: a
// value type would have to be named in lowercase, and these are constants.
namespace LaunchStates {
Q_NAMESPACE
QML_NAMED_ELEMENT(LaunchState)

enum State : int
{
    Idle = static_cast<int>(rocklaunch::SessionState::Idle),
    PreparingPrefix = static_cast<int>(rocklaunch::SessionState::PreparingPrefix),
    Starting = static_cast<int>(rocklaunch::SessionState::Starting),
    Running = static_cast<int>(rocklaunch::SessionState::Running),
    Finished = static_cast<int>(rocklaunch::SessionState::Finished),
    Error = static_cast<int>(rocklaunch::SessionState::Error),
};
Q_ENUM_NS(State)
}

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
    explicit LaunchController(rocklaunch::ProfileManager *manager,
                              ProfileModel *profileModel,
                              rocklaunch::RunnerManager *runners,
                              QObject *parent = nullptr);

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

    // Core SessionState as int; compare it against the LaunchState enum.
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
    // Shared with RunnerModel: one discovery pass.
    rocklaunch::RunnerManager *m_runners = nullptr;
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
