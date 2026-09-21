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


Rectangle {
    anchors.fill: parent
    anchors.margins: 30
    color: "transparent"

    // Title placeholder
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2

        Text {
            text: "Rock Launcher"
            font.pixelSize: 32
            font.bold: true
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Ready to rock on linux"
            font.pixelSize: 16
            color: "#888"
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // Profile selector
    ProfilePicker {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }

    // Launch button
    PlayButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
