find_package(Qt6 REQUIRED COMPONENTS Quick Concurrent)

qt_standard_project_setup(REQUIRES 6.5)

qt_policy(SET QTP0004 NEW)

qt_add_executable(rocklaunch-gui
    src/main.cpp
)

target_include_directories(rocklaunch-gui PRIVATE src src/utils src/runners)

qt_add_qml_module(rocklaunch-gui
    URI RockLaunch.Gui
    VERSION 1.0
    SOURCES
        src/game_profile_model.h
        src/game_profile_model.cpp
        src/profile_model.h
        src/profile_model.cpp
        src/launch_controller.h
        src/launch_controller.cpp
        src/runners/runner_jobs.h
        src/runners/runner_jobs.cpp
        src/runners/runner_model.h
        src/runners/runner_model.cpp
        src/utils/job_worker.h
        src/utils/job_worker.cpp
        src/utils/qt_process_handle.h
        src/utils/qt_process_handle.cpp
        src/utils/theme.h
        src/utils/theme.cpp
        src/utils/dialog_controller.h
        src/utils/dialog_controller.cpp
    QML_FILES
        qml/Main.qml
    QML_FILES
        qml/layout/PageHost.qml
        qml/layout/Dialogs.qml
        qml/layout/TopBar.qml
        qml/layout/SideBar.qml
        qml/pages/launch/Launch.qml
        qml/pages/launch/PlayButton.qml
        qml/pages/launch/ProfilePicker.qml
        qml/pages/runners/Runners.qml
        qml/pages/runners/RunnerList.qml
        qml/pages/runners/RunnerEmpty.qml
        qml/components/IconButton.qml
        qml/components/TextBadge.qml
        qml/components/IconTextButton.qml
        qml/components/ExpandableList.qml
        qml/components/GameProfileButton.qml
        qml/components/SvgIcon.qml
        qml/components/controls/indicator/StyledProgressBar.qml
        qml/components/controls/indicator/StyledScrollBar.qml
        qml/components/controls/input/StyledSearchField.qml
        qml/components/controls/input/StyledTextField.qml
        qml/components/controls/popup/StyledTooltip.qml
        qml/dialogs/EditProfile.qml
        qml/dialogs/Error.qml
    RESOURCES
        qml/assets/banner.png
        qml/assets/icons/device-gamepad.svg
        qml/assets/icons/player-play.svg
        qml/assets/icons/glass-full.svg
        qml/assets/icons/settings.svg
        qml/assets/icons/bandage.svg
        qml/assets/icons/exclamation-circle.svg
        qml/assets/icons/refresh.svg
        qml/assets/icons/download.svg
        qml/assets/icons/trash.svg
        qml/assets/icons/cachyos.svg
        qml/assets/icons/search.svg
        qml/assets/icons/pencil.svg
        qml/assets/icons/plus.svg
        qml/assets/icons/folder.svg
        qml/assets/icons/steam.svg
        qml/assets/icons/device-floppy.svg
        qml/assets/icons/x.svg
        qml/assets/icons/circle-check.svg
        qml/assets/icons/help-circle.svg
        qml/assets/LOGO/rock2014logo.png
        qml/assets/LOGO/rockpluslogo.png
)

target_link_libraries(rocklaunch-gui PRIVATE
    rocklaunch::core
    Qt6::Quick
    Qt6::Concurrent
)

target_compile_features(rocklaunch-gui PRIVATE cxx_std_17)

set_target_properties(rocklaunch-gui PROPERTIES
    MACOSX_BUNDLE TRUE
    QT_KEYWORDS ON
)
