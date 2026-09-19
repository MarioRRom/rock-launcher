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
import "../contents/launch"
import "../contents/runners"

Rectangle {
    id: root
    color: "transparent"

    property string currentPage: "launch"


    //  .-------------------------.
    //  | .---------------------. |
    //  | | Launch Game Section | |
    //  | `---------------------' |
    //  `-------------------------'

    Launch {
        anchors.fill: parent
        visible: root.currentPage === "launch"
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | | Runner List Section | |
    //  | `---------------------' |
    //  `-------------------------'

    Runners {
        anchors.fill: parent
        visible: root.currentPage === "runners"
    }
}
