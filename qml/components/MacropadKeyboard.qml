import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var keymapModel
    property bool showPositions: false

    readonly property var layoutData: keymapModel ? keymapModel.layoutData : null

    implicitWidth: layoutData && layoutData.width ? layoutData.width : 640
    implicitHeight: layoutData && layoutData.height ? layoutData.height : 340

    // Helper function to fetch key properties reactively
    function keyProp(pos, prop, defaultVal) {
        if (!keymapModel) return defaultVal;
        var _rev = keymapModel.revision;
        var data = keymapModel.getKeyData(pos);
        if (data && data[prop] !== undefined) return data[prop];
        return defaultVal;
    }

    Row {
        anchors.centerIn: parent
        spacing: 28

        // Left Rotary Knob
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("LEFT DIAL")
                font.pointSize: 7
                font.bold: true
                font.letterSpacing: 1.5
                color: palette.placeholderText
            }

            RotaryKnob {
                keymapModel: root.keymapModel
                sensorIndex: 0
                pressKeyPos: 0
                showPosition: root.showPositions
            }
        }

        // 5x5 Keypad Grid
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Repeater {
                model: root.layoutData && root.layoutData.grid && root.layoutData.grid.rowsData ? root.layoutData.grid.rowsData : []

                Row {
                    id: gridRow
                    required property var modelData
                    spacing: 6

                    Repeater {
                        model: gridRow.modelData.keys || []

                        KeyCap {
                            required property var modelData
                            readonly property int keyPos: (typeof modelData === "object" && modelData.pos !== undefined) ? modelData.pos : modelData
                            readonly property real keyWidth: (typeof modelData === "object" && modelData.width !== undefined) ? modelData.width : 52

                            width: keyWidth
                            position: keyPos
                            primaryText: keyProp(keyPos, "primaryLabel", "" + keyPos)
                            secondaryText: keyProp(keyPos, "secondaryLabel", "")
                            tooltipText: keyProp(keyPos, "tooltip", "")
                            category: keyProp(keyPos, "category", "misc")
                            showPosition: root.showPositions
                        }
                    }
                }
            }
        }

        // Right Rotary Knob
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("RIGHT DIAL")
                font.pointSize: 7
                font.bold: true
                font.letterSpacing: 1.5
                color: palette.placeholderText
            }

            RotaryKnob {
                keymapModel: root.keymapModel
                sensorIndex: 1
                pressKeyPos: 23
                showPosition: root.showPositions
            }
        }
    }
}
