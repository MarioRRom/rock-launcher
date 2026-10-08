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
import RockLaunch.Gui

// Patch list: one card per patch. The fixed row carries the header, and the
// card grows under it to reveal the description.
ListView {
    id: root

    spacing: 12
    clip: true

    readonly property int scrollGutter: 8
    readonly property int expandDuration: 150
    property string expandedPatchId: ""

    // Placeholder rows: shape mirrors PatchPreset (+ the flags PatchManager persists),
    // minus `reversible`. `tags` is GUI-only; `operations` is one paragraph of text.
    model: [
        {
            patchId: "direct-connect",
            gameId: "rocksmith2014remastered",
            name: "Direct Connect",
            description: "Plug any audio interface or mic instead of the Real Tone Cable.",
            tags: ["audio"],
            installLevel: true,
            operations: "Edit cache.psarc in place (the original stays as cache.psarc.bak) "
                + "and set ExclusiveMode=0 and Win32UltraLowLatencyMode=0 under [Audio] "
                + "in Rocksmith.ini.",
            downloadable: false,
            downloaded: false,
            enabled: false
        },
        {
            patchId: "cdlc",
            gameId: "rocksmith2014remastered",
            name: "CDLC",
            description: "Load custom songs through the CDLC enabler DLL.",
            tags: ["mods"],
            installLevel: true,
            operations: "Copy D3DX9_42.dll next to the game executable.",
            downloadable: true,
            downloaded: false,
            enabled: false
        },
        {
            patchId: "other patch",
            gameId: "rocksmith2014remastered",
            name: "PLACEHOLDER",
            description: "hey, this is a placeholder patch",
            tags: ["audio", "mods"],
            installLevel: true,
            operations: "Copy placeholder.dll next to the game executable.",
            downloadable: true,
            downloaded: false,
            enabled: true
        }
    ]
    ScrollBar.vertical: StyledScrollBar {}


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Patch Card      | |
    //  | `---------------------' |
    //  `-------------------------'

    delegate: Rectangle {
        id: patchCard
        required property var modelData

        readonly property bool expanded: root.expandedPatchId === patchCard.modelData.patchId

        readonly property int statusFadeDuration: 250

        // TODO: take these three flags from the patch model when the backend lands.
        readonly property bool patchEnabled: modelData.enabled
        readonly property bool downloadPending: modelData.downloadable && !modelData.downloaded
        readonly property bool canRefetch: modelData.downloadable && modelData.downloaded

        readonly property int collapsedHeight: 80
        readonly property int expandedHeight: collapsedHeight
            + 6 + descriptionBlock.implicitHeight + 14
        readonly property int actionButtonWidth: 127

        // Colours must stay in sync with the header chips in Patches.qml.
        states: [
            State {
                name: "audioTag"
                when: (patchCard.modelData.tags || []).includes("audio")
                PropertyChanges {
                    patchIcon.color: Theme.mauve
                }
            },
            State {
                name: "modsTag"
                when: (patchCard.modelData.tags || []).includes("mods")
                PropertyChanges {
                    patchIcon.color: Theme.sky
                }
            }
        ]

        // The gutter reserves room for the scrollbar; contentHeight does not
        // depend on width, so this cannot become a binding loop.
        width: ListView.view.width
            - (root.contentHeight > root.height ? root.scrollGutter : 0)
        implicitHeight: patchCard.expanded ? patchCard.expandedHeight
            : patchCard.collapsedHeight
        radius: 12
        color: Theme.surface0
        clip: true

        Behavior on implicitHeight {
            NumberAnimation { duration: root.expandDuration; easing.type: Easing.InOutQuad }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expandedPatchId = patchCard.expanded ? ""
                : patchCard.modelData.patchId
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |     Card Layout     | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: patchCard.collapsedHeight
            spacing: 8

            // Icon
            SvgIcon {
                id: patchIcon
                Layout.alignment: Qt.AlignVCenter
                icon: "bandage"
                color: Theme.text
                size: headerRow.height - 24
            }
            // Patch Info
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 4

                // Patch name
                Text {
                    text: patchCard.modelData.name
                    font.pixelSize: 18
                    color: Theme.text
                }

                // Badges zone
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignLeft

                    TextBadge {
                        text: patchCard.modelData.installLevel ? "Persistent" : "Per launch"
                        bgColor: Theme.peach
                        textColor: Theme.base
                    }

                    TextBadge {
                        text: "Downloadable"
                        bgColor: Theme.blue
                        textColor: Theme.base
                        visible: patchCard.downloadPending
                    }

                    TextBadge {
                        text: "Downloaded"
                        bgColor: Theme.sky
                        textColor: Theme.base
                        visible: patchCard.modelData.downloaded && !patchCard.patchEnabled
                    }

                    TextBadge {
                        text: "Enabled"
                        bgColor: Theme.green
                        textColor: Theme.base
                        visible: patchCard.patchEnabled
                    }
                }
            }
            
            // Separator
            Item { Layout.fillWidth: true }

            // Status Text
            Text {
                id: statusText
                text: "Installing" // TODO: patch status (Downloading, Installing, etc)
                font.pixelSize: 14
                color: Theme.subtext0
                opacity: 0 // TODO: patchCard.jobActive ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: patchCard.statusFadeDuration } }
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |   Action Buttons    | |
            //  | `---------------------' |
            //  `-------------------------'

            // Settings button (only for the suported patches)
            IconButton {
                visible: false // TODO: only for the suported patches, requires a new dialog.
                Layout.alignment: Qt.AlignVCenter
                icon: "settings"
                size: headerRow.height - 36
                borderRadius: 8
            }

            // Refresh button (only for the suported patches)
            IconButton {
                visible: patchCard.canRefetch
                Layout.alignment: Qt.AlignVCenter
                icon: "refresh"
                size: headerRow.height - 36
                borderRadius: 8
                onClicked: {
                    // TODO: re-download the patch assets
                }
            }

            // Install / Apply / Remove button
            IconTextButton {
                id: actionButton
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: patchCard.actionButtonWidth
                centerContent: true
                size: headerRow.height - 36
                borderRadius: 8
                onClicked: {
                    // TODO: install, apply or remove the patch
                }

                states: [
                    State {
                        name: "remove"
                        when: patchCard.patchEnabled
                        PropertyChanges {
                            actionButton.text: "Remove"
                            actionButton.icon: "x"
                            actionButton.textColor: Theme.red
                        }
                    },
                    State {
                        name: "install"
                        when: !patchCard.patchEnabled && patchCard.downloadPending
                        PropertyChanges {
                            actionButton.text: "Install"
                            actionButton.icon: "download"
                            actionButton.textColor: Theme.blue
                        }
                    },
                    State {
                        name: "apply"
                        when: !patchCard.patchEnabled && !patchCard.downloadPending
                        PropertyChanges {
                            actionButton.text: "Apply"
                            actionButton.icon: "bandage"
                            actionButton.textColor: Theme.green
                        }
                    }
                ]
            }

            // Expand arrow
            SvgIcon {
                Layout.alignment: Qt.AlignVCenter
                Layout.margins: 8
                icon: "player-play"
                size: 18
                color: Theme.subtext1
                rotation: patchCard.expanded ? 270 : 90

                Behavior on rotation {
                    NumberAnimation { duration: root.expandDuration; easing.type: Easing.InOutQuad }
                }
            }
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |  Description & Ops  | |
        //  | `---------------------' |
        //  `-------------------------'

        ColumnLayout {
            id: descriptionBlock
            anchors.top: headerRow.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 4

            Text {
                text: "Description:"
                font.pixelSize: 16
                font.bold: true
                color: Theme.text
            }

            Text {
                Layout.fillWidth: true
                text: patchCard.modelData.description
                font.pixelSize: 14
                color: Theme.subtext0
                wrapMode: Text.WordWrap
            }

            // Activation plan only: undoing it is the inverse operation.
            Text {
                Layout.topMargin: 8
                Layout.fillWidth: true
                visible: (patchCard.modelData.operations || "").length > 0
                text: "Operations:"
                font.pixelSize: 16
                font.bold: true
                color: Theme.text
            }

            Text {
                Layout.fillWidth: true
                text: patchCard.modelData.operations
                font.pixelSize: 14
                color: Theme.subtext0
                wrapMode: Text.WordWrap
            }
        }
    }
}
