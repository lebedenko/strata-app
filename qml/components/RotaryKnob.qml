import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property var keymapModel
    property int sensorIndex: 0
    property bool showPosition: false

    implicitWidth: 120
    implicitHeight: 120

    // Fetch sensor data reactively
    readonly property var sensorData: {
        if (!keymapModel) return null;
        var _rev = keymapModel.revision;
        return keymapModel.getSensorData(sensorIndex);
    }
    readonly property string cwLabel: sensorData ? sensorData.cwLabel : "CW"
    readonly property string ccwLabel: sensorData ? sensorData.ccwLabel : "CCW"
    readonly property string pressLabel: sensorData ? sensorData.pressLabel : "PUSH"
    readonly property string pressCategory: sensorData ? sensorData.pressCategory : "media"
    readonly property string tooltipText: sensorData ? sensorData.tooltip : "Rotary Encoder"

    // Theme palette mappings
    readonly property color dialBg: palette.base
    readonly property color dialBorder: palette.mid
    readonly property color pressBtnBg: palette.button
    readonly property color pressBtnHover: Qt.tint(palette.button, Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.2))
    readonly property color textColor: palette.buttonText
    readonly property color subTextColor: palette.placeholderText
    readonly property color accentColor: palette.highlight

    // Outer Dial Container
    Rectangle {
        id: dial
        anchors.centerIn: parent
        width: 104
        height: 104
        radius: width / 2
        color: dialBg
        border.color: dialBorder
        border.width: 1.5

        // Subtle decorative outer ring
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 8
            height: parent.height - 8
            radius: width / 2
            color: "transparent"
            border.color: palette.mid
            border.width: 1
        }

        // CCW Wing Indicator (Left)
        Item {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            width: 28
            height: 38

            Column {
                anchors.centerIn: parent
                spacing: 1

                Label {
                    text: "⟲"
                    font.pointSize: 9
                    font.bold: true
                    color: accentColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: root.ccwLabel
                    font.pointSize: 6.5
                    font.bold: true
                    color: subTextColor
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }
        }

        // CW Wing Indicator (Right)
        Item {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            width: 28
            height: 38

            Column {
                anchors.centerIn: parent
                spacing: 1

                Label {
                    text: "⟳"
                    font.pointSize: 9
                    font.bold: true
                    color: accentColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: root.cwLabel
                    font.pointSize: 6.5
                    font.bold: true
                    color: subTextColor
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }
        }

        // Center Push Button (Key 34)
        Rectangle {
            id: pressButton
            anchors.centerIn: parent
            width: 44
            height: 44
            radius: width / 2
            color: pressArea.containsMouse ? pressBtnHover : pressBtnBg
            border.color: pressArea.containsMouse ? accentColor : dialBorder
            border.width: 1.5

            Behavior on color { ColorAnimation { duration: 150 } }
            Behavior on border.color { ColorAnimation { duration: 150 } }

            Column {
                anchors.centerIn: parent
                spacing: 0

                Label {
                    text: root.pressLabel
                    font.pointSize: root.pressLabel.length > 4 ? 6.5 : 7.5
                    font.bold: true
                    color: textColor
                    anchors.horizontalCenter: parent.horizontalCenter
                    elide: Text.ElideRight
                }

                Label {
                    visible: root.showPosition
                    text: "#34"
                    font.pointSize: 5.5
                    color: subTextColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            MouseArea {
                id: pressArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }

    // Top Header Label
    Label {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("KNOB")
        font.pointSize: 6.5
        font.bold: true
        font.letterSpacing: 1.5
        color: subTextColor
    }

    // ToolTip covering the entire knob
    ToolTip {
        id: tooltip
        visible: hoverArea.containsMouse
        delay: 400
        timeout: 5000
        text: qsTr("Rotary Encoder:\n• Push (Key 34): %1\n• Turn CW: %2\n• Turn CCW: %3\n(%4)").arg(root.pressLabel).arg(root.cwLabel).arg(root.ccwLabel).arg(root.tooltipText)
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
