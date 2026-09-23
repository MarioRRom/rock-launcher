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
import QtQuick.Controls

// Config
import "../../components"


Rectangle {
    id: root
    anchors.fill: parent
    anchors.margins: 30
    color: "transparent"

    ColumnLayout {
        anchors.fill: parent
        spacing: 12


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Header        | |
        //  | `---------------------' |
        //  `-------------------------'
        RowLayout {
            id: headerRow
            Layout.preferredHeight: 40
            spacing: 12

            // Ge-Proton button
            IconTextButton {
                actived: true
                text: "GE-Proton"
                icon: "glass-full"
                textColorActive: "#cba6f7"
                size: parent.height
                onClicked: {
                    // TODO: wire to C++ Ge-Proton list
                }
            }

            // Proton-Cachyos button
            IconTextButton {
                text: "Proton-Cachyos"
                icon: "cachyos"
                textColorActive: "#89dceb"
                size: parent.height
                onClicked: {
                    // TODO: wire to C++ Proton-Cachyos list
                }
            }

            // separator
            Item { Layout.fillWidth: true }

            // Search bar
            TextField {
                id: searchBar
                Layout.preferredWidth: 300
                Layout.preferredHeight: headerRow.Layout.preferredHeight
                Layout.alignment: Qt.AlignVCenter
                color: "#cdd6f4"
                placeholderTextColor: "#a6adc8"
                background: Rectangle {
                    color: "#313244"
                    radius: 12

                    // Search icon
                    SvgIcon {
                        id: searchIcon
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        icon: "search"
                        size: parent.height - 18
                    }
                }
                font.pixelSize: 16
                placeholderText: "Search for runners"
                selectByMouse: true
                leftPadding: 16 + searchIcon.width
                rightPadding: 16
            }

            // reload button
            IconButton {
                icon: "refresh"
                size: parent.height
                onClicked: {
                    // TODO: reload runners
                }
            }
        }

        // Cool separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#45475a"
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |     Runner List     | |
        //  | `---------------------' |
        //  `-------------------------'

        // Runner list
        RunnerList {
            id: runnerList
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}