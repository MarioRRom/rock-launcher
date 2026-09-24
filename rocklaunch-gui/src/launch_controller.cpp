#include "launch_controller.h"

#include "profile_model.h"
#include "utils/qt_process_handle.h"

#include <QDebug>

LaunchController::LaunchController(QObject *parent)
    : QObject(parent)
    , m_runnerManager(rocklaunch::RunnerManager::CreateDefault())
    , m_session([this](rocklaunch::SessionState, const std::string &detail) {
        m_statusDetail = QString::fromStdString(detail);
        emit launchStateChanged();
    })
{
    // Prefix setup: advance through the command queue, then launch the game.
    connect(&m_prefixProcess, &QProcess::finished,
            this, [this](int exitCode, QProcess::ExitStatus) {
                if (exitCode != 0) {
                    qWarning() << "LaunchController: prefix setup command failed (exit"
                               << exitCode << ")";
                }
                RunNextPrefixCommand();
            });

    // Stop escalation: SIGKILL when the game is still alive after kStopGrace.
    m_stopTimer.setSingleShot(true);
    connect(&m_stopTimer, &QTimer::timeout, this, [this] {
        const rocklaunch::SessionState state = m_session.State();
        if (state == rocklaunch::SessionState::Starting
            || state == rocklaunch::SessionState::Running) {
            m_session.Kill();
        }
    });
}

void LaunchController::SetProfileManager(rocklaunch::ProfileManager *manager)
{
    m_profiles = manager;
}

void LaunchController::SetProfileModel(ProfileModel *model)
{
    m_profileModel = model;
}

int LaunchController::launchState() const
{
    return static_cast<int>(m_session.State());
}

QString LaunchController::statusDetail() const
{
    return m_statusDetail;
}

void LaunchController::launch()
{
    const rocklaunch::SessionState state = m_session.State();
    if (state == rocklaunch::SessionState::Starting
        || state == rocklaunch::SessionState::Running) {
        stop(); // toggle: the Play button stops the game while it is playing
        return;
    }
    if (state == rocklaunch::SessionState::PreparingPrefix) {
        return; // prefix setup in flight; ignore extra clicks
    }
    StartLaunch();
}

void LaunchController::StartLaunch()
{
    if (!m_profiles || !m_profileModel) {
        emit launchError("Controller not initialized");
        return;
    }

    // After Finished/Error the session must be reset before launching again.
    const rocklaunch::SessionState state = m_session.State();
    if (state == rocklaunch::SessionState::Finished || state == rocklaunch::SessionState::Error) {
        m_session.Reset();
    }

    QString profileId = m_profileModel->currentProfile();
    if (profileId.isEmpty()) {
        emit launchError("No profile selected");
        return;
    }

    // Pre-flight checks are core business rules shared with the CLI.
    const rocklaunch::ProfileValidation validation =
        m_profiles->ValidateProfile(profileId.toStdString(), m_runnerManager);
    if (!validation.isValid) {
        QString detail;
        for (const rocklaunch::ValidationIssue &issue : validation.issues) {
            detail = QString::fromStdString(issue.message);
            break;
        }
        m_statusDetail = detail;
        emit launchStateChanged();
        emit launchError(detail);
        return;
    }

    std::optional<rocklaunch::ProfileConfig> profile =
        m_profiles->GetProfile(profileId.toStdString());
    if (!profile.has_value()) {
        emit launchError("Profile not found: " + profileId);
        return;
    }
    m_pendingProfile = *profile;
    m_pendingRunner = m_runnerManager.Find(m_pendingProfile.runnerId);
    if (!m_pendingRunner.has_value()) {
        emit launchError("Runner not found: " + QString::fromStdString(m_pendingProfile.runnerId));
        return;
    }

    // Prefix setup runs asynchronously (one QProcess per command) so the UI
    // event loop keeps running while wine writes its registry keys.
    m_session.Begin(); // -> PreparingPrefix
    m_prefixCommands = rocklaunch::BuildPrefixCommands(m_pendingProfile.prefixDir, *m_pendingRunner);
    RunNextPrefixCommand();
}

void LaunchController::RunNextPrefixCommand()
{
    while (!m_prefixCommands.empty()) {
        rocklaunch::LaunchCommand command = std::move(m_prefixCommands.front());
        m_prefixCommands.erase(m_prefixCommands.begin());
        // Defensive: BuildPrefixCommands never yields empty commands.
        if (!QtProcessHandle::Configure(m_prefixProcess, command)) {
            continue;
        }
        m_prefixProcess.start();
        return; // continue on QProcess::finished
    }
    StartGame();
}

void LaunchController::StartGame()
{
    const rocklaunch::LaunchCommand launchCommand =
        rocklaunch::BuildLaunchCommand(m_pendingProfile, *m_pendingRunner, m_gameProfile);

    // Create the handle first so its Qt signals can drive the session, then hand
    // ownership to LaunchSession (the handle's QProcess lives as long as the session).
    auto handle = std::make_unique<QtProcessHandle>();
    QtProcessHandle *rawHandle = handle.get();

    connect(rawHandle, &QtProcessHandle::started, this, [this] {
        m_session.MarkRunning(); // Starting -> Running
    });
    connect(rawHandle, &QtProcessHandle::spawnFailed, this,
            [this](const QString &reason) {
                m_stopTimer.stop();
                m_session.OnSpawnFailed(reason.toStdString()); // Starting -> Error
            });
    connect(rawHandle, &QtProcessHandle::exited, this,
            [this](const rocklaunch::ExitInfo &exit) {
                m_stopTimer.stop();
                m_session.OnExited(exit); // Starting/Running -> Finished
            });

    // On synchronous failure the session moves to Error by itself; the state
    // callback already refreshed the QML side.
    m_session.Start(launchCommand, std::move(handle));
}

void LaunchController::stop()
{
    const rocklaunch::SessionState state = m_session.State();
    if (state != rocklaunch::SessionState::Starting
        && state != rocklaunch::SessionState::Running) {
        return;
    }

    m_session.Terminate(); // SIGTERM to the game

    // Cleanly shut the Wine/Proton prefix down (wineserver -k).
    if (m_pendingRunner.has_value()) {
        std::optional<rocklaunch::LaunchCommand> killCommand =
            rocklaunch::BuildWineKillCommand(m_pendingProfile.prefixDir, *m_pendingRunner);
        if (killCommand.has_value()
            && QtProcessHandle::Configure(m_wineKillProcess, *killCommand)) {
            m_wineKillProcess.start(); // fire and forget
        }
    }

    // Escalate to SIGKILL when the game is still alive after the grace period.
    m_stopTimer.start(rocklaunch::LaunchSession::kStopGrace.count());
}
