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
    anchors.fill: parent
    anchors.margins: 30
    anchors.topMargin: 12
    color: "transparent"

    // Which chip is lit: picking the tag filter belongs to the patch model.
    property string filterTag: "all"

    readonly property int emptyStateFadeDuration: 250

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

            // All patches button
            IconTextButton {
                actived: root.filterTag === "all"
                text: "All"
                icon: "bandage"
                size: parent.height
                onClicked: {
                    root.filterTag = "all"
                    // TODO: drop the tag filter in the patch model
                }
            }

            // Audio filters button
            IconTextButton {
                actived: root.filterTag === "audio"
                text: "Audio"
                icon: "microphone"
                textColorActive: Theme.mauve
                size: parent.height
                onClicked: {
                    root.filterTag = "audio"
                    // TODO: filter by audio tags in the patch model
                }
            }

            // Mods button
            IconTextButton {
                actived: root.filterTag === "mods"
                text: "Mods"
                icon: "puzzle"
                textColorActive: Theme.sky
                size: parent.height
                onClicked: {
                    root.filterTag = "mods"
                    // TODO: filter by mods tags in the patch model
                }
            }

            // separator
            Item { Layout.fillWidth: true }

            // Search bar
            StyledSearchField {
                Layout.preferredWidth: 300
                Layout.preferredHeight: headerRow.Layout.preferredHeight
                Layout.alignment: Qt.AlignVCenter
                placeholderText: "Search for patches"
                onTextChanged: {
                    // TODO: search patches
                }
            }
        }

        // Cool separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.surface1
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |     Patch List      | |
        //  | `---------------------' |
        //  `-------------------------'

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Patch list
            PatchList {
                id: patchList
                anchors.fill: parent
                opacity: patchList.count > 0 ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: root.emptyStateFadeDuration } }
            }

            PatchEmpty {
                anchors.fill: parent
                opacity: patchList.count > 0 ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: root.emptyStateFadeDuration } }
            }
        }
    }
}
