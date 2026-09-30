if (NOT DEFINED ROCKLAUNCH_CLI OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "ROCKLAUNCH_CLI and TEST_ROOT are required")
endif()

function(RunCli expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "XDG_DATA_HOME=${TEST_ROOT}/data"
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

# Ids are colored, so assertions run on the last output without the escapes.
function(LastOutput outVar)
    string(ASCII 27 escape)
    string(REGEX REPLACE "${escape}\\[[0-9]+m" "" plain "${LAST_OUTPUT}")
    set(${outVar} "${plain}" PARENT_SCOPE)
endfunction()

# CMake drops empty arguments while expanding a list, so the empty name that
# clears a profile is spelled out instead of passed through ARGN.
function(RunCliClearName expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "XDG_DATA_HOME=${TEST_ROOT}/data"
            "${ROCKLAUNCH_CLI}" profile rename rocksmith2014remastered-1 ""
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )

    if (NOT "${result}" STREQUAL "${expectedResult}")
        message(FATAL_ERROR
            "Command 'profile rename rocksmith2014remastered-1 \"\"' returned ${result}; "
            "expected ${expectedResult}.\nOutput: ${output}\nError: ${error}"
        )
    endif()

    set(LAST_OUTPUT "${output}${error}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${TEST_ROOT}")

# Ids are always the next free "<gameId>-<n>", never chosen by the user.
RunCli(0 profile new)
LastOutput(output)
if (NOT "${output}" MATCHES "Created profile: rocksmith2014remastered-1")
    message(FATAL_ERROR "The first profile did not get the first generated id")
endif()

file(READ "${TEST_ROOT}/data/rock-launcher/profiles/rocksmith2014remastered-1.json" profileJson)
if (NOT "${profileJson}" MATCHES "\"name\": \"\"")
    message(FATAL_ERROR "A new profile must persist an empty name")
endif()

# A name given at creation is a tag on top of the generated id.
RunCli(0 profile new "Partida de Juan")
LastOutput(output)
if (NOT "${output}" MATCHES "Created profile: rocksmith2014remastered-2 \"Partida de Juan\"")
    message(FATAL_ERROR "The name was not stored on the created profile")
endif()

# The same tag twice is not a conflict: names are not identifiers.
RunCli(0 profile new "Partida de Juan")
LastOutput(output)
if (NOT "${output}" MATCHES "Created profile: rocksmith2014remastered-3 \"Partida de Juan\"")
    message(FATAL_ERROR "A repeated name should still create a new profile")
endif()

# Listing shows the id alone when there is no name.
RunCli(0 profile list)
LastOutput(output)
if ("${output}" MATCHES "rocksmith2014remastered-1 \"")
    message(FATAL_ERROR "An unnamed profile should be listed with its id alone")
endif()

# Renaming an existing profile only changes its tag.
RunCli(0 profile rename rocksmith2014remastered-1 "Nombre establecido")
RunCli(0 profile show rocksmith2014remastered-1)
LastOutput(output)
if (NOT "${output}" MATCHES "Name: Nombre establecido")
    message(FATAL_ERROR "The new name was not saved to the profile")
endif()

RunCli(0 profile list)
LastOutput(output)
if (NOT "${output}" MATCHES "rocksmith2014remastered-1 \"Nombre establecido\"")
    message(FATAL_ERROR "The listing does not show the name of a profile")
endif()

# An empty name clears the tag.
RunCliClearName(0)
RunCli(0 profile show rocksmith2014remastered-1)
LastOutput(output)
if (NOT "${output}" MATCHES "Name: not assigned")
    message(FATAL_ERROR "An empty name should clear the tag")
endif()

# Renaming a profile that does not exist must fail.
RunCli(1 profile rename nonexistent-profile "Nombre")
if (NOT LAST_OUTPUT MATCHES "Profile not found: nonexistent-profile")
    message(FATAL_ERROR "Renaming a nonexistent profile should fail")
endif()

# Names are rendered quoted, so quotes and control characters are rejected.
RunCli(1 profile rename rocksmith2014remastered-1 "con \"comillas\"")
if (NOT LAST_OUTPUT MATCHES "may not contain quotes or control characters")
    message(FATAL_ERROR "A name with quotes was accepted")
endif()

RunCli(1 profile new "con \"comillas\"")
if (NOT LAST_OUTPUT MATCHES "may not contain quotes or control characters")
    message(FATAL_ERROR "A name with quotes was accepted at creation")
endif()

# Listing can be scoped to one game; a game without profiles is reported as such.
RunCli(0 profile list rocksmith2014remastered)
if (NOT LAST_OUTPUT MATCHES "rocksmith2014remastered-1")
    message(FATAL_ERROR "Filtered profile list did not include matching profiles")
endif()
RunCli(0 profile list nosuchgame)
if (NOT LAST_OUTPUT MATCHES "No profiles for game nosuchgame")
    message(FATAL_ERROR "Filtered profile list should report no matches")
endif()
