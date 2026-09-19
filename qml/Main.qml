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

    onClosing: function(close) {
        if (configManager && configManager.showTrayIcon) {
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
            dbusClient: strataDBusClient
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
                visible: strataDBusClient && strataDBusClient.availableDevices && strataDBusClient.availableDevices.length > 1
                anchors.verticalCenter: parent.verticalCenter
                model: strataDBusClient ? strataDBusClient.availableDevices : []
                textRole: "name"
                font.pointSize: 9
                implicitHeight: 28

                currentIndex: {
                    if (!strataDBusClient || !strataDBusClient.availableDevices) return 0;
                    for (var i = 0; i < strataDBusClient.availableDevices.length; ++i) {
                        if (strataDBusClient.availableDevices[i].id === strataDBusClient.deviceId) return i;
                    }
                    return 0;
                }

                onActivated: function(index) {
                    if (strataDBusClient && strataDBusClient.availableDevices && index >= 0 && index < strataDBusClient.availableDevices.length) {
                        strataDBusClient.selectDevice(strataDBusClient.availableDevices[index].id);
                    }
                }
            }

            CheckBox {
                text: qsTr("Show Key IDs")
                checked: configManager ? configManager.showKeyPositions : false
                font.pointSize: 9
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
        anchors.fill: parent
        anchors.margins: 12

        Loader {
            id: keyboardLoader
            anchors.centerIn: parent
            sourceComponent: (strataKeymapModel && strataKeymapModel.layoutData && strataKeymapModel.layoutData.type === "macropad") ? macropadComponent : splitComponent
        }

        Component {
            id: splitComponent
            SplitKeyboard {
                keymapModel: strataKeymapModel
                showPositions: configManager ? configManager.showKeyPositions : false
            }
        }

        Component {
            id: macropadComponent
            MacropadKeyboard {
                keymapModel: strataKeymapModel
                showPositions: configManager ? configManager.showKeyPositions : false
            }
        }
    }

    // Status Footer
    footer: StatusBar {
        id: statusBar
        dbusClient: strataDBusClient
    }
}
