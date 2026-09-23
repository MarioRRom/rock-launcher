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
import "layout"
import RockLaunch.Gui

Window {
    id: root
    visible: true
    width: 1280
    height: 720
    minimumWidth: 1280
    minimumHeight: 720
    title: "RockLauncher"
    color: "#1e1e2e"
    

    //  .-------------------------.
    //  | .---------------------. |
    //  | |   Layout Configs    | |
    //  | `---------------------' |
    //  `-------------------------'

    property int sidebarWidth: 240
    property int topbarHeight: 74

    // Internal states
    property string currentPage: "launch"


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
                Layout.preferredHeight: root.topbarHeight
                currentGame: GameProfileModel.gameId
                onGameChanged: (game) => GameProfileModel.gameId = game
                visible: root.currentPage === "launch"
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |    Main Content     | |
            //  | `---------------------' |
            //  `-------------------------'

            Content {
                Layout.fillHeight: true
                Layout.fillWidth: true
                currentPage: root.currentPage
            }
        }
    }
}
