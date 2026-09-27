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

ExpandableList {
    headerText: ProfileModel.currentProfileName
    listModel: ProfileModel.profiles
    currentKey: ProfileModel.currentProfile

    footButton: true
    footText: "Create new profile"
    footIcon: "plus"
    onFootClicked: DialogController.editProfile.open({ profileNew: true })


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Profile Card    | |
    //  | `---------------------' |
    //  `-------------------------'

    listDelegate: Rectangle {
        id: profileCard
        required property var modelData
        required property int index

        readonly property string profileId: modelData.id
        readonly property string title: modelData.name !== "" ? modelData.name : modelData.id
        readonly property bool selected: modelData.id === ProfileModel.currentProfile

        width: ListView.view.width
        height: 40
        radius: 10
        color: delegateHover.pressed ? Theme.surface1 : delegateHover.containsMouse ? Theme.surface0 : "transparent"

        // Profile selection
        MouseArea {
            id: delegateHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: ProfileModel.currentProfile = profileCard.profileId
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: editPencil.visible ? 0 : 10
            spacing: 8

            // Profile name
            Text {
                text: profileCard.title
                font.pixelSize: 14
                color: profileCard.selected ? Theme.green : Theme.text
                Layout.fillWidth: true
            }

            // Edit profile button
            IconButton {
                id: editPencil
                visible: profileCard.selected
                icon: "pencil"
                size: parent.height
                iconColor: Theme.surface2
                bgColor: "transparent"
                radius: profileCard.radius
                onClicked: DialogController.editProfile.open({ "profile": profileCard.profileId })
            }
        }
    }
}
