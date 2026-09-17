import QtQuick
import QtQuick.Controls

ToolBar {
    id: root

    required property var dbusClient
    property bool isRefreshing: false

    implicitHeight: 36

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // Connection indicator
        Row {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: root.dbusClient && root.dbusClient.connected ? "#10b981" : "#ef4444"
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: root.dbusClient && root.dbusClient.connected ? 
                      root.dbusClient.deviceName + " (" + root.dbusClient.deviceNode + ")" : 
                      qsTr("Keyboard Disconnected")
                font.pointSize: 9
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Build ID
        Label {
            visible: root.dbusClient && root.dbusClient.connected && root.dbusClient.buildId.length > 0
            text: qsTr("Build: ") + root.dbusClient.buildId
            font.pointSize: 9
            color: palette.placeholderText
            anchors.verticalCenter: parent.verticalCenter
        }

        // Cache badge
        Rectangle {
            visible: root.dbusClient && root.dbusClient.connected
            width: cacheText.implicitWidth + 12
            height: 18
            radius: 4
            color: root.dbusClient.isCached ? palette.highlight : palette.mid
            anchors.verticalCenter: parent.verticalCenter

            Label {
                id: cacheText
                text: root.dbusClient.isCached ? qsTr("Cached") : qsTr("Live")
                anchors.centerIn: parent
                font.pointSize: 8
                font.bold: true
                color: root.dbusClient.isCached ? palette.highlightedText : palette.buttonText
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        // Live Active Layer Indicator
        Rectangle {
            visible: root.dbusClient && root.dbusClient.connected
            width: activeLayerText.implicitWidth + 16
            height: 22
            radius: 6
            color: palette.base
            border.color: palette.highlight
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Label {
                id: activeLayerText
                text: qsTr("HW Active: ") + (root.dbusClient ? root.dbusClient.activeLayerName : "")
                anchors.centerIn: parent
                font.pointSize: 9
                font.bold: true
                color: palette.highlight
            }
        }

        // Refresh button
        Button {
            text: qsTr("Refresh")
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 9
            onClicked: {
                if (root.dbusClient) {
                    root.dbusClient.refreshKeymap();
                }
            }
        }

        // Clear Cache button
        Button {
            text: qsTr("Clear Cache")
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: 9
            onClicked: {
                if (root.dbusClient) {
                    root.dbusClient.clearCache();
                }
            }
        }
    }
}
