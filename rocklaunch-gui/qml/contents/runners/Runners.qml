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
    color: "transparent"

    // Core names a source after its repo, so these two strings must match
    // Runners::Repos(); the label is display only.
    readonly property string geProtonSource: "proton-ge-custom"
    readonly property string cachyosSource: "Proton-CachyOS"

    readonly property int emptyStateFadeDuration: 250

    Component.onCompleted: RunnerModel.currentSource = geProtonSource

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
                actived: RunnerModel.currentSource === root.geProtonSource
                text: "GE-Proton"
                icon: "glass-full"
                textColorActive: Theme.mauve
                size: parent.height
                onClicked: RunnerModel.currentSource = root.geProtonSource
            }

            // Proton-Cachyos button
            IconTextButton {
                actived: RunnerModel.currentSource === root.cachyosSource
                text: "Proton-Cachyos"
                icon: "cachyos"
                textColorActive: Theme.sky
                size: parent.height
                onClicked: RunnerModel.currentSource = root.cachyosSource
            }

            // separator
            Item { Layout.fillWidth: true }

            // Search bar
            StyledSearchField {
                Layout.preferredWidth: 300
                Layout.preferredHeight: headerRow.Layout.preferredHeight
                Layout.alignment: Qt.AlignVCenter
                placeholderText: "Search for runners"
                onTextChanged: RunnerModel.search = text
            }

            // reload button
            IconButton {
                icon: "refresh"
                size: parent.height
                enabled: !RunnerJobs.busy
                onClicked: RunnerJobs.refresh(true)

                HoverHandler { id: reloadHover }

                StyledTooltip {
                    visible: reloadHover.hovered
                    label: "Warning:"
                    labelColor: Theme.red
                    delay: 200
                    implicitWidth: 300
                    text: "Reload the releases from GitHub. Requests are limited to 60 per hour."
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
        //  | |     Runner List     | |
        //  | `---------------------' |
        //  `-------------------------'

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Runner list
            RunnerList {
                id: runnerList
                anchors.fill: parent
                opacity: runnerList.count > 0 ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: root.emptyStateFadeDuration } }
            }

            RunnerEmpty {
                anchors.fill: parent
                opacity: runnerList.count > 0 ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: root.emptyStateFadeDuration } }
            }
        }
    }
}
