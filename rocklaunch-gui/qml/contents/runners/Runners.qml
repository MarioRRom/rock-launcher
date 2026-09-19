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
    color: "#181825"
    
    // Content Placeholder
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2

        Text {
            text: "Runners List"
            font.pixelSize: 32
            font.bold: true
            color: "#e0e0e0"
        }

        Text {
            text: "Ready to rock"
            font.pixelSize: 16
            color: "#888"
        }
    }
}