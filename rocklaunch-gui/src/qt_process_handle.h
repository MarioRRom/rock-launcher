#pragma once

#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/launch_session.h"

#include <QObject>
#include <QProcess>

// Qt-backed implementation of the core IProcessHandle contract. Owns a QProcess
// (this object — never moved across threads) and re-emits its lifecycle signals
// so the LaunchController can drive the LaunchSession state machine.
class QtProcessHandle final : public QObject, public rocklaunch::IProcessHandle
{
    Q_OBJECT

public:
    explicit QtProcessHandle(QObject *parent = nullptr);
    ~QtProcessHandle() override;

    // Applies program, arguments, working directory and environment from
    // command onto process. Returns false for an empty command.
    static bool Configure(QProcess &process, const rocklaunch::LaunchCommand &command);

    // Spawns the game. Missing binaries are reported asynchronously through
    // QProcess::errorOccurred, so true means "spawn requested" (per the core
    // contract async failures arrive later via OnSpawnFailed).
    bool Start(const rocklaunch::LaunchCommand &command) override;
    int ProcessId() const override;
    bool IsRunning() const override;
    void Terminate() override; // SIGTERM
    void Kill() override;      // SIGKILL
    // nullopt while the process is still running.
    std::optional<rocklaunch::ExitInfo> PollExit() override;

signals:
    void started();                                // QProcess::started
    void spawnFailed(const QString &reason);       // QProcess::errorOccurred(FailedToStart)
    void exited(const rocklaunch::ExitInfo &exit); // QProcess::finished

private:
    QProcess m_process;
};