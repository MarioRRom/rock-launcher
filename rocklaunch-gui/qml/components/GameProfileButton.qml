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
import RockLaunch.Gui

Rectangle {
    id: root

    // Public API
    property bool actived: false
    property string icon: ""
    property int size: 140
    property int borderRadius: 8

    // Normal colors
    property color imageColor: Theme.text
    property color bgColor: "transparent"
    property color bgHoverColor: Theme.surface1
    property color bgPressedColor: Theme.surface2

    // Active colors
    property color imageColorActive: Theme.surface0
    property color bgColorActive: Theme.yellow
    property color bgHoverColorActive: Theme.surface1
    property color bgPressedColorActive: Theme.surface2

    signal clicked()

    implicitWidth: size
    Layout.fillHeight: true
    radius: borderRadius
    color: {
        if (hover.pressed)
            return root.actived ? bgPressedColorActive : bgPressedColor
        if (hover.containsMouse)
            return root.actived ? bgHoverColorActive : bgHoverColor
        return root.actived ? bgColorActive : bgColor
    }
    clip: true

    // Game image
    Image {
        id: baseImage
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.9
        height: parent.height * 0.7
        smooth: true
        mipmap: true
        source: root.icon
        fillMode: Image.PreserveAspectFit
    }

    // Colorized effect
    MultiEffect {
        anchors.fill: baseImage
        source: baseImage
        visible: {
            var c = root.actived ? root.imageColorActive : root.imageColor
            return c !== "transparent"
        }
        colorization: 1.0
        colorizationColor: root.actived ? root.imageColorActive : root.imageColor
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
