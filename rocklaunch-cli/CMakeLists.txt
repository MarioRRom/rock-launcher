add_executable(rocklaunch-cli
    src/main.cpp
    src/cli_ui.cpp
    src/progress_bar.cpp
    src/runners/runner_commands.cpp
    src/runners/runner_match.cpp
)

target_include_directories(rocklaunch-cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rocklaunch-cli PRIVATE rocklaunch::core)

target_compile_features(rocklaunch-cli PRIVATE cxx_std_17)

target_compile_definitions(rocklaunch-cli PRIVATE
    ROCKLAUNCH_VERSION="${PROJECT_VERSION}"
)
