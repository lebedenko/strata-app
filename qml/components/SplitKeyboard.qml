import QtQuick
import QtQuick.Layouts

Item {
    id: root

    required property var keymapModel
    property bool showPositions: false

    implicitWidth: 960
    implicitHeight: 340

    // Helper function to fetch key properties reactively
    function keyProp(pos, prop, defaultVal) {
        if (!keymapModel) return defaultVal;
        var _rev = keymapModel.revision;
        var data = keymapModel.getKeyData(pos);
        if (data && data[prop] !== undefined) return data[prop];
        return defaultVal;
    }

    // Left Half
    Item {
        id: leftHalf
        width: 350
        height: 280
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        // Matrix columns (6 cols x 3 rows)
        Row {
            spacing: 6
            anchors.top: parent.top

            // Col 0 (Outer)
            Column {
                spacing: 6
                y: 12
                KeyCap { position: 0; primaryText: keyProp(0, "primaryLabel", "0"); secondaryText: keyProp(0, "secondaryLabel", ""); tooltipText: keyProp(0, "tooltip", ""); category: keyProp(0, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 13; primaryText: keyProp(13, "primaryLabel", "13"); secondaryText: keyProp(13, "secondaryLabel", ""); tooltipText: keyProp(13, "tooltip", ""); category: keyProp(13, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 28; primaryText: keyProp(28, "primaryLabel", "28"); secondaryText: keyProp(28, "secondaryLabel", ""); tooltipText: keyProp(28, "tooltip", ""); category: keyProp(28, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 1 (Pinky)
            Column {
                spacing: 6
                y: 6
                KeyCap { position: 1; primaryText: keyProp(1, "primaryLabel", "1"); secondaryText: keyProp(1, "secondaryLabel", ""); tooltipText: keyProp(1, "tooltip", ""); category: keyProp(1, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 14; primaryText: keyProp(14, "primaryLabel", "14"); secondaryText: keyProp(14, "secondaryLabel", ""); tooltipText: keyProp(14, "tooltip", ""); category: keyProp(14, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 29; primaryText: keyProp(29, "primaryLabel", "29"); secondaryText: keyProp(29, "secondaryLabel", ""); tooltipText: keyProp(29, "tooltip", ""); category: keyProp(29, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 2 (Ring)
            Column {
                spacing: 6
                y: 0
                KeyCap { position: 2; primaryText: keyProp(2, "primaryLabel", "2"); secondaryText: keyProp(2, "secondaryLabel", ""); tooltipText: keyProp(2, "tooltip", ""); category: keyProp(2, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 15; primaryText: keyProp(15, "primaryLabel", "15"); secondaryText: keyProp(15, "secondaryLabel", ""); tooltipText: keyProp(15, "tooltip", ""); category: keyProp(15, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 30; primaryText: keyProp(30, "primaryLabel", "30"); secondaryText: keyProp(30, "secondaryLabel", ""); tooltipText: keyProp(30, "tooltip", ""); category: keyProp(30, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 3 (Middle)
            Column {
                spacing: 6
                y: -6
                KeyCap { position: 3; primaryText: keyProp(3, "primaryLabel", "3"); secondaryText: keyProp(3, "secondaryLabel", ""); tooltipText: keyProp(3, "tooltip", ""); category: keyProp(3, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 16; primaryText: keyProp(16, "primaryLabel", "16"); secondaryText: keyProp(16, "secondaryLabel", ""); tooltipText: keyProp(16, "tooltip", ""); category: keyProp(16, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 31; primaryText: keyProp(31, "primaryLabel", "31"); secondaryText: keyProp(31, "secondaryLabel", ""); tooltipText: keyProp(31, "tooltip", ""); category: keyProp(31, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 4 (Index)
            Column {
                spacing: 6
                y: 0
                KeyCap { position: 4; primaryText: keyProp(4, "primaryLabel", "4"); secondaryText: keyProp(4, "secondaryLabel", ""); tooltipText: keyProp(4, "tooltip", ""); category: keyProp(4, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 17; primaryText: keyProp(17, "primaryLabel", "17"); secondaryText: keyProp(17, "secondaryLabel", ""); tooltipText: keyProp(17, "tooltip", ""); category: keyProp(17, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 32; primaryText: keyProp(32, "primaryLabel", "32"); secondaryText: keyProp(32, "secondaryLabel", ""); tooltipText: keyProp(32, "tooltip", ""); category: keyProp(32, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 5 (Inner)
            Column {
                spacing: 6
                y: 4
                KeyCap { position: 5; primaryText: keyProp(5, "primaryLabel", "5"); secondaryText: keyProp(5, "secondaryLabel", ""); tooltipText: keyProp(5, "tooltip", ""); category: keyProp(5, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 18; primaryText: keyProp(18, "primaryLabel", "18"); secondaryText: keyProp(18, "secondaryLabel", ""); tooltipText: keyProp(18, "tooltip", ""); category: keyProp(18, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 33; primaryText: keyProp(33, "primaryLabel", "33"); secondaryText: keyProp(33, "secondaryLabel", ""); tooltipText: keyProp(33, "tooltip", ""); category: keyProp(33, "category", "misc"); showPosition: root.showPositions }
            }
        }

        // Left Thumb Cluster
        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            spacing: 6

            KeyCap {
                position: 42
                primaryText: keyProp(42, "primaryLabel", "42")
                secondaryText: keyProp(42, "secondaryLabel", "")
                tooltipText: keyProp(42, "tooltip", "")
                category: keyProp(42, "category", "misc")
                showPosition: root.showPositions
                rotation: -8
            }
            KeyCap {
                position: 43
                primaryText: keyProp(43, "primaryLabel", "43")
                secondaryText: keyProp(43, "secondaryLabel", "")
                tooltipText: keyProp(43, "tooltip", "")
                category: keyProp(43, "category", "misc")
                showPosition: root.showPositions
                rotation: -4
            }
            KeyCap {
                position: 44
                primaryText: keyProp(44, "primaryLabel", "44")
                secondaryText: keyProp(44, "secondaryLabel", "")
                tooltipText: keyProp(44, "tooltip", "")
                category: keyProp(44, "category", "misc")
                showPosition: root.showPositions
            }
        }
    }

    // Center Navigation & Control Region: Rotary Knob (Left) + 5-Way Joystick (Right)
    Row {
        id: centerControls
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        spacing: 12

        // Left Half Rotary Encoder Knob
        RotaryKnob {
            id: leftKnob
            keymapModel: root.keymapModel
            sensorIndex: 0
            showPosition: root.showPositions
            anchors.verticalCenter: parent.verticalCenter
        }

        // Right Half 5-Way Navigation Switch
        Item {
            id: joystickCluster
            width: 114
            height: 124
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("JOYSTICK")
                font.pointSize: 6.5
                font.bold: true
                font.letterSpacing: 1.5
                color: palette.dark.hslLightness > 0.5 ? "#64748b" : "#a1a1aa"
            }

            Item {
                id: dpad
                width: 106
                height: 106
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter

                // UP
                KeyCap {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    width: 34
                    height: 34
                    position: 6
                    primaryText: keyProp(6, "primaryLabel", "UP")
                    secondaryText: keyProp(6, "secondaryLabel", "")
                    tooltipText: keyProp(6, "tooltip", "")
                    category: "nav"
                    showPosition: root.showPositions
                }

                // LEFT
                KeyCap {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    width: 34
                    height: 34
                    position: 19
                    primaryText: keyProp(19, "primaryLabel", "LFT")
                    secondaryText: keyProp(19, "secondaryLabel", "")
                    tooltipText: keyProp(19, "tooltip", "")
                    category: "nav"
                    showPosition: root.showPositions
                }

                // CENTER / ENTER
                KeyCap {
                    anchors.centerIn: parent
                    width: 34
                    height: 34
                    position: 20
                    primaryText: keyProp(20, "primaryLabel", "OK")
                    secondaryText: keyProp(20, "secondaryLabel", "")
                    tooltipText: keyProp(20, "tooltip", "")
                    category: "nav"
                    showPosition: root.showPositions
                }

                // RIGHT
                KeyCap {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: 34
                    height: 34
                    position: 21
                    primaryText: keyProp(21, "primaryLabel", "RGT")
                    secondaryText: keyProp(21, "secondaryLabel", "")
                    tooltipText: keyProp(21, "tooltip", "")
                    category: "nav"
                    showPosition: root.showPositions
                }

                // DOWN
                KeyCap {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    width: 34
                    height: 34
                    position: 35
                    primaryText: keyProp(35, "primaryLabel", "DWN")
                    secondaryText: keyProp(35, "secondaryLabel", "")
                    tooltipText: keyProp(35, "tooltip", "")
                    category: "nav"
                    showPosition: root.showPositions
                }
            }
        }
    }

    // Right Half
    Item {
        id: rightHalf
        width: 350
        height: 280
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        // Matrix columns (6 cols x 3 rows)
        Row {
            spacing: 6
            anchors.top: parent.top

            // Col 0 (Inner)
            Column {
                spacing: 6
                y: 4
                KeyCap { position: 7; primaryText: keyProp(7, "primaryLabel", "7"); secondaryText: keyProp(7, "secondaryLabel", ""); tooltipText: keyProp(7, "tooltip", ""); category: keyProp(7, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 22; primaryText: keyProp(22, "primaryLabel", "22"); secondaryText: keyProp(22, "secondaryLabel", ""); tooltipText: keyProp(22, "tooltip", ""); category: keyProp(22, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 36; primaryText: keyProp(36, "primaryLabel", "36"); secondaryText: keyProp(36, "secondaryLabel", ""); tooltipText: keyProp(36, "tooltip", ""); category: keyProp(36, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 1 (Index)
            Column {
                spacing: 6
                y: 0
                KeyCap { position: 8; primaryText: keyProp(8, "primaryLabel", "8"); secondaryText: keyProp(8, "secondaryLabel", ""); tooltipText: keyProp(8, "tooltip", ""); category: keyProp(8, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 23; primaryText: keyProp(23, "primaryLabel", "23"); secondaryText: keyProp(23, "secondaryLabel", ""); tooltipText: keyProp(23, "tooltip", ""); category: keyProp(23, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 37; primaryText: keyProp(37, "primaryLabel", "37"); secondaryText: keyProp(37, "secondaryLabel", ""); tooltipText: keyProp(37, "tooltip", ""); category: keyProp(37, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 2 (Middle)
            Column {
                spacing: 6
                y: -6
                KeyCap { position: 9; primaryText: keyProp(9, "primaryLabel", "9"); secondaryText: keyProp(9, "secondaryLabel", ""); tooltipText: keyProp(9, "tooltip", ""); category: keyProp(9, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 24; primaryText: keyProp(24, "primaryLabel", "24"); secondaryText: keyProp(24, "secondaryLabel", ""); tooltipText: keyProp(24, "tooltip", ""); category: keyProp(24, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 38; primaryText: keyProp(38, "primaryLabel", "38"); secondaryText: keyProp(38, "secondaryLabel", ""); tooltipText: keyProp(38, "tooltip", ""); category: keyProp(38, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 3 (Ring)
            Column {
                spacing: 6
                y: 0
                KeyCap { position: 10; primaryText: keyProp(10, "primaryLabel", "10"); secondaryText: keyProp(10, "secondaryLabel", ""); tooltipText: keyProp(10, "tooltip", ""); category: keyProp(10, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 25; primaryText: keyProp(25, "primaryLabel", "25"); secondaryText: keyProp(25, "secondaryLabel", ""); tooltipText: keyProp(25, "tooltip", ""); category: keyProp(25, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 39; primaryText: keyProp(39, "primaryLabel", "39"); secondaryText: keyProp(39, "secondaryLabel", ""); tooltipText: keyProp(39, "tooltip", ""); category: keyProp(39, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 4 (Pinky)
            Column {
                spacing: 6
                y: 6
                KeyCap { position: 11; primaryText: keyProp(11, "primaryLabel", "11"); secondaryText: keyProp(11, "secondaryLabel", ""); tooltipText: keyProp(11, "tooltip", ""); category: keyProp(11, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 26; primaryText: keyProp(26, "primaryLabel", "26"); secondaryText: keyProp(26, "secondaryLabel", ""); tooltipText: keyProp(26, "tooltip", ""); category: keyProp(26, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 40; primaryText: keyProp(40, "primaryLabel", "40"); secondaryText: keyProp(40, "secondaryLabel", ""); tooltipText: keyProp(40, "tooltip", ""); category: keyProp(40, "category", "misc"); showPosition: root.showPositions }
            }

            // Col 5 (Outer)
            Column {
                spacing: 6
                y: 12
                KeyCap { position: 12; primaryText: keyProp(12, "primaryLabel", "12"); secondaryText: keyProp(12, "secondaryLabel", ""); tooltipText: keyProp(12, "tooltip", ""); category: keyProp(12, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 27; primaryText: keyProp(27, "primaryLabel", "27"); secondaryText: keyProp(27, "secondaryLabel", ""); tooltipText: keyProp(27, "tooltip", ""); category: keyProp(27, "category", "misc"); showPosition: root.showPositions }
                KeyCap { position: 41; primaryText: keyProp(41, "primaryLabel", "41"); secondaryText: keyProp(41, "secondaryLabel", ""); tooltipText: keyProp(41, "tooltip", ""); category: keyProp(41, "category", "misc"); showPosition: root.showPositions }
            }
        }

        // Right Thumb Cluster
        Row {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            spacing: 6

            KeyCap {
                position: 45
                primaryText: keyProp(45, "primaryLabel", "45")
                secondaryText: keyProp(45, "secondaryLabel", "")
                tooltipText: keyProp(45, "tooltip", "")
                category: keyProp(45, "category", "misc")
                showPosition: root.showPositions
            }
            KeyCap {
                position: 46
                primaryText: keyProp(46, "primaryLabel", "46")
                secondaryText: keyProp(46, "secondaryLabel", "")
                tooltipText: keyProp(46, "tooltip", "")
                category: keyProp(46, "category", "misc")
                showPosition: root.showPositions
                rotation: 4
            }
            KeyCap {
                position: 47
                primaryText: keyProp(47, "primaryLabel", "47")
                secondaryText: keyProp(47, "secondaryLabel", "")
                tooltipText: keyProp(47, "tooltip", "")
                category: keyProp(47, "category", "misc")
                showPosition: root.showPositions
                rotation: 8
            }
        }
    }
}
