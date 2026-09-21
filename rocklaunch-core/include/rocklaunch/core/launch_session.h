#pragma once

#include "rocklaunch/core/launch.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rocklaunch
{

// High-level lifecycle of a launched game.
//
// The CLI launches by replacing itself (ExecLaunchCommand, execvp) and does not
// need a session: its process IS the game. This API targets frontends that must
// stay alive and own the child process (the Qt GUI), plus the headless tests.
enum class SessionState
{
    Idle,            // nothing in flight / session reset
    PreparingPrefix, // prefix setup commands being applied by the frontend
    Starting,        // spawn requested, child not confirmed alive yet
    Running,         // child confirmed alive (playing)
    Finished,        // child exited or was stopped
    Error,           // could not start
};

// Human-readable name of a session state.
std::string SessionStateName(SessionState state);

// Why and how the launched process ended.
struct ExitInfo
{
    bool signaled = false; // terminated by a signal
    int code = 0;          // exit status, or 128 + signal when signaled
    int signal = 0;        // terminating signal number when signaled
};

// Abstraction over the concrete process the frontend uses. Core stays Qt-free:
// the GUI supplies a QProcess-backed implementation, tests supply a fake.
class IProcessHandle
{
public:
    virtual ~IProcessHandle() = default;

    // Spawns the game with the given command. False when the spawn could not be
    // requested at all (asynchronous failures arrive later via OnSpawnFailed).
    virtual bool Start(const LaunchCommand &command) = 0;
    // PID of the child, or 0 before it has spawned.
    virtual int ProcessId() const = 0;
    virtual bool IsRunning() const = 0;
    // SIGTERM.
    virtual void Terminate() = 0;
    // SIGKILL.
    virtual void Kill() = 0;
    // Exit information once the child is no longer running, nullopt otherwise.
    // The frontend calls this (e.g. from a QProcess::finished handler) and feeds
    // the result to LaunchSession::OnExited(). LaunchSession never polls it
    // internally: the session stays thread-free and leaves event-loop driving
    // to the frontend, so this method exists for the frontend, not for the
    // session.
    virtual std::optional<ExitInfo> PollExit() = 0;
};

// State machine and stop policy for one game launch. Qt-free and thread-free:
// the frontend drives every transition from its own event loop.
//
// Flow:
//   Begin()                     -> PreparingPrefix (frontend applies BuildPrefixCommands)
//   Start(cmd, handle)          -> Starting, then Running once MarkRunning() is called
//   MarkRunning()               -> Running (playing)
//   OnExited(exit)              -> Finished, where exit comes from handle->PollExit()
//   Stop: Terminate(), wait kStopGrace, Kill() if still alive; for Wine runners
//                                         also run BuildWineKillCommand().
class LaunchSession final
{
public:
    using StateCallback = std::function<void(SessionState state, const std::string &detail)>;

    explicit LaunchSession(StateCallback onState = {});

    SessionState State() const;
    int ProcessId() const;

    // Starts the prefix phase. Only valid from Idle, Finished or Error.
    void Begin();

    // Spawns the game process, taking ownership of the handle. Valid from Idle
    // (prefix skipped) or after Begin(). Returns false without changing state
    // when a launch is already in flight or the session is not reusable (call
    // Reset() after Finished/Error to launch again). Moves to Error and returns
    // false when the handle is missing, the command is empty, or the spawn
    // could not even be requested (asynchronous failures arrive via OnSpawnFailed).
    bool Start(const LaunchCommand &command, std::unique_ptr<IProcessHandle> handle);

    // The child is confirmed alive (e.g. QProcess::started). Starting -> Running.
    void MarkRunning();

    // The child failed to start asynchronously (e.g. QProcess FailedToStart).
    // Starting -> Error.
    void OnSpawnFailed(const std::string &reason);

    // The child exited. Starting/Running -> Finished.
    void OnExited(const ExitInfo &exit);

    // SIGTERM, only while Starting or Running. The frontend escalates to Kill()
    // when the child is still alive after kStopGrace.
    void Terminate();
    // SIGKILL, only while Starting or Running.
    void Kill();

    // Grace period between Terminate() and Kill() during a stop.
    static constexpr std::chrono::milliseconds kStopGrace{ 5000 };

    // Human-readable diagnostic for an exit (127 -> missing shared library, ...).
    static std::string InterpretExit(const ExitInfo &exit);

    // Back to Idle and releases the handle, so the session can be reused.
    void Reset();

private:
    void SetState(SessionState state, const std::string &detail);

    StateCallback m_onState;
    SessionState m_state = SessionState::Idle;
    std::unique_ptr<IProcessHandle> m_handle;
};

} // namespace rocklaunch