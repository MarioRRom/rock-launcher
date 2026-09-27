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

RunCli(0 profile new steam)

# A profile whose JSON can no longer be parsed must degrade to logged errors on
# every read path (never an uncaught exception), and stay deletable.
file(WRITE "${TEST_ROOT}/data/rocksmith-launcher/profiles/rocksmith2014remastered-1.json" "{ not valid json !!!")

RunCli(1 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "could not be read")
    message(FATAL_ERROR "Corrupt profile read was not reported as a logged error")
endif()

RunCli(1 launch rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "could not be read")
    message(FATAL_ERROR "Corrupt profile launch was not reported as a logged error")
endif()

RunCli(0 profile list)
if (NOT LAST_OUTPUT MATCHES "skipping unreadable profile")
    message(FATAL_ERROR "Corrupt profile was not skipped during listing")
endif()

RunCli(0 profile remove -f rocksmith2014remastered-1)
RunCli(1 profile show rocksmith2014remastered-1)
if (NOT LAST_OUTPUT MATCHES "Profile not found: rocksmith2014remastered-1")
    message(FATAL_ERROR "Corrupt profile could not be removed")
endif()