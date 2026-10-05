//--------------------------------------------------------------
//  ____   ___   ____ _  ___        _   _   _ _   _  ____ _   _ 
// |  _ \ / _ \ / ___| |/ / |      / \ | | | | \ | |/ ___| | | |
// | |_) | | | | |   | ' /| |     / _ \| | | |  \| | |   | |_| |
// |  _ <| |_| | |___| . \| |___ / ___ \ |_| | |\  | |___|  _  |
// |_| \_\\___/ \____|_|\_\_____/_/   \_\___/|_| \_|\____|_| |_|
//               a linux launcher for rock games
//          https://github.com/MarioRRom/rock-launcher
//--------------------------------------------------------------


// A ToolTip that prefixes its text with a separately colored label, for the
// "Warning:" or "Tip:" lead that has to stand out from the message itself.


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

ToolTip {
    id: root

    // Public API / normal Tooltip functions are usable
    property string label: "" // e.g. "Warning:", "Tip:" or "Info:"
    property color labelColor: Theme.red

    readonly property int enterDuration: 200
    readonly property int exitDuration: 100


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Customizations    | |
    //  | `---------------------' |
    //  `-------------------------'

    // In-Out fade animations
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: root.enterDuration; easing.type: Easing.OutQuad }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: root.exitDuration; easing.type: Easing.InQuad }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Tooltip Content   | |
    //  | `---------------------' |
    //  `-------------------------'

    contentItem: Text {
        text: "<font color='" + root.labelColor + "'>" + root.label + "</font> " + root.text
        textFormat: Text.StyledText
        color: Theme.text
        font.pixelSize: 14
        wrapMode: Text.WordWrap
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Background      | |
    //  | `---------------------' |
    //  `-------------------------'

    background: Rectangle {
        color: Theme.surface1
        radius: 8
        border.width: 1
        border.color: Theme.surface2
    }
}
