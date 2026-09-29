if (NOT DEFINED ROCKLAUNCH_CLI OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "ROCKLAUNCH_CLI and TEST_ROOT are required")
endif()

function(RunCli expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "HOME=${TEST_ROOT}/home"
            "XDG_DATA_HOME=${TEST_ROOT}/data"
            "PATH=/nonexistent"
            "${ROCKLAUNCH_CLI}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )

    if (NOT "${result}" STREQUAL "${expectedResult}")
        message(FATAL_ERROR
            "Command '${ARGN}' returned ${result}; expected ${expectedResult}.\n"
            "Output: ${output}\nError: ${error}"
        )
    endif()

    set(LAST_OUTPUT "${output}${error}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/Proton Test")
file(WRITE "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/Proton Test/proton" "")

# A Steam tool is listed with its own source, and that is the token a listing
# prints, so it is the token every command takes.
RunCli(0 runner list)
if (NOT LAST_OUTPUT MATCHES "steam/Proton Test")
    message(FATAL_ERROR "The Steam Proton runner was not discovered")
endif()

RunCli(0 profile new)
RunCli(0 runner set rocksmith2014remastered-1 "steam/Proton Test")
RunCli(0 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "Runner: steam/Proton Test")
    message(FATAL_ERROR "The selected runner was not saved to the profile")
endif()

# A bare name works too when only one source carries it.
RunCli(0 runner set rocksmith2014remastered-1 "Proton Test")
RunCli(0 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "Runner: steam/Proton Test")
    message(FATAL_ERROR "A bare runner name was not resolved to its source")
endif()

# Runner set with a nonexistent profile must fail.
RunCli(1 runner set nonexistent-profile "steam/Proton Test")
if (NOT LAST_OUTPUT MATCHES "Profile not found")
    message(FATAL_ERROR "Setting a runner on a nonexistent profile did not fail")
endif()

# Runner set with a nonexistent runner must fail.
RunCli(1 runner set rocksmith2014remastered-1 nonexistent-runner)
if (NOT LAST_OUTPUT MATCHES "Runner not installed")
    message(FATAL_ERROR "Setting a nonexistent runner did not fail")
endif()

# With no wine on PATH there is no system runner to assign.
RunCli(1 runner set rocksmith2014remastered-1 "system/system-wine")

# A profile without a runner assigned must show 'not assigned'.
RunCli(0 profile new)
RunCli(0 profile show rocksmith2014remastered-2)
if (NOT LAST_OUTPUT MATCHES "Runner: not assigned")
    message(FATAL_ERROR "Unassigned runner should show 'not assigned'")
endif()

# Reassigning a runner on an existing profile must work.
RunCli(0 runner set rocksmith2014remastered-1 "steam/Proton Test")
RunCli(0 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "Runner: steam/Proton Test")
    message(FATAL_ERROR "Runner reassignment did not take effect")
endif()

# A managed runner lives at runners/<source>/<name>/ and is listed under that source.
file(MAKE_DIRECTORY "${TEST_ROOT}/data/rock-launcher/runners/proton-ge-custom/GE-Proton Test")
file(WRITE "${TEST_ROOT}/data/rock-launcher/runners/proton-ge-custom/GE-Proton Test/proton" "")
# The same name as a Steam tool: a real name collision, one name in two sources.
file(MAKE_DIRECTORY "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/GE-Proton Test")
file(WRITE "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/GE-Proton Test/proton" "")
RunCli(0 runner list)
if (NOT LAST_OUTPUT MATCHES "proton-ge-custom/GE-Proton Test")
    message(FATAL_ERROR "The managed runner was not discovered under its repo source")
endif()

# The same name in a second source is no longer ambiguous once qualified.
RunCli(0 runner set rocksmith2014remastered-1 "proton-ge-custom/GE-Proton Test")
RunCli(0 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "Runner: proton-ge-custom/GE-Proton Test")
    message(FATAL_ERROR "The managed runner was not assigned")
endif()

# Two installed runners of the same name: a bare name is refused and both
# qualified forms are named, rather than one being picked.
RunCli(1 runner set rocksmith2014remastered-1 "GE-Proton Test")
if (NOT LAST_OUTPUT MATCHES "more than one source")
    message(FATAL_ERROR "A bare name in two sources was not reported as ambiguous")
endif()
if (NOT LAST_OUTPUT MATCHES "steam/GE-Proton Test")
    message(FATAL_ERROR "The ambiguity message did not name the Steam runner")
endif()
if (NOT LAST_OUTPUT MATCHES "proton-ge-custom/GE-Proton Test")
    message(FATAL_ERROR "The ambiguity message did not name the managed runner")
endif()

# A managed runner can be removed; a Steam one is refused, folder untouched.
RunCli(0 runner remove -f "proton-ge-custom/GE-Proton Test")
RunCli(1 runner remove -f "steam/Proton Test")
if (NOT EXISTS "${TEST_ROOT}/home/.steam/steam/compatibilitytools.d/Proton Test")
    message(FATAL_ERROR "Refusing to remove a Steam runner still deleted it")
endif()

# Profile remove must succeed with -f.
RunCli(0 profile remove -f rocksmith2014remastered-2)
RunCli(1 profile show rocksmith2014remastered-2)
if (NOT LAST_OUTPUT MATCHES "Profile not found")
    message(FATAL_ERROR "Deleted profile is still available")
endif()

# A managed runner lives at runners/<source>/<name>. Removing it by a bare name, in
# any case, resolves to the same pair the listing prints.
set(MANAGED "${TEST_ROOT}/data/rock-launcher/runners/proton-ge-custom/GE-Proton11-7")
file(WRITE "${MANAGED}/proton" "")
RunCli(0 runner list)
if (NOT LAST_OUTPUT MATCHES "proton-ge-custom/GE-Proton11-7")
    message(FATAL_ERROR "The managed runner was not listed under its source")
endif()

RunCli(0 runner remove ge-proton11-7 --force)
if (EXISTS "${MANAGED}")
    message(FATAL_ERROR "Removing a managed runner by its bare name left it on disk")
endif()

# A runner in the flat layout of older versions is not listed as a bogus source.
file(WRITE "${TEST_ROOT}/data/rock-launcher/runners/GE-Proton9-20/proton" "")
file(WRITE "${TEST_ROOT}/data/rock-launcher/runners/GE-Proton9-20/files/bin/wine" "")
RunCli(0 runner list)
if (LAST_OUTPUT MATCHES "GE-Proton9-20")
    message(FATAL_ERROR "A flat-layout directory was listed as a runner")
endif()
