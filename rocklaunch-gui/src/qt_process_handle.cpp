#include "qt_process_handle.h"

#include <QProcessEnvironment>
#include <QStringList>

QtProcessHandle::QtProcessHandle(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started, this, &QtProcessHandle::started);

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit spawnFailed(m_process.errorString());
        }
    });

    connect(&m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        std::optional<rocklaunch::ExitInfo> exit = PollExit();
        if (exit.has_value()) {
            emit exited(*exit);
        }
    });
}

QtProcessHandle::~QtProcessHandle() = default;

bool QtProcessHandle::Configure(QProcess &process, const rocklaunch::LaunchCommand &command)
{
    if (command.command.empty()) {
        return false;
    }
    process.setProgram(QString::fromStdString(command.command.front()));
    QStringList arguments;
    for (std::size_t i = 1; i < command.command.size(); ++i) {
        arguments.append(QString::fromStdString(command.command[i]));
    }
    process.setArguments(arguments);
    process.setWorkingDirectory(QString::fromStdString(command.workingDirectory.string()));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const std::string &variable : command.environment) {
        std::size_t separator = variable.find('=');
        if (separator != std::string::npos) {
            env.insert(QString::fromStdString(variable.substr(0, separator)),
                       QString::fromStdString(variable.substr(separator + 1)));
        }
    }
    process.setProcessEnvironment(env);
    return true;
}

bool QtProcessHandle::Start(const rocklaunch::LaunchCommand &command)
{
    if (!Configure(m_process, command)) {
        return false;
    }
    m_process.start();
    return true;
}

int QtProcessHandle::ProcessId() const
{
    return static_cast<int>(m_process.processId());
}

bool QtProcessHandle::IsRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void QtProcessHandle::Terminate()
{
    // SIGTERM — the session escalates to Kill() after kStopGrace if needed.
    m_process.terminate();
}

void QtProcessHandle::Kill()
{
    m_process.kill();
}

std::optional<rocklaunch::ExitInfo> QtProcessHandle::PollExit()
{
    if (m_process.state() != QProcess::NotRunning) {
        return std::nullopt;
    }
    rocklaunch::ExitInfo exit;
    exit.signaled = m_process.exitStatus() == QProcess::CrashExit;
    exit.code = m_process.exitCode();
    // QProcess only exposes 128 + signal on a signal death; that code is enough
    // for InterpretExit's diagnostics (the exact signal number is not exposed).
    exit.signal = exit.signaled ? m_process.exitCode() - 128 : 0;
    return exit;
}