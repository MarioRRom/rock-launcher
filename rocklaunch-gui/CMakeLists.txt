find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_policy(SET QTP0004 NEW)

qt_add_executable(rocklaunch-gui
    src/main.cpp
    src/game_profile_model.cpp
    src/launch_controller.cpp
    src/profile_model.cpp
    src/runners/runner_model.cpp
)

qt_add_qml_module(rocklaunch-gui
    URI RockLaunch.Gui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    QML_FILES
        qml/layout/Content.qml
        qml/layout/TopBar.qml
        qml/layout/SideBar.qml
        qml/contents/launch/Launch.qml
        qml/contents/launch/PlayButton.qml
        qml/contents/launch/ProfilePicker.qml
        qml/contents/runners/Runners.qml
        qml/components/IconTextButton.qml
        qml/components/GameProfileButton.qml
        qml/components/SvgIcon.qml
    RESOURCES
        qml/assets/LOGO.png
        qml/assets/icons/device-gamepad.svg
        qml/assets/icons/player-play.svg
        qml/assets/icons/glass-full.svg
        qml/assets/icons/settings.svg
        qml/assets/icons/bandage.svg
        qml/assets/icons/exclamation-circle.svg
        qml/assets/LOGO/rock2014logo.png
        qml/assets/LOGO/rockpluslogo.png
)

target_link_libraries(rocklaunch-gui PRIVATE
    rocklaunch::core
    Qt6::Quick
)

target_compile_features(rocklaunch-gui PRIVATE cxx_std_17)

set_target_properties(rocklaunch-gui PROPERTIES
    MACOSX_BUNDLE TRUE
    QT_KEYWORDS ON
)
