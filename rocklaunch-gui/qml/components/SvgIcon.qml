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
import QtQuick.Effects

Item {
    id: iconLoader

    // Public API
    property string icon: ""
    property int size: 24
    property color color: "transparent"
    property var fillMode: Image.PreserveAspectFit

    implicitWidth: size
    implicitHeight: size

    // Base icon
    Image {
        id: baseIcon
        anchors.fill: parent
        source: iconLoader.icon !== "" ? "../assets/icons/" + iconLoader.icon + ".svg" : ""
        sourceSize.width: iconLoader.size
        sourceSize.height: iconLoader.size
        fillMode: iconLoader.fillMode
        smooth: true
        antialiasing: true
        cache: true
        visible: iconLoader.color === "transparent"
    }

    // Colorized effect
    MultiEffect {
        anchors.fill: baseIcon
        source: baseIcon
        visible: iconLoader.color !== "transparent"
        colorization: 1.0
        colorizationColor: iconLoader.color
    }
}
