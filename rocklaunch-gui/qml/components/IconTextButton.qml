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

// Config
import RockLaunch.Gui

Rectangle {
    id: root

    // Public API
    property bool actived: false
    property string text: ""
    property string icon: ""
    property int size: 48
    property int borderRadius: 8

    // Normal colors
    property color bgColor: "transparent"
    property color bgHoverColor: Theme.surface1
    property color bgPressedColor: Theme.surface2
    property color textColor: Theme.text

    // Active colors
    property color bgColorActive: Theme.surface0
    property color bgHoverColorActive: Theme.surface1
    property color bgPressedColorActive: Theme.surface2
    property color textColorActive: Theme.blue


    signal clicked()

    implicitWidth: row.implicitWidth + 24
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

    // Content layout
    RowLayout {
        id: row
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8

        SvgIcon {
            visible: root.icon !== ""
            icon: root.icon
            size: root.size - 16
            color: root.actived ? root.textColorActive : root.textColor
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            visible: root.text !== ""
            text: root.text
            font.pixelSize: root.size / 3
            color: root.actived ? root.textColorActive : root.textColor
            Layout.alignment: Qt.AlignVCenter
        }

        Rectangle { Layout.fillWidth: true }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
