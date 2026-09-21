// Core unit tests for LaunchSession and the launch command builders.
// Exercised through ctest as rocklaunch-core-launch-session: the session has no
// CLI surface, so it runs as a small core unit binary instead of a cmake script.
// STATE ISOLATED: the test root directory is passed as argv[1] and HOME /
// XDG_* are pointed at it, exactly like the CLI test scripts.

#include "rocklaunch/core/launch.h"
#include "rocklaunch/core/launch_session.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{

namespace fs = std::filesystem;

int gChecks = 0;
int gFailures = 0;

void Check(bool condition, const std::string &what)
{
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::cerr << "FAIL: " << what << '\n';
    }
}

class FakeProcessHandle final : public rocklaunch::IProcessHandle
{
public:
    bool startResult = true;
    bool started = false;
    int pid = 4242;
    int terminateCalls = 0;
    int killCalls = 0;
    rocklaunch::LaunchCommand lastCommand;

    bool Start(const rocklaunch::LaunchCommand &command) override
    {
        lastCommand = command;
        started = startResult;
        return startResult;
    }

    int ProcessId() const override
    {
        return started ? pid : 0;
    }

    bool IsRunning() const override
    {
        return started;
    }

    void Terminate() override
    {
        ++terminateCalls;
    }

    void Kill() override
    {
        ++killCalls;
    }

    std::optional<rocklaunch::ExitInfo> PollExit() override
    {
        return std::nullopt;
    }
};

// A command the fake handle accepts; LaunchSession rejects empty commands.
rocklaunch::LaunchCommand GameCommand()
{
    rocklaunch::LaunchCommand command;
    command.command = { "fake-game" };
    return command;
}

// --- LaunchSession state machine --------------------------------------------

void TestSessionFlow()
{
    std::vector<std::string> states;
    std::vector<std::string> details;
    rocklaunch::LaunchSession session(
        [&states, &details](rocklaunch::SessionState state, const std::string &detail) {
            states.push_back(rocklaunch::SessionStateName(state));
            details.push_back(detail);
        });

    Check(session.State() == rocklaunch::SessionState::Idle, "fresh session is Idle");
    Check(session.ProcessId() == 0, "no pid before spawn");

    // Control calls on a fresh session are no-ops.
    session.Terminate();
    session.Kill();
    Check(session.State() == rocklaunch::SessionState::Idle, "terminate/kill on Idle are no-ops");

    auto handle = std::make_unique<FakeProcessHandle>();
    FakeProcessHandle *raw = handle.get();

    session.Begin();
    Check(session.State() == rocklaunch::SessionState::PreparingPrefix, "Begin -> PreparingPrefix");

    rocklaunch::LaunchCommand command;
    command.command = { "fake-game" };
    Check(session.Start(command, std::move(handle)), "Start accepted");
    Check(session.State() == rocklaunch::SessionState::Starting, "Start -> Starting");
    Check(raw->started, "handle spawned");
    Check(raw->lastCommand.command == command.command, "handle received the command");
    Check(session.ProcessId() == raw->pid, "pid exposed after spawn");

    session.MarkRunning();
    Check(session.State() == rocklaunch::SessionState::Running, "MarkRunning -> Running");

    session.OnExited({ false, 0, 0 });
    Check(session.State() == rocklaunch::SessionState::Finished, "OnExited -> Finished");

    const std::vector<std::string> expected = { "PreparingPrefix", "Starting", "Running", "Finished" };
    Check(states == expected, "state callback sequence");
    Check(details.back().find("exited cleanly") != std::string::npos, "exit detail reported");
}

// A fast wine wrapper may die right after spawn, before the frontend confirms it
// is running: Starting -> Finished directly, with no Running in between.
void TestExitBeforeMarkRunning()
{
    std::vector<std::string> states;
    std::vector<std::string> details;
    rocklaunch::LaunchSession session(
        [&states, &details](rocklaunch::SessionState state, const std::string &detail) {
            states.push_back(rocklaunch::SessionStateName(state));
            details.push_back(detail);
        });

    auto handle = std::make_unique<FakeProcessHandle>();
    session.Begin();
    Check(session.Start(GameCommand(), std::move(handle)), "Start accepted");
    Check(session.State() == rocklaunch::SessionState::Starting, "Starting before running confirmation");

    session.OnExited({ false, 1, 0 }); // wrapper died during startup
    Check(session.State() == rocklaunch::SessionState::Finished, "Starting -> Finished without MarkRunning");

    const std::vector<std::string> expected = { "PreparingPrefix", "Starting", "Finished" };
    Check(states == expected, "fast death skips the Running transition");
    Check(details.back().find("exited with code 1") != std::string::npos, "exit detail reported");
}

