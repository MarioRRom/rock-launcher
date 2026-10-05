//--------------------------------------------------------------
//  ____   ___   ____ _  ___        _   _   _ _   _  ____ _   _ 
// |  _ \ / _ \ / ___| |/ / |      / \ | | | | \ | |/ ___| | | |
// | |_) | | | | |   | ' /| |     / _ \| | | |  \| | |   | |_| |
// |  _ <| |_| | |___| . \| |___ / ___ \ |_| | |\  | |___|  _  |
// |_| \_\\___/ \____|_|\_\_____/_/   \_\___/|_| \_|\____|_| |_|
//               a linux launcher for rock games
//          https://github.com/MarioRRom/rock-launcher
//--------------------------------------------------------------


//  A styled TextField rather than Qt's SearchField: the specific design we want
//  here cannot be expressed through SearchField.


//  .-------------------------.
//  | .---------------------. |
//  | |   Import Modules    | |
//  | `---------------------' |
//  `-------------------------'

// Qt Imports
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// Config
import RockLaunch.Gui

TextField {
    id: root

    // Public API
    property color bgColor: Theme.surface0
    property color textColor: Theme.text
    property color placeholderColor: Theme.subtext0
    property color iconColor: Theme.subtext0
    property color focusBorderColor: Theme.blue
    property int fontSize: 16
    property int iconSize: 16
    property int iconInset: 12
    property int borderRadius: 12
    property int borderWidth: 1

    signal clearButtonPressed()

    font.pixelSize: fontSize
    selectByMouse: true

    color: root.textColor
    placeholderTextColor: root.placeholderColor
    selectionColor: root.focusBorderColor
    selectedTextColor: Theme.crust

    leftPadding: root.iconInset * 2 + root.iconSize
    rightPadding: clearIcon.visible ? root.iconInset * 2 + root.iconSize : root.iconInset

    Keys.onEscapePressed: root.focus = false


    //  .-------------------------.
    //  | .---------------------. |
    //  | |      Background     | |
    //  | `---------------------' |
    //  `-------------------------'

    background: Rectangle {
        color: root.bgColor
        radius: root.borderRadius
        border.width: root.borderWidth
        border.color: root.activeFocus ? root.focusBorderColor : "transparent"
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |        Icons        | |
    //  | `---------------------' |
    //  `-------------------------'

    SvgIcon {
        z: 1
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: root.iconInset
        icon: "search"
        size: root.iconSize
        color: root.iconColor
    }

    SvgIcon {
        id: clearIcon
        z: 1
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: root.iconInset
        icon: "x"
        size: root.iconSize
        color: clearHover.hovered ? root.textColor : root.iconColor
        visible: root.text.length > 0

        HoverHandler { id: clearHover }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.clear()
                root.clearButtonPressed()
            }
        }
    }
}
