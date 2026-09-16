import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property var dbusClient
    implicitHeight: 40
    implicitWidth: tabRow.width

    Row {
        id: tabRow
        spacing: 4
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: root.dbusClient ? root.dbusClient.layers : []

            Rectangle {
                id: tabButton
                required property var modelData
                required property int index

                width: tabText.implicitWidth + (isCurrentHardwareActive ? 28 : 20)
                height: 32
                radius: 6

                readonly property bool isSelected: root.dbusClient && 
                                                  root.dbusClient.selectedLayerIndex === modelData.index
                readonly property bool isCurrentHardwareActive: modelData.active === true

                color: isSelected ? 
                       (palette.dark.hslLightness > 0.5 ? "#0284c7" : "#0369a1") :
                       (tabMouse.containsMouse ? 
                        (palette.dark.hslLightness > 0.5 ? "#e2e8f0" : "#27272a") : 
                        "transparent")

                border.color: isCurrentHardwareActive ? "#38bdf8" : "transparent"
                border.width: isCurrentHardwareActive ? 1.5 : 0

                Behavior on color { ColorAnimation { duration: 100 } }

                Row {
                    anchors.centerIn: parent
                    spacing: 6

                    // Glowing dot for active hardware layer
                    Rectangle {
                        visible: tabButton.isCurrentHardwareActive
                        width: 6
                        height: 6
                        radius: 3
                        color: tabButton.isSelected ? "#ffffff" : "#38bdf8"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        id: tabText
                        text: tabButton.modelData.name + " [" + tabButton.modelData.index + "]"
                        font.bold: tabButton.isSelected || tabButton.isCurrentHardwareActive
                        font.pointSize: 9.5
                        color: tabButton.isSelected ? "#ffffff" : palette.text
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: tabMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.dbusClient) {
                            root.dbusClient.selectedLayerIndex = tabButton.modelData.index;
                        }
                    }
                }
            }
        }
    }
}
