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
import RockLaunch.Gui

Window {
    id: root
    visible: true
    width: 1280
    height: 720
    minimumWidth: 1280
    minimumHeight: 720
    title: "RockLauncher"
    color: Theme.base


    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Layout Configs    | |
    //  | `---------------------' |
    //  `-------------------------'

    property int sidebarWidth: 240
    property int topbarHeight: 74
    readonly property int showHideLayoutDuration: 250

    // Internal states
    property string currentPage: "launch"
    readonly property bool topbarVisible: currentPage === "launch" || currentPage === "patches"


    // Horizontal Layout
    RowLayout {
        anchors.fill: parent
        spacing: 0


        //  .-------------------------.
        //  | .---------------------. |
        //  | |       Sidebar       | |
        //  | `---------------------' |
        //  `-------------------------'

        SideBar {
            Layout.fillHeight: true
            Layout.preferredWidth: root.sidebarWidth
            currentPage: root.currentPage
            onPageChanged: (page) => root.currentPage = page
        }

        // Vertical Layout
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0


            //  .-------------------------.
            //  | .---------------------. |
            //  | |     Game Picker     | |
            //  | `---------------------' |
            //  `-------------------------'

            TopBar {
                Layout.fillWidth: true

                visible: Layout.preferredHeight > 0
                Layout.preferredHeight: root.topbarVisible ? root.topbarHeight : 0
                Behavior on Layout.preferredHeight { NumberAnimation { duration: root.showHideLayoutDuration } }
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |    Main Content     | |
            //  | `---------------------' |
            //  `-------------------------'

            PageHost {
                Layout.fillHeight: true
                Layout.fillWidth: true
                currentPage: root.currentPage
            }
        }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |    Overlay Layer    | |
    //  | `---------------------' |
    //  `-------------------------'

    // Dialogs is the overlay layer, above everything else.
    Dialogs {
        anchors.fill: parent
        z: 50 // above the layout content
    }
}
