import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: window

    visible: true
    title: qsTr("Strata - Keyboard Visualizer")

    width: configManager ? configManager.windowWidth : 980
    height: configManager ? configManager.windowHeight : 520
    minimumWidth: 840
    minimumHeight: 460

    color: palette.dark.hslLightness > 0.5 ? "#f8fafc" : "#09090b"

    onClosing: function(close) {
        if (configManager && configManager.showTrayIcon) {
            close.accepted = false;
            window.hide();
        }
    }

    // Top Navigation & Header Bar
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 54
        color: palette.dark.hslLightness > 0.5 ? "#ffffff" : "#18181b"
        border.color: palette.dark.hslLightness > 0.5 ? "#e2e8f0" : "#27272a"
        border.width: 1

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            Image {
                source: "qrc:/qt/qml/Strata/desktop/strata.svg"
                width: 28
                height: 28
                sourceSize.width: 28
                sourceSize.height: 28
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("STRATA")
                font.pixelSize: 15
                font.bold: true
                font.letterSpacing: 2
                color: palette.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Layer Switcher in Center
        LayerTabBar {
            id: layerTabs
            dbusClient: strataDBusClient
            anchors.centerIn: parent
        }

        // Right Header Controls
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16

            CheckBox {
                text: qsTr("Show Key IDs")
                checked: configManager ? configManager.showKeyPositions : false
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
                onToggled: {
                    if (configManager) {
                        configManager.showKeyPositions = checked;
                    }
                }
            }
        }
    }

    // Main Keyboard Visualizer Area
    Item {
        id: visualizerArea
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        anchors.margins: 12

        SplitKeyboard {
            id: splitKeyboard
            keymapModel: strataKeymapModel
            showPositions: configManager ? configManager.showKeyPositions : false
            anchors.centerIn: parent
        }
    }

    // Status Footer
    StatusBar {
        id: statusBar
        dbusClient: strataDBusClient
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
}
