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
        case "layer": return palette.highlight; // Theme highlight for layer switches
        case "nav":   return "#a855f7"; // Purple for navigation/mouse
        case "media": return "#10b981"; // Emerald for media
        default:      return palette.placeholderText; // Theme placeholder for alphas/symbols
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.margins: 2
        radius: 8
        color: mouseArea.containsMouse ? 
               Qt.tint(palette.button, Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.15)) : 
               palette.button

        border.color: mouseArea.containsMouse ? 
                      root.accentColor : 
                      palette.mid
        border.width: mouseArea.containsMouse ? 1.5 : 1

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }

        // Position badge
        Label {
            visible: root.showPosition && root.position >= 0
            text: root.position
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 4
            font.pointSize: 6.5
            font.bold: true
            color: palette.placeholderText
        }

        // Secondary / Hold modifier label (top-right)
        Label {
            id: secLabel
            visible: root.secondaryText.length > 0
            text: root.secondaryText
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            font.pointSize: 7.5
            font.bold: true
            font.capitalization: Font.AllUppercase
            color: root.accentColor
        }

        // Primary / Tap key label (center)
        Label {
            id: primLabel
            text: root.primaryText
            anchors.centerIn: parent
            anchors.verticalCenterOffset: root.secondaryText.length > 0 ? 3 : 0
            font.pointSize: {
                if (root.primaryText.length <= 2) return 10.5;
                if (root.primaryText.length <= 4) return 8.5;
                return 7.5;
            }
            font.bold: true
            color: palette.buttonText
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