void TestSpawnFailures()
{
    // Synchronous failure.
    rocklaunch::LaunchSession session;
    auto handle = std::make_unique<FakeProcessHandle>();
    handle->startResult = false;
    session.Begin();
    Check(!session.Start(GameCommand(), std::move(handle)), "sync spawn failure rejected");
    Check(session.State() == rocklaunch::SessionState::Error, "sync spawn failure -> Error");

    // Asynchronous failure (e.g. QProcess FailedToStart).
    rocklaunch::LaunchSession asyncSession;
    auto asyncHandle = std::make_unique<FakeProcessHandle>();
    asyncSession.Begin();
    Check(asyncSession.Start(GameCommand(), std::move(asyncHandle)), "async-capable spawn accepted");
    asyncSession.OnSpawnFailed("FailedToStart");
    Check(asyncSession.State() == rocklaunch::SessionState::Error, "spawn failure -> Error");

    // Missing handle.
    rocklaunch::LaunchSession noHandleSession;
    noHandleSession.Begin();
    Check(!noHandleSession.Start(GameCommand(), nullptr), "Start without handle rejected");
    Check(noHandleSession.State() == rocklaunch::SessionState::Error, "no-handle -> Error");

    // Empty command (would crash a real handle dereferencing command.front()).
    rocklaunch::LaunchSession emptySession;
    auto emptyHandle = std::make_unique<FakeProcessHandle>();
    emptySession.Begin();
    rocklaunch::LaunchCommand emptyCommand;
    Check(!emptySession.Start(emptyCommand, std::move(emptyHandle)), "empty command rejected");
    Check(emptySession.State() == rocklaunch::SessionState::Error, "empty command -> Error");
}

void TestStopAndReuse()
{
    rocklaunch::LaunchSession session;
    auto handle = std::make_unique<FakeProcessHandle>();
    FakeProcessHandle *raw = handle.get();
    session.Begin();
    session.Start(GameCommand(), std::move(handle));
    session.MarkRunning();

    session.Terminate();
    Check(raw->terminateCalls == 1, "Terminate forwarded to handle");
    Check(session.State() == rocklaunch::SessionState::Running, "state stays Running after Terminate");

    session.Kill();
    Check(raw->killCalls == 1, "Kill forwarded to handle");

    session.OnExited({ true, 137, 9 }); // killed by SIGKILL
    Check(session.State() == rocklaunch::SessionState::Finished, "exited -> Finished");

    // After Finished, control is a no-op and a new launch needs Reset().
    session.Terminate();
    Check(raw->terminateCalls == 1, "Terminate after Finished is a no-op");

    auto second = std::make_unique<FakeProcessHandle>();
    Check(!session.Start(GameCommand(), std::move(second)), "Start after Finished without Reset rejected");
    Check(session.State() == rocklaunch::SessionState::Finished, "state unchanged after rejected Start");

    session.Reset();
    Check(session.State() == rocklaunch::SessionState::Idle, "Reset -> Idle");

    auto third = std::make_unique<FakeProcessHandle>();
    session.Begin();
    Check(session.Start(GameCommand(), std::move(third)), "session reusable after Reset");
    Check(session.State() == rocklaunch::SessionState::Starting, "reused session spawns");

    // Double start while in flight is rejected without changing state.
    auto fourth = std::make_unique<FakeProcessHandle>();
    Check(!session.Start(GameCommand(), std::move(fourth)), "double Start rejected");
    Check(session.State() == rocklaunch::SessionState::Starting, "double Start keeps state");
}

