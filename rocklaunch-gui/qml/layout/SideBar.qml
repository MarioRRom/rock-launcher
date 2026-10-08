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

    // topbar height for the banner
    property int topbarHeight: 74

    // Internal Settings
    property string currentPage: "launch"
    signal pageChanged(string page)

    // Launcher banner
    Image {
        id: banner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.topbarHeight
        source: "../assets/banner.png"
        fillMode: Image.PreserveAspectFit
    }

    ColumnLayout {
        anchors.top: banner.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 9
        spacing: 12


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
