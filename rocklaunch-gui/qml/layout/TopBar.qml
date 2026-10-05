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
    color: Theme.surface0
    clip: true

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
            actived: GameProfileModel.gameId === "rocksmith2014remastered"
            icon: "../assets/LOGO/rock2014logo.png"
            onClicked: GameProfileModel.gameId = "rocksmith2014remastered"
        }

        GameProfileButton {
            enabled: false // TODO: enable when the game is supported
            actived: GameProfileModel.gameId === "rocksmithplus"
            icon: "../assets/LOGO/rockpluslogo.png"
            onClicked: GameProfileModel.gameId = "rocksmithplus"

            HoverHandler { id: rockplusHover }

            StyledTooltip {
                visible: rockplusHover.hovered
                delay: 200
                label: "Info:"
                labelColor: Theme.blue
                implicitWidth: 300
                text: "Rocksmith+ is available soon."
            }
        }

        // Spacer
        Rectangle { Layout.fillWidth: true }
    }
}
