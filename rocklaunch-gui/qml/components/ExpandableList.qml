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

Rectangle {
    id: root

    // List
    property var listModel: []
    property Component listDelegate
    property real expandedHeight: 164

    // Selection — the entry matching currentKey is kept in view
    property string currentKey: ""
    property string keyProperty: "id"

    // Header
    property string headerText: ""
    property int headerHeight: 34
    property int headerTextSize: 16
    property color headerTextColor: Theme.base
    property int headerLeftMargin: 16
    property int headerRightMargin: 12
    property string headerIcon: "player-play"
    property int headerIconSize: 18

    // Footer button
    property bool footButton: false
    property string footText: ""
    property string footIcon: ""
    property int footSize: 35
    property int footTextSize: 14
    property int footRadius: 10

    // Colors
    property color boxColor: Theme.mauve
    property int boxBorderWidth: 0
    property color boxBorderColor: Theme.mauve
    property color innerColor: Theme.mantle
    property int innerMargin: 2
    property color separatorColor: Theme.base

    // State
    property bool actived: false
    readonly property int expandDuration: 150

    signal footClicked()

    implicitWidth: 270
    implicitHeight: actived ? header.height + expandedHeight : header.height
    radius: 12
    color: boxColor
    border.width: boxBorderWidth
    border.color: actived ? boxBorderColor : "transparent"
    clip: true

    Behavior on implicitHeight {
        NumberAnimation { duration: root.expandDuration; easing.type: Easing.InOutQuad }
    }

    // Brings an entry into view, e.g. the selected one.
    function scrollToIndex(index) {
        listView.positionViewAtIndex(index, ListView.Center)
    }

    function scrollToKey() {
        const list = root.listModel
        for (let i = 0; i < list.length; ++i) {
            if (list[i][root.keyProperty] === root.currentKey) {
                root.scrollToIndex(i)
                return
            }
        }
    }

    onCurrentKeyChanged: scrollToKey()


    //  .-------------------------.
    //  | .---------------------. |
    //  | |       Header        | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.headerHeight
        radius: 12
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.headerLeftMargin
            anchors.rightMargin: root.headerRightMargin
            spacing: 8

            Text {
                text: root.headerText
                font.pixelSize: root.headerTextSize
                color: root.headerTextColor
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
            }

            SvgIcon {
                icon: root.headerIcon
                Layout.alignment: Qt.AlignVCenter
                size: root.headerIconSize
                color: root.headerTextColor
                rotation: root.actived ? 270 : 90

                Behavior on rotation {
                    NumberAnimation { duration: root.expandDuration }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.actived = !root.actived
        }
    }


    //  .-------------------------.
    //  | .---------------------. |
    //  | |       Content       | |
    //  | `---------------------' |
    //  `-------------------------'

    Rectangle {
        id: container
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.margins: root.innerMargin
        color: root.innerColor
        radius: 12
        clip: true
        visible: root.implicitHeight > header.height

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.innerMargin
            spacing: 0


            //  .-------------------------.
            //  | .---------------------. |
            //  | |         List        | |
            //  | `---------------------' |
            //  `-------------------------'

            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                model: root.listModel
                delegate: root.listDelegate
            }

            // separator
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: root.separatorColor
                visible: root.footButton
            }


            //  .-------------------------.
            //  | .---------------------. |
            //  | |    Footer Button    | |
            //  | `---------------------' |
            //  `-------------------------'

            IconTextButton {
                Layout.fillWidth: true
                visible: root.footButton
                text: root.footText
                icon: root.footIcon
                size: root.footSize
                textSize: root.footTextSize
                borderRadius: root.footRadius
                onClicked: root.footClicked()
            }
        }
    }
}