void TestInterpretExit()
{
    Check(rocklaunch::LaunchSession::InterpretExit({ false, 0, 0 }) == "exited cleanly",
          "exit 0");

    const std::string missingLibrary = rocklaunch::LaunchSession::InterpretExit({ false, 127, 0 });
    Check(missingLibrary.find("shared library") != std::string::npos,
          "exit 127 -> shared library hint");

    const std::string wineConflict = rocklaunch::LaunchSession::InterpretExit({ false, 1, 0 });
    Check(wineConflict.find("wineserver") != std::string::npos, "exit 1 -> wineserver hint");

    const std::string signal = rocklaunch::LaunchSession::InterpretExit({ true, 137, 9 });
    Check(signal.find("SIGKILL") != std::string::npos, "signal 9 -> SIGKILL");

    const std::string other = rocklaunch::LaunchSession::InterpretExit({ false, 42, 0 });
    Check(other.find("42") != std::string::npos, "unknown code reported");

    Check(rocklaunch::SessionStateName(rocklaunch::SessionState::Running) == "Running",
          "SessionStateName");
    Check(rocklaunch::LaunchSession::kStopGrace.count() == 5000, "stop grace constant");
}

// --- Prefix / wine kill command builders ------------------------------------

void WriteScript(const fs::path &path, const std::string &body)
{
    std::ofstream output(path);
    output << body;
    output.close();
    std::error_code error;
    fs::permissions(path, fs::perms::owner_all, error);
}

void TestBuildPrefixCommands(const fs::path &testRoot)
{
    // Wine runner: the binary is the runner executable itself.
    fs::path wineDir = testRoot / "runners" / "wine";
    fs::create_directories(wineDir);
    WriteScript(wineDir / "wine", "#!/bin/sh\nexit 0\n");

    rocklaunch::Runner wineRunner;
    wineRunner.type = rocklaunch::RunnerType::Wine;
    wineRunner.executable = wineDir / "wine";

    std::vector<rocklaunch::LaunchCommand> commands =
        rocklaunch::BuildPrefixCommands(testRoot / "prefix", wineRunner);
    Check(commands.size() == 2, "two Audio=alsa registry commands");
    Check(!commands[0].command.empty()
              && commands[0].command[0] == (wineDir / "wine").string(),
          "wine binary is the first argument");
    bool hasAudio = false;
    for (const std::string &token : commands[0].command) {
        hasAudio = hasAudio || token == "Audio";
    }
    Check(hasAudio, "registry value Audio present");
    Check(commands[0].environment.size() == 1
              && commands[0].environment[0] == "WINEPREFIX=" + (testRoot / "prefix").string(),
          "wine prefix env");

    // Proton runner: the wine binary is probed under files/.
    fs::path protonDir = testRoot / "compatibilitytools.d" / "GE-Proton";
    fs::create_directories(protonDir / "files" / "bin");
    WriteScript(protonDir / "files" / "bin" / "wine", "#!/bin/sh\nexit 0\n");

    rocklaunch::Runner protonRunner;
    protonRunner.type = rocklaunch::RunnerType::Proton;
    protonRunner.rootDir = protonDir;

    std::vector<rocklaunch::LaunchCommand> protonCommands =
        rocklaunch::BuildPrefixCommands(testRoot / "prefix-proton", protonRunner);
    Check(protonCommands.size() == 2, "proton prefix commands");
    Check(protonCommands[0].environment[0]
              == "WINEPREFIX=" + (testRoot / "prefix-proton" / "pfx").string(),
          "proton uses the pfx subdirectory");

    // Missing wine binary -> no commands.
    rocklaunch::Runner emptyRunner;
    emptyRunner.type = rocklaunch::RunnerType::Wine;
    emptyRunner.executable = testRoot / "missing" / "wine";
    Check(rocklaunch::BuildPrefixCommands(testRoot / "prefix-empty", emptyRunner).empty(),
          "no commands without wine binary");
}

