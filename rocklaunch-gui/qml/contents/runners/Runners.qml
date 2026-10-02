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
            TextField {
                id: searchBar
                Layout.preferredWidth: 300
                Layout.preferredHeight: headerRow.Layout.preferredHeight
                Layout.alignment: Qt.AlignVCenter
                color: Theme.text
                placeholderTextColor: Theme.subtext0
                background: Rectangle {
                    color: Theme.surface0
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
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }

            RunnerEmpty {
                anchors.fill: parent
                opacity: runnerList.count > 0 ? 0 : 1
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }
        }
    }
}