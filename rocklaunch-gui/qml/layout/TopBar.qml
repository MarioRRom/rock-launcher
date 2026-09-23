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

Rectangle {
    id: root
    color: "#313244"

    property string currentGame: "rocksmith2014remastered"
    signal gameChanged(string game)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 30
        anchors.rightMargin: 30
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        spacing: 8


        //  .-------------------------.
        //  | .---------------------. |
        //  | |     Game Buttons    | |
        //  | `---------------------' |
        //  `-------------------------'

        GameProfileButton {
            actived: root.currentGame === "rocksmith2014remastered"
            icon: "../assets/LOGO/rock2014logo.png"
            onClicked: { root.currentGame = "rocksmith2014remastered"; root.gameChanged("rocksmith2014remastered") }
        }

        GameProfileButton {
            visible: false // THIS IS A PLACEHOLDER, rocksmith+ is supported coming soon
            actived: root.currentGame === "rocksmithplus"
            icon: "../assets/LOGO/rockpluslogo.png"
            onClicked: { root.currentGame = "rocksmithplus"; root.gameChanged("rocksmithplus") }
        }

        // Spacer
        Rectangle { Layout.fillWidth: true }
    }
}