void TestBuildWineKillCommand(const fs::path &testRoot)
{
    // Wine: wineserver sits next to the wine binary.
    fs::path wineDir = testRoot / "runners" / "wine-kill";
    fs::create_directories(wineDir);
    WriteScript(wineDir / "wine", "#!/bin/sh\nexit 0\n");
    WriteScript(wineDir / "wineserver", "#!/bin/sh\nexit 0\n");

    rocklaunch::Runner wineRunner;
    wineRunner.type = rocklaunch::RunnerType::Wine;
    wineRunner.executable = wineDir / "wine";

    std::optional<rocklaunch::LaunchCommand> killCommand =
        rocklaunch::BuildWineKillCommand(testRoot / "prefix", wineRunner);
    Check(killCommand.has_value(), "wine kill command found");
    if (killCommand.has_value()) {
        Check(killCommand->command.size() == 2 && killCommand->command[1] == "-k",
              "wineserver -k arguments");
        Check(killCommand->environment[0] == "WINEPREFIX=" + (testRoot / "prefix").string(),
              "wine kill env");
    }

    // Proton: wineserver under files/bin.
    fs::path protonDir = testRoot / "compatibilitytools.d" / "GE-Proton-kill";
    fs::create_directories(protonDir / "files" / "bin");
    WriteScript(protonDir / "files" / "bin" / "wineserver", "#!/bin/sh\nexit 0\n");

    rocklaunch::Runner protonRunner;
    protonRunner.type = rocklaunch::RunnerType::Proton;
    protonRunner.rootDir = protonDir;
    protonRunner.executable = protonDir / "proton";

    std::optional<rocklaunch::LaunchCommand> protonKill =
        rocklaunch::BuildWineKillCommand(testRoot / "prefix-proton", protonRunner);
    Check(protonKill.has_value(), "proton kill command found");
    if (protonKill.has_value()) {
        Check(protonKill->environment[0]
                  == "WINEPREFIX=" + (testRoot / "prefix-proton" / "pfx").string(),
              "proton kill uses the pfx subdirectory");
    }

    // No wineserver anywhere -> nullopt.
    rocklaunch::Runner bareRunner;
    bareRunner.type = rocklaunch::RunnerType::Wine;
    bareRunner.executable = testRoot / "missing-kill" / "wine";
    Check(!rocklaunch::BuildWineKillCommand(testRoot / "prefix-bare", bareRunner).has_value(),
          "no kill command without wineserver");
}

// --- EnsurePrefix keeps working (the CLI path) -------------------------------

void TestEnsurePrefix(const fs::path &testRoot)
{
    fs::path runnerDir = testRoot / "runners" / "ensure";
    fs::create_directories(runnerDir);
    WriteScript(runnerDir / "wine", "#!/bin/sh\nexit 0\n");

    rocklaunch::Runner wineRunner;
    wineRunner.type = rocklaunch::RunnerType::Wine;
    wineRunner.executable = runnerDir / "wine";

    fs::path prefix = testRoot / "prefix-ensure";
    std::vector<std::string> warnings = rocklaunch::EnsurePrefix(prefix, wineRunner);
    Check(warnings.empty(), "happy path produces no warnings");
    Check(fs::is_directory(prefix), "prefix directory created");

    // A failing registry write is reported, not fatal.
    fs::path failingDir = testRoot / "runners" / "ensure-fail";
    fs::create_directories(failingDir);
    WriteScript(failingDir / "wine", "#!/bin/sh\nexit 1\n");

    rocklaunch::Runner failingRunner;
    failingRunner.type = rocklaunch::RunnerType::Wine;
    failingRunner.executable = failingDir / "wine";

    std::vector<std::string> failWarnings =
        rocklaunch::EnsurePrefix(testRoot / "prefix-ensure-fail", failingRunner);
    Check(failWarnings.size() == 1
              && failWarnings[0].find("Audio=alsa failed") != std::string::npos,
          "failing registry write reported as warning");
}

} // namespace

int main(int argc, char **argv)
{
    fs::path testRoot = argc > 1 ? fs::path(argv[1])
                                 : fs::path("/tmp/rocklaunch-launch-session-test");

    std::error_code error;
    fs::remove_all(testRoot, error);
    fs::create_directories(testRoot / "home", error);
    fs::create_directories(testRoot / "data", error);
    fs::create_directories(testRoot / "config", error);

    // Isolate state exactly like the CLI test scripts: never touch the user's
    // real XDG directories.
    setenv("HOME", (testRoot / "home").string().c_str(), 1);
    setenv("XDG_DATA_HOME", (testRoot / "data").string().c_str(), 1);
    setenv("XDG_CONFIG_HOME", (testRoot / "config").string().c_str(), 1);

    TestSessionFlow();
    TestExitBeforeMarkRunning();
    TestSpawnFailures();
    TestStopAndReuse();
    TestInterpretExit();
    TestBuildPrefixCommands(testRoot);
    TestBuildWineKillCommand(testRoot);
    TestEnsurePrefix(testRoot);

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}