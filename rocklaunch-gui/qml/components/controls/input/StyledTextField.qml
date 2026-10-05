//--------------------------------------------------------------
//  ____   ___   ____ _  ___        _   _   _ _   _  ____ _   _ 
// |  _ \ / _ \ / ___| |/ / |      / \ | | | | \ | |/ ___| | | |
// | |_) | | | | |   | ' /| |     / _ \| | | |  \| | |   | |_| |
// |  _ <| |_| | |___| . \| |___ / ___ \ |_| | |\  | |___|  _  |
// |_| \_\\___/ \____|_|\_\_____/_/   \_\___/|_| \_|\____|_| |_|
//               a linux launcher for rock games
//          https://github.com/MarioRRom/rock-launcher
//--------------------------------------------------------------


// A styled TextField with an optional right side indicator slot for action
// buttons. Border color is the caller's: this component applies no state logic.


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
    property color borderColor: "transparent"
    property int fontSize: 16
    property int inset: 12
    property int borderRadius: 12
    property int borderWidth: 1
    property Component indicator

    implicitHeight: 38

    font.pixelSize: fontSize
    selectByMouse: true

    color: root.textColor
    placeholderTextColor: root.placeholderColor
    selectionColor: Theme.blue
    selectedTextColor: Theme.crust

    leftPadding: root.inset
    rightPadding: indicatorSlot.width > 0 ? root.inset + indicatorSlot.width : root.inset


    //  .-------------------------.
    //  | .---------------------. |
    //  | |      Background     | |
    //  | `---------------------' |
    //  `-------------------------'

    background: Rectangle {
        color: root.bgColor
        radius: root.borderRadius
        border.width: root.borderWidth
        border.color: root.borderColor
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |      Indicator      | |
    //  | `---------------------' |
    //  `-------------------------'

    Loader {
        id: indicatorSlot
        z: 1
        sourceComponent: root.indicator
        readonly property var content: item
        width: content ? content.width : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
}
