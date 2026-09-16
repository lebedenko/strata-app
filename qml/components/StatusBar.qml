import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    required property var dbusClient
    property bool isRefreshing: false

    implicitHeight: 36
    color: palette.dark.hslLightness > 0.5 ? "#f1f5f9" : "#18181b"
    border.color: palette.dark.hslLightness > 0.5 ? "#e2e8f0" : "#27272a"
    border.width: 1

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

            Text {
                text: root.dbusClient && root.dbusClient.connected ? 
                      root.dbusClient.deviceName + " (" + root.dbusClient.deviceNode + ")" : 
                      qsTr("Keyboard Disconnected")
                font.pixelSize: 11
                font.bold: true
                color: palette.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Build ID
        Text {
            visible: root.dbusClient && root.dbusClient.connected && root.dbusClient.buildId.length > 0
            text: qsTr("Build: ") + root.dbusClient.buildId
            font.pixelSize: 11
            color: palette.placeholderText
            anchors.verticalCenter: parent.verticalCenter
        }

        // Cache badge
        Rectangle {
            visible: root.dbusClient && root.dbusClient.connected
            width: cacheText.implicitWidth + 12
            height: 18
            radius: 4
            color: root.dbusClient.isCached ? "#0ea5e9" : "#f59e0b"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: cacheText
                text: root.dbusClient.isCached ? qsTr("Cached") : qsTr("Live")
                anchors.centerIn: parent
                font.pixelSize: 10
                font.bold: true
                color: "#ffffff"
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
            color: palette.dark.hslLightness > 0.5 ? "#e2e8f0" : "#27272a"
            border.color: "#38bdf8"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: activeLayerText
                text: qsTr("HW Active: ") + (root.dbusClient ? root.dbusClient.activeLayerName : "")
                anchors.centerIn: parent
                font.pixelSize: 11
                font.bold: true
                color: "#38bdf8"
            }
        }

        // Refresh button
        Button {
            text: qsTr("Refresh")
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 11
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
            font.pixelSize: 11
            onClicked: {
                if (root.dbusClient) {
                    root.dbusClient.clearCache();
                }
            }
        }
    }
}
