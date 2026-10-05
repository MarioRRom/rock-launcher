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

    property int dialogFadeDuration: 150

    readonly property bool modalOpen: DialogController.currentDialog !== "" || DialogController.errorMessage !== ""


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Dark Backdrop     | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        anchors.fill: parent
        visible: opacity > 0
        color: "black"
        opacity: root.modalOpen ? 0.4 : 0
        Behavior on opacity { NumberAnimation { duration: root.dialogFadeDuration } }

        MouseArea {
            anchors.fill: parent
            enabled: root.modalOpen
            hoverEnabled: root.modalOpen
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
        sourceComponent: EditProfile { }
        Behavior on opacity { NumberAnimation { duration: root.dialogFadeDuration } }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |        Error        | |
    //  | `---------------------' |
    //  `-------------------------'

    // The error dialog is narrower than the editor, so without this the editor's
    // edges keep the mouse. enabled alone leaves hover and cursor to items below.
    MouseArea {
        anchors.fill: parent
        visible: DialogController.errorMessage !== ""
        enabled: visible
        hoverEnabled: visible
    }

    Loader {
        id: errorDialog
        anchors.centerIn: parent
        opacity: DialogController.errorMessage !== "" ? 1 : 0
        active: opacity > 0
        sourceComponent: Error { }
        Behavior on opacity { NumberAnimation { duration: root.dialogFadeDuration } }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Close Any Dialog  | |
    //  | `---------------------' |
    //  `-------------------------'

    Shortcut {
        enabled: root.modalOpen
        sequence: "Escape"
        onActivated: {
            if (DialogController.errorMessage !== "")
                DialogController.dismissError()
            else
                DialogController.close()
        }
    }
}
