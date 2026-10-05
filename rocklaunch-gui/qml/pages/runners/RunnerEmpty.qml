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

Item {
    id: root

    // Refresh outranks the filter, and the filter outranks the last failure;
    // refreshFailed (not error) keeps an install failure from blaming the list.
    readonly property bool refreshing: RunnerJobs.busy && RunnerJobs.targetSource === ""
    readonly property bool searching: !root.refreshing && RunnerModel.search !== ""
    readonly property bool failed: !root.searching && RunnerJobs.refreshFailed

    readonly property color stateColor: root.refreshing ? Theme.blue
        : root.failed ? Theme.red
        : Theme.surface1
    readonly property string title: root.refreshing ? "Updating runners..."
        : root.searching ? "No runner matches “" + RunnerModel.search + "”"
        : root.failed ? "Couldn't load the release list"
        : "No runners to show"
    readonly property string detail: root.refreshing ? "Loading the release list and rescanning this machine"
        : root.searching ? "Clear the search to see every runner"
        : root.failed ? "Press refresh to try again. Details are in rocklaunch.log."
        : "Press refresh to load the release list from GitHub"


    //  .-------------------------.
    //  | .---------------------. |
    //  | |     Empty State     | |
    //  | `---------------------' |
    //  `-------------------------'

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 480)
        spacing: 16

        SvgIcon {
            Layout.alignment: Qt.AlignHCenter
            size: 200
            icon: "glass-full"
            color: root.stateColor
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.title
            font.pixelSize: 18
            color: Theme.text
        }

        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.detail
            font.pixelSize: 14
            color: Theme.subtext0
        }
    }
}
