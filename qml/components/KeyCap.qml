import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string primaryText: ""
    property string secondaryText: ""
    property string tooltipText: ""
    property string category: "alpha"
    property int position: -1
    property bool showPosition: false

    width: 52
    height: 52

    // Category-specific subtle accent colors
    readonly property color accentColor: {
        switch (category) {
        case "mod":   return "#f59e0b"; // Amber for modifiers
        case "layer": return "#38bdf8"; // Sky blue for layer switches
        case "nav":   return "#a855f7"; // Purple for navigation/mouse
        case "media": return "#10b981"; // Emerald for media
        default:      return "#94a3b8"; // Slate for alphas/symbols
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.margins: 2
        radius: 8
        color: mouseArea.containsMouse ? 
               (palette.dark.hslLightness > 0.5 ? "#e2e8f0" : "#27272a") : 
               (palette.dark.hslLightness > 0.5 ? "#ffffff" : "#18181b")

        border.color: mouseArea.containsMouse ? 
                      root.accentColor : 
                      (palette.dark.hslLightness > 0.5 ? "#cbd5e1" : "#3f3f46")
        border.width: mouseArea.containsMouse ? 1.5 : 1

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }

        // Position badge
        Text {
            visible: root.showPosition && root.position >= 0
            text: root.position
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 4
            font.pixelSize: 8
            font.bold: true
            color: palette.placeholderText
        }

        // Secondary / Hold modifier label (top-right)
        Text {
            id: secLabel
            visible: root.secondaryText.length > 0
            text: root.secondaryText
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            font.pixelSize: 9
            font.bold: true
            font.capitalization: Font.AllUppercase
            color: root.accentColor
        }

        // Primary / Tap key label (center)
        Text {
            id: primLabel
            text: root.primaryText
            anchors.centerIn: parent
            anchors.verticalCenterOffset: root.secondaryText.length > 0 ? 3 : 0
            font.pixelSize: {
                if (root.primaryText.length <= 2) return 14;
                if (root.primaryText.length <= 4) return 10;
                return 9;
            }
            font.bold: true
            color: palette.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            width: parent.width - 8
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        ToolTip.visible: containsMouse && root.tooltipText.length > 0
        ToolTip.text: root.tooltipText
        ToolTip.delay: 300
    }
}
