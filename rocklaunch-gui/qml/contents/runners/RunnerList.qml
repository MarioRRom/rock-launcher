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

// Runner list: one card per runner (icon, version, badges, actions).
// The rows come straight from RunnerModel, already filtered by the page.
ListView {
    id: root

    spacing: 12
    clip: true

    readonly property int scrollGutter: 8

    model: RunnerModel.runners
    ScrollBar.vertical: StyledScrollBar {}

    function sizeLabel(bytes) {
        if (bytes <= 0)
            return "?"
        const units = [" B", " KB", " MB", " GB"]
        let value = bytes
        let unit = 0
        while (value >= 1024 && unit < units.length - 1) {
            value /= 1024
            unit += 1
        }
        const digits = unit === 0 || value >= 100 ? 0 : 1
        return value.toFixed(digits) + units[unit]
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Runner Card     | |
    //  | `---------------------' |
    //  `-------------------------'

    delegate: Rectangle {
        id: runnerCard
        required property var modelData
        required property int index

        readonly property string runnerName: modelData.name
        readonly property string runnerSource: modelData.source
        readonly property bool installed: modelData.installed
        readonly property real sizeBytes: modelData.installed ? modelData.diskSize : modelData.downloadSize
        readonly property bool jobActive: RunnerJobs.busy
            && runnerCard.runnerSource === RunnerJobs.targetSource
            && runnerCard.runnerName === RunnerJobs.targetName

        // Icon by source
        states: [
            State {
                name: "proton"
                when: runnerCard.runnerSource === "proton-ge-custom"
                PropertyChanges {
                    target: runnerIcon
                    icon: "glass-full"
                    color: Theme.mauve
                }
            },
            State {
                name: "cachy"
                when: runnerCard.runnerSource === "Proton-CachyOS"
                PropertyChanges {
                    target: runnerIcon
                    icon: "cachyos"
                    color: Theme.sky
                }
            }
        ]

        // The gutter reserves room for the scrollbar; contentHeight does not
        // depend on width, so this cannot become a binding loop.
        width: ListView.view.width
            - (root.contentHeight > root.height ? root.scrollGutter : 0)
        height: 80
        radius: 12
        color: Theme.surface0


        //  .-------------------------.
        //  | .---------------------. |
        //  | |     Card Layout     | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            // Icon
            SvgIcon {
                id: runnerIcon
                Layout.alignment: Qt.AlignVCenter
                icon: "glass-full"
                color: Theme.mauve
                size: parent.height - 24
            }

            // Runner Info
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 4

                // Runner version
                Text {
                    text: runnerCard.runnerName
                    font.pixelSize: 18
                    color: Theme.text
                }

                // Runner badges (size + installed, side by side)
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignLeft

                    // Runner size
                    TextBadge {
                        text: root.sizeLabel(runnerCard.sizeBytes)
                        bgColor: Theme.blue
                        textColor: Theme.base
                        size: 20
                    }

                    // Installed check
                    TextBadge {
                        text: "Installed"
                        bgColor: Theme.green
                        textColor: Theme.base
                        visible: runnerCard.installed
                    }
                }
            }

            // Separator
            Item { Layout.fillWidth: true }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |     Status Row      | |
            //  | `---------------------' |
            //  `-------------------------'

            RowLayout {
                id: statusRow
                spacing: 8
                opacity: runnerCard.jobActive && RunnerJobs.stage !== "" ? 1 : 0
                visible: statusRow.opacity > 0
                Behavior on opacity { NumberAnimation { duration: 250 } }

                // Status Text
                Text {
                    id: statusText
                    text: RunnerJobs.stage
                    font.pixelSize: 14
                    color: Theme.subtext0
                }

                // Progress bar
                StyledProgressBar {
                    id: progressBar
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 240
                    to: 100 // Core reports 0-100, not a fraction.
                    value: RunnerJobs.percent
                    opacity: RunnerJobs.determinate ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // percentage text
                Text {
                    id: percentText
                    opacity: RunnerJobs.determinate ? 1 : 0
                    text: Math.round(RunnerJobs.percent) + "%"
                    font.pixelSize: 14
                    color: Theme.subtext0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |   Action Buttons    | |
            //  | `---------------------' |
            //  `-------------------------'

            // Download / Cancel button
            IconButton {
                visible: !runnerCard.installed
                enabled: runnerCard.jobActive ? RunnerJobs.cancellable : !RunnerJobs.busy
                Layout.alignment: Qt.AlignVCenter
                icon: runnerCard.jobActive ? "x" : "download"
                iconColor: runnerCard.jobActive ? Theme.red : Theme.text
                size: parent.height - 36
                borderRadius: 8
                onClicked: {
                    if (runnerCard.jobActive)
                        RunnerJobs.cancel()
                    else
                        RunnerJobs.install(runnerCard.runnerSource, runnerCard.runnerName)
                }
            }

            // Delete button
            IconButton {
                visible: runnerCard.installed
                enabled: !RunnerJobs.busy
                Layout.alignment: Qt.AlignVCenter
                icon: "trash"
                size: parent.height - 36
                borderRadius: 8
                onClicked: RunnerJobs.remove(runnerCard.runnerSource, runnerCard.runnerName)
            }
        }
    }
}