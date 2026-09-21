if (NOT DEFINED ROCKLAUNCH_CLI OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "ROCKLAUNCH_CLI and TEST_ROOT are required")
endif()

function(RunCli expectedResult)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "XDG_CONFIG_HOME=${TEST_ROOT}/config"
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

file(REMOVE_RECURSE "${TEST_ROOT}")

# A duplicate name is rejected, with the message reported by the core logger
# (visible on stderr exactly as the CLI user and the GUI terminal would see it).
RunCli(0 profile new steam)
RunCli(1 profile new steam)
if (NOT LAST_OUTPUT MATCHES "Profile already exists: steam")
    message(FATAL_ERROR "Duplicate profile name was not rejected")
endif()

# Explicit names and auto-generated "<gameId>-<n>" names coexist.
RunCli(0 profile new)
RunCli(0 profile new)
RunCli(0 profile list)
if (NOT LAST_OUTPUT MATCHES "rocksmith2014remastered-1")
    message(FATAL_ERROR "Auto-generated default profile id was not created")
endif()

# Listing can be scoped to one game; a game without profiles is reported as such.
RunCli(0 profile list rocksmith2014remastered)
if (NOT LAST_OUTPUT MATCHES "steam")
    message(FATAL_ERROR "Filtered profile list did not include matching profiles")
endif()
RunCli(0 profile list nosuchgame)
if (NOT LAST_OUTPUT MATCHES "No profiles for game nosuchgame")
    message(FATAL_ERROR "Filtered profile list should report no matches")
endif()