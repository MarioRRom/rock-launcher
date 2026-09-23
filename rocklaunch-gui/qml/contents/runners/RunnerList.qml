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
import "../../components"

// Runner list: one card per runner (icon, version, badges, actions).
// Feed it a model from the parent page (e.g. root.runners).
ListView {
    id: root

    spacing: 12
    clip: true

    property var runners: [ "GE-Proton11-7", "cachyos-11.0-20260703-slr" ] //TODO: temporal placeholder
    model: root.runners

    // Runner card
    delegate: Rectangle {
        id: runnerCard
        required property string modelData
        required property int index

        width: ListView.view.width
        height: 80
        radius: 12
        color: "#313244"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            // Icon
            SvgIcon {
                id: runnerIcon
                Layout.alignment: Qt.AlignVCenter
                icon: "glass-full" // TODO: wire to C++ icon per type
                color: "#cba6f7"
                size: parent.height - 24
            }

            // Runner Info
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 4

                // Runner version
                Text {
                    text: runnerCard.modelData
                    font.pixelSize: 18
                    color: "#cdd6f4"
                }

                // Runner badges (size + installed, side by side)
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignLeft

                    // Runner size
                    TextBadge {
                        text: "240MB" // TODO: wire to C++ size
                        bgColor: "#89b4fa"
                        textColor: "#1e1e2e"
                        size: 20
                    }

                    // Installed check
                    TextBadge {
                        text: "Installed"
                        bgColor: "#a6e3a1"
                        textColor: "#1e1e2e"
                        visible: false // TODO: wire to C++ installed check, flip to true when installed
                    }
                }
            }

            // Separator
            Item { Layout.fillWidth: true }

            // Download button
            IconButton {
                visible: true // TODO: wire to C++ download action
                Layout.alignment: Qt.AlignVCenter
                icon: "download"
                size: parent.height - 36
                borderRadius: 8
                onClicked: {
                    // TODO: wire to C++ download action
                }
            }

            // Delete button
            IconButton {
                visible: true // TODO: wire to C++ delete action
                Layout.alignment: Qt.AlignVCenter
                icon: "trash"
                size: parent.height - 36
                borderRadius: 8
                onClicked: {
                    // TODO: wire to C++ delete action
                }
            }
        }
    }
}