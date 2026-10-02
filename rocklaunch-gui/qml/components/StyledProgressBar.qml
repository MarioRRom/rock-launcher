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
import QtQuick.Controls

// Config
import RockLaunch.Gui

ProgressBar {
    id: root

    // Public API
    property color bgColor: Theme.surface1
    property color accentColor: Theme.green
    property color gradientColor: Theme.teal

    implicitHeight: 12
    padding: 0


    //  .-------------------------.
    //  | .---------------------. |
    //  | |      Background     | |
    //  | `---------------------' |
    //  `-------------------------'

    background: Rectangle {
        color: root.bgColor
        radius: 12
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Progress Bar    | |
    //  | `---------------------' |
    //  `-------------------------'

    contentItem: Rectangle {
        color: "transparent"
        radius: 12
        clip: true

        Rectangle {
            id: bar

            height: parent.height
            radius: 12
            width: root.indeterminate ? Math.max(height, parent.width * 0.3)
                                       : Math.max(height, parent.width * root.visualPosition)
            x: root.indeterminate ? busyX : 0

            property real busyX: 0

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: root.accentColor
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
                GradientStop {
                    position: 1.0
                    color: root.gradientColor
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }

            Behavior on width {
                NumberAnimation {
                    duration: 350
                    easing.type: Easing.OutQuint
                }
            }
        }

        NumberAnimation {
            target: bar
            property: "busyX"
            running: root.indeterminate && root.visible
            loops: Animation.Infinite
            from: -Math.max(bar.height, bar.parent.width * 0.3)
            to: bar.parent.width
            duration: 1200
            easing.type: Easing.InOutQuad
        }
    }
}
