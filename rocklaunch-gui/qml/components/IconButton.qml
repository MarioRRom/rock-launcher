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

// Config
import RockLaunch.Gui

Rectangle {
    id: root

    // Public API
    property bool actived: false
    property string icon: ""
    property int size: 48
    property int borderRadius: 12

    // Normal colors
    property color bgColor: Theme.surface0
    property color bgHoverColor: Theme.surface1
    property color bgPressedColor: Theme.surface2
    property color iconColor: Theme.text

    // Active colors
    property color bgColorActive: Theme.surface0
    property color bgHoverColorActive: Theme.surface1
    property color bgPressedColorActive: Theme.surface2
    property color iconColorActive: Theme.blue


    signal clicked()

    implicitWidth: root.size
    implicitHeight: root.size
    radius: borderRadius
    color: {
        if (hover.pressed)
            return root.actived ? bgPressedColorActive : bgPressedColor
        if (hover.containsMouse)
            return root.actived ? bgHoverColorActive : bgHoverColor
        return root.actived ? bgColorActive : bgColor
    }
    clip: true

    // Centered icon only
    SvgIcon {
        anchors.centerIn: parent
        icon: root.icon
        size: root.size - 16
        color: root.actived ? root.iconColorActive : root.iconColor
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}