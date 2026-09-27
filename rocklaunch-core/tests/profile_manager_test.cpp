// Core unit tests for the profile naming rules: id generation and name tag
// validation. Exercised through ctest as rocklaunch-core-profile-manager: a form
// calls PreviewNextId()/NameValid() before saving, and neither has a CLI surface,
// so they run as a small core unit binary instead of a cmake script.
// STATE ISOLATED: the test root directory is passed as argv[1] and HOME /
// XDG_* are pointed at it, exactly like the CLI test scripts.

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/profile_manager.h"
#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

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

// The id a form shows before saving has to be the id the profile actually gets.
void TestPreviewMatchesCreatedId(rocklaunch::ProfileManager &profiles,
                                 const rocklaunch::ConfigStore &store)
{
    Check(profiles.PreviewNextId() == "rocksmith2014remastered-1", "first free id");

    std::optional<rocklaunch::ProfileConfig> first = profiles.CreateProfile("");
    Check(first.has_value(), "unnamed profile created");
    Check(first->id == "rocksmith2014remastered-1", "created id is the previewed one");
    Check(first->name.empty(), "unnamed profile carries no name");
    Check(first->prefixDir == store.DataDir() / "prefixes" / first->id,
          "prefix is keyed by the generated id, not by the name");
    Check(profiles.PreviewNextId() == "rocksmith2014remastered-2", "preview moves forward");

    std::optional<rocklaunch::ProfileConfig> second = profiles.CreateProfile("Partida de Juan");
    Check(second.has_value(), "named profile created");
    Check(second->id == "rocksmith2014remastered-2", "named profile takes the previewed id");
    Check(second->name == "Partida de Juan", "name stored on creation");
}

void TestNameValid()
{
    using rocklaunch::ProfileManager;

    Check(ProfileManager::NameValid(""), "empty name is valid (it clears the tag)");
    Check(ProfileManager::NameValid("Partida de Juan"), "spaces are valid");
    Check(ProfileManager::NameValid("ñandú \xF0\x9F\x98\x80 100%"), "UTF-8 is valid");
    Check(!ProfileManager::NameValid("con \"comillas\""), "quotes are rejected");
    Check(!ProfileManager::NameValid("salto\nde linea"), "newlines are rejected");
    Check(!ProfileManager::NameValid("tab\taqui"), "tabs are rejected");
}

void TestSetName(rocklaunch::ProfileManager &profiles)
{
    Check(profiles.SetName("rocksmith2014remastered-1", "Partida de Juan"), "rename accepted");
    Check(profiles.GetProfile("rocksmith2014remastered-1")->name == "Partida de Juan",
          "rename persisted");

    Check(profiles.SetName("rocksmith2014remastered-1", ""), "clearing the name is accepted");
    Check(profiles.GetProfile("rocksmith2014remastered-1")->name.empty(), "name cleared");

    Check(!profiles.SetName("nonexistent", "Nombre"), "renaming a missing profile fails");
    Check(!profiles.SetName("rocksmith2014remastered-1", "con \"comillas\""),
          "an invalid name is rejected");
    Check(profiles.GetProfile("rocksmith2014remastered-1")->name.empty(),
          "a rejected rename leaves the profile untouched");
}

} // namespace

int main(int argc, char **argv)
{
    fs::path testRoot = argc > 1 ? fs::path(argv[1])
                                 : fs::path("/tmp/rocklaunch-profile-manager-test");

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

    rocklaunch::ConfigStore store;
    rocklaunch::Rocksmith2014RemasteredProfile gameProfile;
    rocklaunch::ProfileManager profiles(store, gameProfile);

    TestPreviewMatchesCreatedId(profiles, store);
    TestNameValid();
    TestSetName(profiles);

    std::cout << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
