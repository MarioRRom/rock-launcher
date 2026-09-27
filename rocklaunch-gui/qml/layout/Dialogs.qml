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

Item {
    id: root
    anchors.fill: parent

    property int opacityAnimDuration: 150


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Dark Backdrop     | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        anchors.fill: parent
        visible: opacity > 0
        color: "black"
        opacity: DialogController.currentDialog !== "" ? 0.4 : 0
        Behavior on opacity { NumberAnimation { duration: root.opacityAnimDuration } }

        MouseArea {
            anchors.fill: parent
            enabled: DialogController.currentDialog !== ""
            hoverEnabled: DialogController.currentDialog !== ""
        }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Edit Profile    | |
    //  | `---------------------' |
    //  `-------------------------'

    Loader {
        id: editProfile
        anchors.centerIn: parent
        opacity: DialogController.currentDialog === "editProfile" ? 1 : 0
        active: opacity > 0
        source: "../dialogs/EditProfile.qml"
        Behavior on opacity { NumberAnimation { duration: root.opacityAnimDuration } }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Close Any Dialog  | |
    //  | `---------------------' |
    //  `-------------------------'

    Shortcut {
        enabled: DialogController.currentDialog !== ""
        sequence: "Escape"
        onActivated: DialogController.close()
    }
}