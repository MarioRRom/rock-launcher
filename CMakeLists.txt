cmake_minimum_required(VERSION 3.21)

project(RocksmithLauncher
    VERSION 0.1.0
    DESCRIPTION "A Linux launcher for Rocksmith 2014 Edition Remastered"
    LANGUAGES CXX
)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json 3.11 QUIET)

if (NOT nlohmann_json_FOUND)
    include(FetchContent)

    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_subdirectory(rocklaunch-core)
add_subdirectory(rocklaunch-cli)

if (BUILD_TESTING)
    add_test(
        NAME rocklaunch-cli-help
        COMMAND rocklaunch-cli --help
    )
    set_tests_properties(rocklaunch-cli-help PROPERTIES
        ENVIRONMENT "XDG_DATA_HOME=${CMAKE_BINARY_DIR}/test-data"
    )

    add_test(
        NAME rocklaunch-cli-profile-lifecycle
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-lifecycle-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_lifecycle.cmake
    )

    add_test(
        NAME rocklaunch-cli-profile-duplicate
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-duplicate-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_duplicate.cmake
    )

    add_test(
        NAME rocklaunch-cli-profile-corrupt
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-corrupt-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_corrupt.cmake
    )

    add_test(
        NAME rocklaunch-cli-runner-manager
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/runner-manager-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runner_manager.cmake
    )

    add_test(
        NAME rocklaunch-cli-launch
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/launch-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/launch.cmake
    )

    # Core LaunchSession tests: the session has no CLI surface (the CLI keeps
    # ExecLaunchCommand), so it is exercised as a small core unit-test binary.
    add_executable(rocklaunch-core-launch-session-test
        rocklaunch-core/tests/launch_session_test.cpp
    )
    target_link_libraries(rocklaunch-core-launch-session-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-launch-session
        COMMAND rocklaunch-core-launch-session-test
            ${CMAKE_BINARY_DIR}/launch-session-test
    )
endif()
