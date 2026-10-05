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

// Config
import RockLaunch.Gui

Item {
    id: root

    property string currentPage: "launch"
    property int pageChangeDuration: 250


    //  .-------------------------.
    //  | .---------------------. |
    //  | | Launch Game Section | |
    //  | `---------------------' |
    //  `-------------------------'

    Launch {
        z: root.currentPage === "launch" ? 1 : 0
        opacity: root.currentPage === "launch" ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: root.pageChangeDuration } }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | | Runner List Section | |
    //  | `---------------------' |
    //  `-------------------------'

    Runners {
        z: root.currentPage === "runners" ? 1 : 0
        opacity: root.currentPage === "runners" ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: root.pageChangeDuration } }
    }
}
