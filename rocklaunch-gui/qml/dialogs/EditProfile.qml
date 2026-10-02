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
import "../components"
import RockLaunch.Gui

Rectangle {
    id: root

    readonly property var payload: DialogController.editProfile.payload
    readonly property bool isNew: payload["profileNew"] === true
    readonly property string profileId: payload["profile"] !== undefined ? payload["profile"] : ""
    readonly property bool nameValid: ProfileModel.nameValid(nameEdit.text)

    readonly property int margin: 20

    Component.onCompleted: loadRunner()

    function loadRunner() {
        const profile = ProfileModel.profileEntry(root.profileId)
        runnerPicker.currentRunner = profile.runnerName
        runnerPicker.currentRunnerSource = profile.runnerSource
    }

    implicitWidth: 620
    implicitHeight: dialogColumn.implicitHeight + margin * 2
    radius: 14
    color: Theme.base
    clip: true


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Popup Heart     | |
    //  | `---------------------' |
    //  `-------------------------'

    ColumnLayout {
        id: dialogColumn
        anchors.fill: parent
        anchors.margins: root.margin
        spacing: 18


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Header        | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.isNew ? "New Profile" : "Edit Profile"
                font.pixelSize: 18
                font.bold: true
                color: Theme.text
            }

            SvgIcon {
                id: profileHelp
                icon: "help-circle"
                size: 20
                color: helpArea.containsMouse ? Theme.text : Theme.subtext0

                MouseArea {
                    id: helpArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    StyledTooltip {
                        visible: helpArea.containsMouse
                        x: profileHelp.width + 4 - width
                        y: profileHelp.height + 6
                        delay: 200
                        label: "Info:"
                        labelColor: Theme.blue
                        implicitWidth: 300
                        text: "A profile is one installation of the game: it has its own prefix and configuration. By design, a game path belongs to exactly one profile."
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 5


            //  .-------------------------.
            //  | .---------------------. |
            //  | |    Profile Name     | |
            //  | `---------------------' |
            //  `-------------------------'

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "Profile Name"
                    font.pixelSize: 16
                    color: Theme.text
                }

                TextField {
                    id: nameEdit
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.text
                    placeholderText: root.isNew ? ProfileModel.freeProfileId : root.profileId
                    placeholderTextColor: Theme.subtext0
                    font.pixelSize: 16
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12

                    // First open: the Loader had just created this item. Later
                    // opens reuse the live item, and Connections below refills.
                    Component.onCompleted: nameEdit.text = root.isNew ? "" : ProfileModel.profileName(root.profileId)

                    background: Rectangle {
                        color: Theme.surface0
                        radius: 12
                        border.width: 1
                        border.color: !root.nameValid ? Theme.red : nameEdit.activeFocus ? Theme.blue : "transparent"
                    }
                }
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |       Runner        | |
            //  | `---------------------' |
            //  `-------------------------'

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "Runner"
                    font.pixelSize: 16
                    color: Theme.text
                }

                ExpandableList {
                    id: runnerPicker

                    property string currentRunner: ""
                    property string currentRunnerSource: ""

                    readonly property string currentLabel: currentRunner === ""
                        ? "wine / proton"
                        : currentRunner + " (" + currentRunnerSource + ")"

                    Layout.fillWidth: true
                    headerText: currentLabel
                    headerHeight: 38
                    headerTextSize: 14
                    headerTextColor: Theme.text
                    headerLeftMargin: 12
                    headerRightMargin: 12
                    headerIconSize: 16
                    boxColor: Theme.surface0
                    boxBorderWidth: 1
                    expandedHeight: 116

                    listModel: RunnerModel.installedRunners
                    listDelegate: IconTextButton {
                        required property var modelData

                        width: ListView.view.width
                        text: modelData.name + " (" + modelData.source + ")"
                        icon: ({ "proton-ge-custom": "glass-full",
                            "Proton-CachyOS": "cachyos" }[modelData.source]
                        ) || "glass-full"
                        size: 36
                        textSize: 14
                        borderRadius: 10
                        textColorActive: Theme.mauve
                        bgColorActive: "transparent"
                        actived: modelData.name === runnerPicker.currentRunner
                            && modelData.source === runnerPicker.currentRunnerSource
                        onClicked: {
                            runnerPicker.currentRunner = modelData.name
                            runnerPicker.currentRunnerSource = modelData.source
                            runnerPicker.actived = false
                        }
                    }
                }
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |     Game Path       | |
            //  | `---------------------' |
            //  `-------------------------'

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "Game Path"
                    font.pixelSize: 16
                    color: Theme.text
                }

                TextField {
                    id: pathEdit
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.text
                    placeholderText: "Game installation path"
                    placeholderTextColor: Theme.subtext0
                    font.pixelSize: 16
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12 + pathFolderBtn.width + pathSteamBtn.width + 2

                    background: Rectangle {
                        color: Theme.surface0
                        radius: 12
                        border.width: 1
                        border.color: pathEdit.activeFocus ? Theme.yellow : "transparent"

                        // Open the game folder
                        IconButton {
                            id: pathFolderBtn
                            anchors.right: pathSteamBtn.left
                            anchors.verticalCenter: parent.verticalCenter
                            icon: "folder"
                            size: parent.height - 4
                            borderRadius: 10
                            iconMargin: 10
                            bgColor: "transparent"
                            onClicked: {
                                // TODO: set the game install folder
                            }
                        }

                        // Open the game on Steam
                        IconButton {
                            id: pathSteamBtn
                            anchors.right: parent.right
                            anchors.rightMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            icon: "steam"
                            size: parent.height - 4
                            iconMargin: 10
                            borderRadius: 10
                            bgColor: "transparent"
                            onClicked: {
                                // TODO: get the game from steam
                            }
                        }
                    }
                }
            }
        }


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Footer        | |
        //  | `---------------------' |
        //  `-------------------------'

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            IconTextButton {
                visible: !root.isNew
                text: "Delete Profile"
                icon: "trash"
                size: 34

                bgColor: Theme.red
                bgHoverColor: Qt.lighter(Theme.red, 1.08)
                bgPressedColor: Qt.lighter(Theme.red, 1.16)
                textColor: Theme.base
                onClicked: {
                    if (ProfileModel.removeProfile(root.profileId))
                        DialogController.editProfile.close()
                }
            }

            // Separator
            Item { Layout.fillWidth: true }

            IconTextButton {
                text: "Cancel"
                icon: "x"
                size: 34
                bgColor: Theme.surface0
                bgHoverColor: Theme.surface1
                bgPressedColor: Theme.surface2
                onClicked: DialogController.editProfile.close()
            }

            IconTextButton {
                text: "Save"
                icon: "device-floppy"
                size: 34
                // close() leaves this clickable through the fade-out, and a
                // second click would create a second profile.
                enabled: root.nameValid && DialogController.currentDialog === "editProfile"
                bgColor: Theme.green
                bgHoverColor: Qt.lighter(Theme.green, 1.08)
                bgPressedColor: Qt.lighter(Theme.green, 1.16)
                textColor: Theme.base
                onClicked: {
                    if (ProfileModel.saveProfile(
                        root.profileId, nameEdit.text,
                        runnerPicker.currentRunner, runnerPicker.currentRunnerSource) !== "")
                        DialogController.editProfile.close()
                }
            }
        }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |      Connections    | |
    //  | `---------------------' |
    //  `-------------------------'

    Connections {
        target: DialogController.editProfile.payload
        function onValueChanged(key, value) {
            // open() clears the map before refilling it; skip that pass.
            if (value === undefined)
                return
            nameEdit.text = root.isNew ? "" : ProfileModel.profileName(root.profileId)
            root.loadRunner()
        }
    }
}
