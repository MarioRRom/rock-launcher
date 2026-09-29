#include "rocklaunch/cli/runners/runner_commands.h"

#include "rocklaunch/cli/cli_ui.h"
#include "rocklaunch/cli/progress_bar.h"
#include "rocklaunch/cli/runners/runner_match.h"

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"
#include "rocklaunch/core/runners/runners.h"

#include <exception>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{

bool LoadReleases(nlohmann::json &releases, bool forceRefresh)
{
    try {
        releases = rocklaunch::Runners::Releases(forceRefresh);
    } catch (const std::exception &error) {
        PrintError(std::string(error.what()));
        return false;
    }
    return true;
}

// "source/name" of every runner on this machine, the join key between both listings.
// Without sizes: search only needs membership, and weighing a Proton means walking it.
std::set<std::string> InstalledKeys()
{
    std::set<std::string> keys;
    for (const nlohmann::json &row : rocklaunch::Runners::Installed(false)) {
        keys.insert(runner_match::Qualify(row));
    }
    return keys;
}

int ReportUnresolved(const std::string &token,
                     const std::string &error,
                     const char *fallback)
{
    PrintError(error.empty() ? std::string(fallback) + ": " + token : error);
    return 1;
}

// "?" rather than "0 B" for a release with no tarball for this host.
std::string ReleaseSize(const nlohmann::json &row)
{
    return row.value("size", uint64_t(0)) > 0 ? HumanSize(row.value("size", uint64_t(0)))
                                              : std::string("?");
}

} // anonymous namespace

int RunnerList()
{
    nlohmann::json rows = rocklaunch::Runners::Installed();
    if (rows.empty()) {
        std::cout << "No Wine or Proton runners were found.\n";
        return 0;
    }

    std::cout << Color(PadLeft("Runner", 56), "1") << Color(PadLeft("Kind", 8), "1")
              << Color(PadLeft("Size", 10), "1") << Color("Executable", "1") << '\n';
    for (const nlohmann::json &row : rows) {
        std::cout << Color(PadLeft(runner_match::Qualify(row), 56), kRunnerColor)
                  << PadLeft(row.value("kind", ""), 8)
                  << PadLeft(HumanSize(row.value("size", uint64_t(0))), 10)
                  << row.value("executable", "") << '\n';
    }

    return 0;
}

int RunnerSet(const std::string &profileId, const std::string &runnerToken)
{
    const RunnerToken token = runner_match::ParseToken(runnerToken);

    // Installed only: assigning a release that is merely cached would fail at launch.
    const nlohmann::json installed = rocklaunch::Runners::Installed(false);
    std::string error;
    const nlohmann::json *row = runner_match::Resolve(installed, token, error);
    if (row == nullptr) {
        return ReportUnresolved(runnerToken, error, "Runner not installed");
    }

    rocklaunch::ConfigStore configStore;
    rocklaunch::Rocksmith2014RemasteredProfile gameProfile;
    rocklaunch::ProfileManager profiles(configStore, gameProfile);
    if (!profiles.SetRunner(profileId, row->value("name", ""), row->value("source", ""))) {
        return 1;
    }

    std::cout << "Assigned runner " << runner_match::Qualify(*row) << " to profile "
              << profileId << '\n';
    return 0;
}

int RunnerSearch(const std::string &query, bool forceRefresh)
{
    nlohmann::json releases;
    if (!LoadReleases(releases, forceRefresh)) {
        return 1;
    }

    const std::vector<const nlohmann::json *> rows =
        runner_match::Filter(releases["releases"], query);
    if (rows.empty()) {
        std::cout << "No releases found for: " << query << '\n';
        return 0;
    }

    const std::set<std::string> installed = InstalledKeys();

    // "Download", not "Size": runner list measures the folder on disk and this measures
    // the tarball, so the same header over both commands would mean two quantities.
    std::cout << Color(PadLeft("Runner", 56), "1") << Color(PadLeft("Download", 10), "1")
              << Color("Status", "1") << '\n';

    // Each line is the exact token install, remove and set take.
    for (const nlohmann::json *row : rows) {
        std::string line = Color(PadLeft(runner_match::Qualify(*row), 56), kRunnerColor)
            + PadLeft(ReleaseSize(*row), 10);
        if (installed.count(runner_match::Qualify(*row)) > 0) {
            line += Color("installed", "32");
        }
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
        std::cout << line << '\n';
    }

    return 0;
}

int RunnerInstall(const std::string &runnerToken,
                  const std::string &fileName,
                  bool force)
{
    const RunnerToken token = runner_match::ParseToken(runnerToken);

    // The cache, never a refresh: installing a known release costs no request.
    const nlohmann::json cache = rocklaunch::Runners::Cached();
    if (!cache.contains("releases")) {
        PrintError("No runner cache found. Run 'runner -u' first.");
        return 1;
    }

    std::string error;
    const nlohmann::json *row = runner_match::Resolve(cache["releases"], token, error);
    if (row == nullptr) {
        return ReportUnresolved(runnerToken, error, "No release matches");
    }

    const std::string name = row->value("name", "");
    const std::string source = row->value("source", "");
    const std::string qualified = runner_match::Qualify(*row);

    if (!force) {
        const std::string prompt = rocklaunch::Runners::Find(name, source).has_value()
            ? "Runner " + qualified + " is already installed. Reinstall it?"
            : "Install runner " + qualified + "?";
        if (!ConfirmDestructive(prompt)) {
            std::cout << "Aborted.\n";
            return 1;
        }
    }

    ConsoleProgressBar bar;
    try {
        rocklaunch::Runners::Install(name, source, fileName, std::ref(bar));
    } catch (const std::exception &error) {
        bar.Finish();
        PrintError("Install failed: " + std::string(error.what()));
        return 1;
    }
    bar.Finish();

    std::cout << "Installed runner: " << qualified << '\n';
    return 0;
}

int RunnerRemove(const std::string &runnerToken, bool force)
{
    const RunnerToken token = runner_match::ParseToken(runnerToken);

    // Resolved from disk, so removing needs no cache and no network. The row, not
    // the token, names what is removed: a bare name only becomes a pair here.
    const nlohmann::json installed = rocklaunch::Runners::Installed(false);
    std::string error;
    const nlohmann::json *row = runner_match::Resolve(installed, token, error);
    if (row == nullptr) {
        return ReportUnresolved(runnerToken, error, "Runner not installed");
    }
    const std::string qualified = runner_match::Qualify(*row);

    if (!force && !ConfirmDestructive("Remove runner " + qualified + "?")) {
        std::cout << "Aborted.\n";
        return 1;
    }

    try {
        rocklaunch::Runners::Remove(row->value("name", ""), row->value("source", ""));
    } catch (const std::exception &failure) {
        PrintError("Remove failed: " + std::string(failure.what()));
        return 1;
    }

    std::cout << "Removed runner: " << qualified << '\n';
    return 0;
}
