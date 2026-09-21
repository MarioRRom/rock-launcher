#include "rocklaunch/core/launch_session.h"

#include "rocklaunch/core/logger.h"

#include <csignal>
#include <utility>

namespace rocklaunch
{

namespace
{

const char *SignalName(int signal)
{
    switch (signal) {
    case SIGHUP: return "SIGHUP";
    case SIGINT: return "SIGINT";
    case SIGQUIT: return "SIGQUIT";
    case SIGILL: return "SIGILL";
    case SIGTRAP: return "SIGTRAP";
    case SIGABRT: return "SIGABRT";
    case SIGBUS: return "SIGBUS";
    case SIGFPE: return "SIGFPE";
    case SIGKILL: return "SIGKILL";
    case SIGUSR1: return "SIGUSR1";
    case SIGSEGV: return "SIGSEGV";
    case SIGUSR2: return "SIGUSR2";
    case SIGPIPE: return "SIGPIPE";
    case SIGALRM: return "SIGALRM";
    case SIGTERM: return "SIGTERM";
    default: return "unknown";
    }
}

} // namespace

std::string SessionStateName(SessionState state)
{
    switch (state) {
    case SessionState::Idle: return "Idle";
    case SessionState::PreparingPrefix: return "PreparingPrefix";
    case SessionState::Starting: return "Starting";
    case SessionState::Running: return "Running";
    case SessionState::Finished: return "Finished";
    case SessionState::Error: return "Error";
    }
    return "Unknown";
}

LaunchSession::LaunchSession(StateCallback onState)
    : m_onState(std::move(onState))
{
}

SessionState LaunchSession::State() const
{
    return m_state;
}

int LaunchSession::ProcessId() const
{
    return m_handle != nullptr ? m_handle->ProcessId() : 0;
}

void LaunchSession::Begin()
{
    if (m_state != SessionState::Idle
        && m_state != SessionState::Finished
        && m_state != SessionState::Error) {
        return;
    }
    SetState(SessionState::PreparingPrefix, "preparing WINEPREFIX");
}

bool LaunchSession::Start(const LaunchCommand &command, std::unique_ptr<IProcessHandle> handle)
{
    // Never disturb a session whose process is already in flight.
    if (m_state == SessionState::Starting || m_state == SessionState::Running) {
        return false;
    }
    // After Finished/Error the session must be Reset before launching again.
    if (m_state != SessionState::Idle && m_state != SessionState::PreparingPrefix) {
        return false;
    }

    if (handle == nullptr) {
        SetState(SessionState::Error, "start called without a process handle");
        return false;
    }
    // An empty command would crash implementations that dereference the first
    // token (QProcess::setProgram); fail with a clear state instead.
    if (command.command.empty()) {
        SetState(SessionState::Error, "launch command is empty");
        return false;
    }

    m_handle = std::move(handle);
    SetState(SessionState::Starting, "spawning game process");
    if (!m_handle->Start(command)) {
        SetState(SessionState::Error, "failed to spawn the game process");
        return false;
    }
    return true;
}

void LaunchSession::MarkRunning()
{
    if (m_state != SessionState::Starting) {
        return;
    }
    SetState(SessionState::Running,
             "game process running (pid " + std::to_string(ProcessId()) + ")");
}

void LaunchSession::OnSpawnFailed(const std::string &reason)
{
    if (m_state != SessionState::Starting) {
        return;
    }
    SetState(SessionState::Error, reason.empty() ? "game process failed to start" : reason);
}

void LaunchSession::OnExited(const ExitInfo &exit)
{
    if (m_state != SessionState::Starting && m_state != SessionState::Running) {
        return;
    }
    SetState(SessionState::Finished, InterpretExit(exit));
}

void LaunchSession::Terminate()
{
    if (m_handle == nullptr
        || (m_state != SessionState::Starting && m_state != SessionState::Running)) {
        return;
    }
    m_handle->Terminate();
}

void LaunchSession::Kill()
{
    if (m_handle == nullptr
        || (m_state != SessionState::Starting && m_state != SessionState::Running)) {
        return;
    }
    m_handle->Kill();
}

std::string LaunchSession::InterpretExit(const ExitInfo &exit)
{
    if (exit.signaled) {
        return "killed by signal " + std::to_string(exit.signal)
               + " (" + SignalName(exit.signal) + ")";
    }

    switch (exit.code) {
    case 0:
        return "exited cleanly";
    case 127:
        return "exited with code 127 — a shared library is missing (check the game log)";
    case 1:
        return "exited with code 1 — possible wineserver conflict or a game error";
    default:
        return "exited with code " + std::to_string(exit.code);
    }
}

void LaunchSession::Reset()
{
    m_handle.reset();
    SetState(SessionState::Idle, "");
}

void LaunchSession::SetState(SessionState state, const std::string &detail)
{
    m_state = state;
    if (m_onState != nullptr) {
        m_onState(state, detail);
    }

    Logger logger;
    logger.Debug("LaunchSession: state " + SessionStateName(state)
                 + (detail.empty() ? "" : " — " + detail));
}

} // namespace rocklaunch