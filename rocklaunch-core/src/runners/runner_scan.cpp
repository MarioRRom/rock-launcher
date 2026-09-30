#include "rocklaunch/core/runners/runners.h"

#include "rocklaunch/core/runners/runners_internal.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/utils/path_util.h"

#include <cstdlib>
#include <sstream>

namespace rocklaunch
{
namespace Runners
{

namespace
{

// A function, not a value: the paths read HOME, which a static initializer would
// read before the caller has set it.
std::vector<fs::path> SteamCompatToolDirs()
{
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
        return {};
    }
    return {
        fs::path(home) / ".steam" / "steam" / "compatibilitytools.d",
        fs::path(home) / ".local" / "share" / "Steam" / "compatibilitytools.d",
    };
}

struct ExternalSource
{
    const char *source;
    std::vector<fs::path> (*dirs)();
};

const std::vector<ExternalSource> kExternalSources = {
    { kSteamRunnerSource, &SteamCompatToolDirs },
};

// A runner is categorised by the executable it ships, not by its name.
struct RunnerExecutable
{
    std::string kind;
    fs::path executable;
};

std::optional<RunnerExecutable> Classify(const fs::path &rootDir)
{
    for (const char *binary : { "proton", "wine", "bin/wine" }) {
        const fs::path candidate = rootDir / binary;
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) {
            const bool isProton = std::string(binary) == "proton";
            return RunnerExecutable{ isProton ? kRunnerKindProton : kRunnerKindWine,
                                     candidate };
        }
    }
    return std::nullopt;
}

uint64_t DirSize(const fs::path &path)
{
    uint64_t total = 0;
    std::error_code error;
    for (fs::recursive_directory_iterator it(path, error), end;
         it != end; it.increment(error)) {
        std::error_code fileError;
        if (it->is_regular_file(fileError)) {
            total += it->file_size(fileError);
        }
    }
    return total;
}

std::optional<fs::path> SystemWine()
{
    const char *pathValue = std::getenv("PATH");
    if (pathValue == nullptr) {
        return std::nullopt;
    }

    std::istringstream paths(pathValue);
    std::string directory;
    while (std::getline(paths, directory, ':')) {
        const fs::path executable = fs::path(directory) / "wine";
        std::error_code error;
        if (fs::is_regular_file(executable, error)) {
            return executable;
        }
    }
    return std::nullopt;
}

nlohmann::json InstalledRow(const fs::path &rootDir, const std::string &source,
                            const std::string &name, const RunnerExecutable &executable,
                            bool withSize)
{
    nlohmann::json row = {
        { "source", source },
        { "name", name },
        { "kind", executable.kind },
        { "root", rootDir.string() },
        { "executable", executable.executable.string() },
    };
    if (withSize) {
        row["size"] = DirSize(rootDir);
    }
    return row;
}

bool IsHidden(const fs::directory_entry &entry)
{
    return entry.path().filename().string().front() == '.';
}

// Hidden entries are launcher bookkeeping (scratch dirs, .locks) or Steam's own.
void AppendRunners(nlohmann::json &rows, const fs::path &parent, const std::string &source,
                   bool withSize)
{
    std::error_code error;
    for (fs::directory_iterator it(parent, error), end; it != end; it.increment(error)) {
        if (!it->is_directory(error) || IsHidden(*it)) {
            continue;
        }

        const std::optional<RunnerExecutable> executable = Classify(it->path());
        if (executable.has_value()) {
            rows.push_back(InstalledRow(it->path(), source, it->path().filename().string(),
                                        *executable, withSize));
        }
    }
}

} // anonymous namespace

namespace detail
{

fs::path RunnersDir()
{
    return ConfigStore::DefaultDataDir() / "runners";
}

fs::path RunnerDir(const std::string &source, const std::string &name)
{
    return RunnersDir() / RequirePathComponent(source) / RequirePathComponent(name);
}

bool IsExternal(const std::string &source)
{
    if (source == kSystemRunnerSource) {
        return true;
    }
    for (const ExternalSource &external : kExternalSources) {
        if (source == external.source) {
            return true;
        }
    }
    return false;
}

} // namespace detail

nlohmann::json Installed(bool withSize)
{
    nlohmann::json rows = nlohmann::json::array();

    std::error_code error;
    for (fs::directory_iterator it(detail::RunnersDir(), error), end;
         it != end; it.increment(error)) {
        // A runner directly under runners/ is not a source: scanning it as one would
        // list its files/ folder as a runner.
        if (!it->is_directory(error) || IsHidden(*it) || Classify(it->path()).has_value()) {
            continue;
        }
        AppendRunners(rows, it->path(), it->path().filename().string(), withSize);
    }

    for (const ExternalSource &external : kExternalSources) {
        for (const fs::path &dir : external.dirs()) {
            AppendRunners(rows, dir, external.source, withSize);
        }
    }

    if (const std::optional<fs::path> wine = SystemWine()) {
        rows.push_back(InstalledRow(wine->parent_path(), kSystemRunnerSource,
                                    kSystemRunnerName, { kRunnerKindWine, *wine }, withSize));
    }

    return rows;
}

std::optional<RunnerRef> Find(const std::string &name, const std::string &source)
{
    if (!IsPathComponent(name) || !IsPathComponent(source)) {
        return std::nullopt;
    }

    if (source == kSystemRunnerSource) {
        const std::optional<fs::path> wine = SystemWine();
        if (name != kSystemRunnerName || !wine.has_value()) {
            return std::nullopt;
        }
        return RunnerRef{ name, source, kRunnerKindWine, *wine, *wine };
    }

    // An external source lives only in its own directories. Falling through to
    // RunnerDir with none (HOME unset) would look for runners/steam/<name>.
    std::vector<fs::path> roots;
    if (detail::IsExternal(source)) {
        for (const ExternalSource &external : kExternalSources) {
            if (source == external.source) {
                for (const fs::path &dir : external.dirs()) {
                    roots.push_back(dir / name);
                }
            }
        }
    } else {
        roots.push_back(detail::RunnerDir(source, name));
    }

    for (const fs::path &root : roots) {
        const std::optional<RunnerExecutable> executable = Classify(root);
        if (executable.has_value()) {
            return RunnerRef{ name, source, executable->kind, root, executable->executable };
        }
    }
    return std::nullopt;
}

} // namespace Runners
} // namespace rocklaunch
