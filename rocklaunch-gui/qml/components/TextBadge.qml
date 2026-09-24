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
import QtQuick
import QtQuick.Layouts

// Config
import RockLaunch.Gui

Rectangle {
    id: root

    // Public API
    property string text: ""
    property color bgColor: Theme.surface0
    property color textColor: Theme.text
    property int size: 22

    // Width hugs the text; height is the badge size
    implicitWidth: badgeLabel.implicitWidth + 16
    implicitHeight: root.size
    radius: implicitHeight / 2
    color: root.bgColor

    // Never stretch inside parent layouts
    Layout.fillWidth: false
    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

    Text {
        id: badgeLabel
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Math.round(root.size * 0.55)
        font.bold: true
        color: root.textColor
    }
}