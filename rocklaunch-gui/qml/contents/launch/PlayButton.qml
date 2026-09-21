//--------------------------------------------------------------
//  ____   ___   ____ _  ___        _   _   _ _   _  ____ _   _ 
// |  _ \ / _ \ / ___| |/ / |      / \ | | | | \ | |/ ___| | | |
// | |_) | | | | |   | ' /| |     / _ \| | | |  \| | |   | |_| |
// |  _ <| |_| | |___| . \| |___ / ___ \ |_| | |\  | |___|  _  |
// |_| \_\\___/ \____|_|\_\_____/_/   \_\___/|_| \_|\____|_| |_|
//               a linux launcher for rock games
//          https://github.com/MarioRRom/rock-launcher
//--------------------------------------------------------------


//  .-------------------------.
//  | .---------------------. |
//  | |   Import Modules    | |
//  | `---------------------' |
//  `-------------------------'

// Qt Imports
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

// Config
import "../../components"
import RockLaunch.Gui

Rectangle {
    id: root

    property color bgColor: "#181825"
    property color bgHoverColor: "#313244"
    property color bgPressedColor: "#45475a"
    property color accentColor: "#a6e3a1"
    property color accentHoverColor: "#94d29c"
    property color accentPressedColor: "#80c788"
    property string label: "LAUNCH"

    // States are driven by the LaunchController state machine.
    // Idle=0, PreparingPrefix=1, Starting=2, Running=3, Finished=4, Error=5.
    states: [
        State {
            name: "preparing"
            when: LaunchController.launchState === 1
            PropertyChanges {
                target: root
                label: "PREPARING"
                accentColor: "#f9e2af"
                accentHoverColor: "#e8cf94"
                accentPressedColor: "#d4b878"
            }
        },
        State {
            name: "starting"
            when: LaunchController.launchState === 2
            PropertyChanges {
                target: root
                label: "LAUNCHING"
                accentColor: "#f9e2af"
                accentHoverColor: "#e8cf94"
                accentPressedColor: "#d4b878"
            }
        },
        State {
            name: "running"
            when: LaunchController.launchState === 3
            PropertyChanges {
                target: root
                label: "STOP"
                accentColor: "#f38ba8"
                accentHoverColor: "#e2748f"
                accentPressedColor: "#ce5c7b"
            }
        }
    ]

    implicitWidth: 260
    implicitHeight: 64
    radius: 16
    color: hover.pressed ? bgPressedColor : hover.containsMouse ? bgHoverColor : bgColor

    // Button content
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            radius: 16
            color: hover.pressed ? root.accentPressedColor : hover.containsMouse ? root.accentHoverColor : root.accentColor

            Text {
                anchors.centerIn: parent
                text: root.label
                font.pixelSize: 30
                font.bold: true
                color: "#1e1e2e"
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: root.height
            color: "transparent"

            SvgIcon {
                anchors.centerIn: parent
                icon: "player-play"
                size: 28
                color: root.accentColor
            }
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: LaunchController.launch()
    }
}
