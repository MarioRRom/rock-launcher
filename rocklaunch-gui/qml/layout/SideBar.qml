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
    color: Theme.mantle

    // Internal Settings
    property string currentPage: "launch"
    signal pageChanged(string page)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 9
        spacing: 12


        // Launcher logo
        Image {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            smooth: true
            mipmap: true
            source: "../assets/banner.png"
            fillMode: Image.PreserveAspectFit
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | | Launch Game Section | |
        //  | `---------------------' |
        //  `-------------------------'

        IconTextButton {
            Layout.fillWidth: true
            text: "Launch"
            icon: "device-gamepad"
            actived: root.currentPage === "launch"
            onClicked: root.pageChanged("launch")
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | | Runner List Section | |
        //  | `---------------------' |
        //  `-------------------------'

        IconTextButton {
            Layout.fillWidth: true
            text: "Runners"
            icon: "glass-full"
            actived: root.currentPage === "runners"
            onClicked: root.pageChanged("runners")
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | | Patches list Section| |
        //  | `---------------------' |
        //  `-------------------------'

        IconTextButton {
            Layout.fillWidth: true
            text: "Patches"
            icon: "bandage"
            actived: root.currentPage === "patches"
            onClicked: root.pageChanged("patches")
        }

        // Separator
        Item { Layout.fillHeight: true }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |  Launcher Section   | |
        //  | `---------------------' |
        //  `-------------------------'

        IconTextButton {
            Layout.fillWidth: true
            size: 40
            text: "Settings"
            icon: "settings"
            enabled: false // TODO: enable when the page is ready
            actived: root.currentPage === "settings"
            onClicked: root.pageChanged("settings")
        }

        IconTextButton {
            Layout.fillWidth: true
            size: 40
            text: "About"
            icon: "exclamation-circle"
            enabled: false // TODO: enable when the page is ready
            actived: root.currentPage === "about"
            onClicked: root.pageChanged("about")
        }
    }
}
