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
import "../components"
import RockLaunch.Gui

Rectangle {
    id: root

    readonly property string message: DialogController.errorMessage

    readonly property int margin: 20

    implicitWidth: 560
    implicitHeight: dialogColumn.implicitHeight + margin * 2
    radius: 14
    color: Theme.base
    clip: true


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Dialog Body     | |
    //  | `---------------------' |
    //  `-------------------------'

    ColumnLayout {
        id: dialogColumn
        anchors.fill: parent
        anchors.margins: root.margin
        spacing: 18


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Header        | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            SvgIcon {
                icon: "exclamation-circle"
                size: 22
                color: Theme.red
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                Layout.fillWidth: true
                text: "Error"
                font.pixelSize: 18
                font.bold: true
                color: Theme.red
            }
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Message       | |
        //  | `---------------------' |
        //  `-------------------------'

        Text {
            Layout.fillWidth: true
            text: root.message
            font.pixelSize: 15
            color: Theme.text
            wrapMode: Text.WordWrap
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Footer        | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item { Layout.fillWidth: true }

            IconTextButton {
                text: "Accept"
                icon: "circle-check"
                size: 34
                bgColor: Theme.surface0
                onClicked: DialogController.dismissError()
            }
        }
    }
}
