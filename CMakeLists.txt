cmake_minimum_required(VERSION 3.21)

project(RockLauncher
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
find_package(CURL 7.85 REQUIRED)
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
add_subdirectory(rocklaunch-gui)

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
        NAME rocklaunch-cli-profile-name
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-name-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_name.cmake
    )

    add_test(
        NAME rocklaunch-cli-profile-corrupt
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/profile-corrupt-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/profile_corrupt.cmake
    )

    # The script empties PATH itself: the host's system wine must not appear in
    # `runner list` or the row assertions stop being about the fixtures.
    add_test(
        NAME rocklaunch-cli-runner-manager
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/runner-manager-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/runner_manager.cmake
    )

    # PATH is emptied: a wine on the host would show up in `runner list` and the
    # fixture rows would no longer be the only ones the script can see.
    add_test(
        NAME rocklaunch-cli-launch
        COMMAND ${CMAKE_COMMAND}
            -DROCKLAUNCH_CLI=$<TARGET_FILE:rocklaunch-cli>
            -DTEST_ROOT=${CMAKE_BINARY_DIR}/launch-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/launch.cmake
    )
    set_tests_properties(rocklaunch-cli-launch PROPERTIES
        ENVIRONMENT "PATH=/nonexistent"
    )

    # Core ProfileManager naming rules: PreviewNextId()/NameValid() back the
    # profile form and have no CLI surface, so they are exercised as a core unit
    # test binary, like LaunchSession.
    add_executable(rocklaunch-core-profile-manager-test
        rocklaunch-core/tests/profile_manager_test.cpp
    )
    target_link_libraries(rocklaunch-core-profile-manager-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-profile-manager
        COMMAND rocklaunch-core-profile-manager-test
            ${CMAKE_BINARY_DIR}/profile-manager-test
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

    # Progress and cancellation have no CLI surface. libcurl's FILE protocol
    # drives the same code path without a network.
    add_executable(rocklaunch-core-downloader-test
        rocklaunch-core/tests/downloader_test.cpp
    )
    target_link_libraries(rocklaunch-core-downloader-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-downloader
        COMMAND rocklaunch-core-downloader-test
            ${CMAKE_BINARY_DIR}/downloader-test
    )

    # The installed scan across its three origins, the (source, name) lookup, and
    # install/remove incl. hash failure, stage cancel, lock and scratch sweep. The
    # tarballs are served over file://, so no test touches the network.
    add_executable(rocklaunch-core-runners-test
        rocklaunch-core/tests/runners_test.cpp
    )
    target_link_libraries(rocklaunch-core-runners-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-runners
        COMMAND rocklaunch-core-runners-test
            ${CMAKE_BINARY_DIR}/runners-test
    )

    # PathLock and RequirePathComponent have no CLI surface. The lock test forks a
    # child: release on holder death cannot be observed from one process.
    add_executable(rocklaunch-core-file-lock-test
        rocklaunch-core/tests/file_lock_test.cpp
    )
    target_link_libraries(rocklaunch-core-file-lock-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-file-lock
        COMMAND rocklaunch-core-file-lock-test
            ${CMAKE_BINARY_DIR}/file-lock-test
    )

    # The subprocess contract: non-throwing primitive, fail-fast wrapper, and the
    # cancel predicate. Every case is a local binary, so no runner or network.
    add_executable(rocklaunch-core-subprocess-test
        rocklaunch-core/tests/subprocess_test.cpp
    )
    target_link_libraries(rocklaunch-core-subprocess-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-subprocess
        COMMAND rocklaunch-core-subprocess-test
            ${CMAKE_BINARY_DIR}/subprocess-test
    )

    # Logger has no CLI surface, and a logfile that cannot be written must not
    # abort the operation being logged about.
    add_executable(rocklaunch-core-logger-test
        rocklaunch-core/tests/logger_test.cpp
    )
    target_link_libraries(rocklaunch-core-logger-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-logger
        COMMAND rocklaunch-core-logger-test
            ${CMAKE_BINARY_DIR}/logger-test
    )

    # The shared tmp-and-rename write. Its three failure messages are what the CLI
    # prints verbatim, and it has no CLI surface of its own.
    add_executable(rocklaunch-core-atomic-file-test
        rocklaunch-core/tests/atomic_file_test.cpp
    )
    target_link_libraries(rocklaunch-core-atomic-file-test PRIVATE rocklaunch::core)
    add_test(
        NAME rocklaunch-core-atomic-file
        COMMAND rocklaunch-core-atomic-file-test
            ${CMAKE_BINARY_DIR}/atomic-file-test
    )
endif()
