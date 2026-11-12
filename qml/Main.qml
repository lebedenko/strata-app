pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "components"

ApplicationWindow {
    id: window

    required property var trayManager
    required property var configManager
    required property var strataDBusClient
    required property var strataKeymapModel

    visible: true
    title: qsTr("Strata - Keyboard Visualizer")

    width: window.configManager ? window.configManager.windowWidth : 980
    height: window.configManager ? window.configManager.windowHeight : 520
    minimumWidth: 840
    minimumHeight: 460

    onClosing: function (close) {
        if (window.trayManager && window.trayManager.canHideToTray) {
            close.accepted = false;
            window.hide();
        }
    }

    // Top Navigation & Header Bar
    header: ToolBar {
        id: headerBar

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

            Label {
                text: qsTr("STRATA")
                font.pointSize: 12
                font.bold: true
                font.letterSpacing: 2
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Layer Switcher in Center
        LayerTabBar {
            id: layerTabs
            dbusClient: window.strataDBusClient
            anchors.centerIn: parent
        }

        // Right Header Controls
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            ComboBox {
                id: deviceSelector
                visible: window.strataDBusClient && window.strataDBusClient.availableDevices && window.strataDBusClient.availableDevices.length > 1
                anchors.verticalCenter: parent.verticalCenter
                model: window.strataDBusClient ? window.strataDBusClient.availableDevices : []
                textRole: "name"
                font.pointSize: 9
                implicitHeight: 28

                currentIndex: {
                    if (!window.strataDBusClient || !window.strataDBusClient.availableDevices)
                        return 0;
                    for (let i = 0; i < window.strataDBusClient.availableDevices.length; ++i) {
                        if (window.strataDBusClient.availableDevices[i].id === window.strataDBusClient.deviceId)
                            return i;
                    }
                    return 0;
                }

                onActivated: function (index) {
                    if (window.strataDBusClient && window.strataDBusClient.availableDevices && index >= 0 && index < window.strataDBusClient.availableDevices.length) {
                        window.strataDBusClient.selectDevice(window.strataDBusClient.availableDevices[index].id);
                    }
                }
            }

            CheckBox {
                text: qsTr("Show Key IDs")
                checked: window.configManager ? window.configManager.showKeyPositions : false
                font.pointSize: 9
                anchors.verticalCenter: parent.verticalCenter
                onToggled: {
                    if (window.configManager) {
                        window.configManager.showKeyPositions = checked;
                    }
                }
            }
        }
    }

    // Main Keyboard Visualizer Area
    Item {
        id: visualizerArea
        anchors.fill: parent
        anchors.margins: 12

        Loader {
            id: keyboardLoader
            anchors.centerIn: parent
            sourceComponent: (window.strataKeymapModel && window.strataKeymapModel.layoutData && window.strataKeymapModel.layoutData.type === "macropad") ? macropadComponent : splitComponent
        }

        Component {
            id: splitComponent
            SplitKeyboard {
                keymapModel: window.strataKeymapModel
                showPositions: window.configManager ? window.configManager.showKeyPositions : false
            }
        }

        Component {
            id: macropadComponent
            MacropadKeyboard {
                keymapModel: window.strataKeymapModel
                showPositions: window.configManager ? window.configManager.showKeyPositions : false
            }
        }
    }

    // Status Footer
    footer: StatusBar {
        id: statusBar
        dbusClient: window.strataDBusClient
    }
}
