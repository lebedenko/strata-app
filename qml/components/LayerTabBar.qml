import QtQuick
import QtQuick.Controls

TabBar {
    id: root

    required property var dbusClient

    function updateCurrentIndex() {
        if (!dbusClient || !dbusClient.layers) return;
        for (let i = 0; i < dbusClient.layers.length; ++i) {
            if (dbusClient.layers[i].index === dbusClient.selectedLayerIndex) {
                if (currentIndex !== i) {
                    currentIndex = i;
                }
                return;
            }
        }
    }

    Component.onCompleted: updateCurrentIndex()

    Connections {
        target: root.dbusClient
        function onSelectedLayerChanged() {
            root.updateCurrentIndex();
        }
        function onLayersChanged() {
            root.updateCurrentIndex();
        }
    }

    onCurrentIndexChanged: {
        if (dbusClient && dbusClient.layers && currentIndex >= 0 && currentIndex < dbusClient.layers.length) {
            let newIndex = dbusClient.layers[currentIndex].index;
            if (dbusClient.selectedLayerIndex !== newIndex) {
                dbusClient.selectedLayerIndex = newIndex;
            }
        }
    }

    Repeater {
        model: root.dbusClient ? root.dbusClient.layers : []

        TabButton {
            id: tabBtn
            required property var modelData
            required property int index

            text: (modelData.active ? "● " : "") + modelData.name + " [" + modelData.index + "]"

            onClicked: {
                if (root.dbusClient) {
                    root.dbusClient.selectedLayerIndex = modelData.index;
                }
            }
        }
    }
}
