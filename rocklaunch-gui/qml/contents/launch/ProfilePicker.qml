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
import RockLaunch.Gui

Rectangle {
    id: root

    property bool actived: false
    property string currentProfile: ProfileModel.currentProfile
    property var profiles: ProfileModel.profiles

    signal profileSelected(string profile)
    signal createProfile()

    implicitWidth: 240
    implicitHeight: actived ? header.height + 150 : header.height
    radius: 12
    color: "#cba6f7"
    clip: true

    Behavior on implicitHeight {
        NumberAnimation { duration: 150; easing.type: Easing.InOutQuad }
    }

    //  .-------------------------.
    //  | .---------------------. |
    //  | |    Button/Header    | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 34
        radius: 12
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 8

            Text {
                text: root.currentProfile
                font.pixelSize: 16
                color: "#181825"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
            }

            SvgIcon {
                icon: "player-play"
                Layout.alignment: Qt.AlignVCenter
                size: 18
                color: "#181825"
                rotation: root.actived ? 270 : 90

                Behavior on rotation {
                    NumberAnimation { duration: 150 }
                }
            }
        }

        MouseArea {
            id: headerHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.actived = !root.actived
        }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Profile Content   | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        id: container
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.margins: 2
        color: "#181825"
        radius: 12
        clip: true
        visible: root.implicitHeight > header.height

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 0

            //  .-------------------------.
            //  | .---------------------. |
            //  | |    Profile List     | |
            //  | `---------------------' |
            //  `-------------------------'

            ListView {
                id: profileList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                model: root.profiles

                // Profile card
                delegate: Rectangle {
                    id: profileCard
                    required property string modelData
                    required property int index

                    width: ListView.view.width
                    height: 40
                    radius: 8
                    color: delegateHover.pressed ? "#45475a" : delegateHover.containsMouse ? "#313244" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12
                        spacing: 8

                        Text {
                            text: profileCard.modelData
                            font.pixelSize: 14
                            color: profileCard.modelData === root.currentProfile ? "#a6e3a1" : "#cdd6f4"
                            Layout.fillWidth: true
                        }

                        SvgIcon {
                            visible: profileCard.modelData === root.currentProfile
                            icon: "player-play"
                            size: 16
                            color: "#a6e3a1"
                        }
                    }

                    MouseArea {
                        id: delegateHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            ProfileModel.currentProfile = profileCard.modelData
                            root.profileSelected(profileCard.modelData)
                            root.actived = false
                        }
                    }
                }
            }

            // separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1e1e2e"
            }

            //  .-------------------------.
            //  | .---------------------. |
            //  | |Create profile button| |
            //  | `---------------------' |
            //  `-------------------------'

            IconTextButton {
                Layout.fillWidth: true
                text: "Create new profile"
                size: 35
                onClicked: {
                    ProfileModel.createProfile()
                    root.createProfile()
                }
            }
        }
    }
}
