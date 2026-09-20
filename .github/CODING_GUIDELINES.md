# Rocksmith Launcher — Coding Guidelines

Keep the codebase readable, consistent, and easy to review. These are not meant to restrict contributors — they exist to keep the project coherent as it grows.

**English** is the default language: code, identifiers, comments, commit messages, and documentation.

---

## Architecture

The project is layered so every feature can be tested without a GUI:

```
rocklaunch-core/     Business logic. No Qt dependencies.
rocklaunch-cli/      Thin CLI exercising the core.
rocklaunch-gui/      Qt/QML frontend consuming the core.
```

Nothing outside the core implements game detection, runner handling, patching, or CDLC. Tests cover the core via the CLI.

**Key abstractions** (defined from phase 0):

- `IGameProfile` — behavior of a supported game (`Id`, `ValidateInstall`, `RequiredEnv`, `Executable`).
- `ILaunchPatch` — patch interface (`Id`, `GameId`, `Preset`, `IsEnabled`, `Apply`, `Remove`).
- `GameSource` — locates the game (`SteamSource`, `ManualSource`). No scattered conditional logic.
- `IRunnerSource` — discovers local Wine/Proton runners. `RunnerManager` merges sources.

**Profiles** (`ProfileConfig`) represent one installation of a game with its own path, runner, prefix, and patch settings. The same install directory cannot belong to more than one profile.

**Launcher reimplements** the methods of relevant projects at launcher level — no forks, no `launcher.exe -> game.exe` chains, no files required from the user. CDLC is the exception under evaluation (see `.github/CustomDLCPatch.md`). Inspiration credited in the README.

**Data paths:**

```
~/.config/rocksmith-launcher/
    config.json                        launcher-wide settings
                                       (future: runner defaults, colors, language)
~/.local/share/rocksmith-launcher/
    profiles/<profile_id>.json         profile state (game, runner, patches)
    prefixes/<profile_id>/             WINEPREFIX / STEAM_COMPAT_DATA_PATH
    runners/                           downloaded GE-Proton versions
    patches/                           cached downloadable patches
    logs/
```

---

## Config (JSON)

Per-installation configuration lives in `profiles/<profile_id>.json` (game ID, install path, runner, patches). Launcher-wide settings live in `config.json`. Keep keys descriptive and predictable; new settings are exposed through the CLI first.

---

## C++ Style

Follow the [Qt Group Coding Conventions](https://wiki.qt.io/Coding_Conventions). This project adds:

- Interfaces use the `I` prefix (`IGameProfile`, `ILaunchPatch`).
- Concrete leaf classes use `final` (`Rocksmith2014RemasteredProfile final : public IGameProfile`).
- Private members use the `m_` prefix. Struct fields use bare names.
- Use `std::filesystem` (`fs`) for paths. Declare `namespace fs = std::filesystem;` in headers.
- `#pragma once` as include guard.

### Includes

Order: own header first, then project headers, then system / third-party. Each group separated by a blank line. No unused includes.

### Naming

Files and directories: `snake_case`. Classes and types: `PascalCase`. Methods: `PascalCase`. Constants: `constexpr` with `k` prefix (`kMaxLogSize`, `kEnablerRepo`). One declaration per line.

### Error Handling

- `std::runtime_error` for recoverable errors, `std::invalid_argument` for bad input.
- `std::error_code` overloads for non-critical filesystem checks.
- `std::optional` for nullable return values. Output parameters (`std::string &error`) for graceful error reporting.
- Core never catches its own exceptions; the CLI catches at the top level.

### Logging

`Logger` is instantiated as a stack local per function. Format: `"ComponentName: description"`. Levels: `Debug` (trace, file-excluded), `Info` (milestones), `Warn` (non-fatal), `Error` (failures).

### Memory

`std::unique_ptr` for ownership. Raw pointers for non-owning access. No `std::shared_ptr`. Use `std::move` only for by-value parameters being stored.

### Data vs Behavior

Structs for plain data with aggregate initialization. Classes for behavior with constructors and methods.

### File Writes

Atomic writes (`.tmp` + `fs::rename()`) for files that must not be corrupted on failure.

### Comments

Comments explain intent, not obvious implementation details. Assume the reader already understands the language syntax.

**Good** — explains why:

```cpp
// Only the expected files matter; the launcher does not care how the install was obtained.
bool Rocksmith2014RemasteredProfile::ValidateInstall(const fs::path &installDir) const
```

**Bad** — restates what the code does:

```cpp
// Increment counter by one
counter++
```

For compact or minor sections, use a simple separator with a blank line above:

```cpp
// Profile commands

// Path assignment
```

---

## Qt / QML Style

### Project Structure

```
rocklaunch-gui/
├── src/
└── qml/
    ├── Main.qml                Window root, layout composition
    ├── layout/                 Structural chrome (SideBar, TopBar, Content)
    ├── contents/               Page-level views (launch, runners, patches...)
    ├── components/             Reusable generic UI (buttons, icons, inputs)
    └── assets/                 Static resources (icons, logos)
```

### File Header

Every `.qml` file starts with the project banner, followed by two blank lines:

```qml
//--------------------------------------------------------------
//  ____   ___   ____ _  ___        _   _   _ _   _  ____ _   _ 
// |  _ \ / _ \ / ___| |/ / |      / \ | | | \ | |/ ___| | | |
// | |_) | | | | |   | ' /| |     / _ \| | | |  \| | |   | |_| |
// |  _ <| |_| | |___| . \| |___ / ___ \ |_| | |\  | |___|  _  |
// |_| \_\\___/ \____|_|\_\_____/_/   \_\___/|_| \_|\____|_| |_|
//               a linux launcher for rock games
//          https://github.com/MarioRRom/rock-launcher
//--------------------------------------------------------------


```

### Imports

Imports are grouped by purpose and separated by a blank line.

```qml
// Qt Imports
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

// Config
import "../components"
```

Typical grouping:

1. Qt / Quick
2. Internal project imports (relative paths)

Avoid unordered import blocks.

### Section Separators

Two styles, matching the C++ convention.

**ASCII section block** — for major sections:

```qml
//  .-------------------------.
//  | .---------------------. |
//  | |      Section Name   | |
//  | `---------------------' |
//  `-------------------------'
```

Rules:

- Two blank lines above.
- One blank line below.

Use for: component definitions, large logical groups, backend bindings, animations.

**Simple comment** — for compact or minor sections:

```qml
// Public API
// Internal state
// Signals
```

Leave one blank line above.

### Typical Component Structure

Most components should follow a predictable order.

Typical layout:

1. `id`
2. Public properties
3. Internal state/config
4. Signals
5. Backend/service bindings
6. Public API
7. Main component structure
8. Animations/timers
9. Connections
10. Lifecycle hooks

Exact ordering may vary depending on complexity.

The important part is consistency and readability.

### Comments

The same rule as C++ applies: explain intent, not obvious implementation.

**Good** — explains why:

```qml
// Hide the top bar on non-launch pages to reclaim vertical space
visible: root.currentPage === "launch"
```

**Bad** — restates what the code does:

```qml
// Set visible to true or false
visible: root.currentPage === "launch"
```