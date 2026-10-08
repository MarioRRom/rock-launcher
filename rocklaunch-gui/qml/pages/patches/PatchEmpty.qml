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

Item {
    id: root


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Empty State     | |
    //  | `---------------------' |
    //  `-------------------------'

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 480)
        spacing: 16

        SvgIcon {
            Layout.alignment: Qt.AlignHCenter
            size: 200
            icon: "bandage"
            color: Theme.surface1
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "No patches to show"
            font.pixelSize: 18
            color: Theme.text
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "No patch targets the selected game"
            font.pixelSize: 14
            color: Theme.subtext0
        }
    }
}
