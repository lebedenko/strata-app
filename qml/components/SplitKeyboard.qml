import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var keymapModel
    property bool showPositions: false

    readonly property var layoutData: keymapModel ? keymapModel.layoutData : null

    implicitWidth: layoutData && layoutData.width ? layoutData.width : 960
    implicitHeight: layoutData && layoutData.height ? layoutData.height : 340

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
        width: root.layoutData && root.layoutData.leftHalf && root.layoutData.leftHalf.width ? root.layoutData.leftHalf.width : 350
        height: root.layoutData && root.layoutData.leftHalf && root.layoutData.leftHalf.height ? root.layoutData.leftHalf.height : 280
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        // Matrix columns
        Row {
            spacing: 6
            anchors.top: parent.top

            Repeater {
                model: root.layoutData && root.layoutData.leftHalf && root.layoutData.leftHalf.columns ? root.layoutData.leftHalf.columns : []
                Column {
                    id: leftCol
                    required property var modelData
                    spacing: 6
                    y: modelData.offsetY !== undefined ? modelData.offsetY : 0

                    Repeater {
                        model: leftCol.modelData.keys || []
                        KeyCap {
                            required property int modelData
                            position: modelData
                            primaryText: keyProp(modelData, "primaryLabel", "" + modelData)
                            secondaryText: keyProp(modelData, "secondaryLabel", "")
                            tooltipText: keyProp(modelData, "tooltip", "")
                            category: keyProp(modelData, "category", "misc")
                            showPosition: root.showPositions
                        }
                    }
                }
            }
        }

        // Left Thumb Cluster
        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            spacing: 6

            Repeater {
                model: root.layoutData && root.layoutData.leftHalf && root.layoutData.leftHalf.thumbCluster ? root.layoutData.leftHalf.thumbCluster : []
                KeyCap {
                    required property var modelData
                    position: modelData.pos
                    rotation: modelData.rotation !== undefined ? modelData.rotation : 0
                    primaryText: keyProp(modelData.pos, "primaryLabel", "" + modelData.pos)
                    secondaryText: keyProp(modelData.pos, "secondaryLabel", "")
                    tooltipText: keyProp(modelData.pos, "tooltip", "")
                    category: keyProp(modelData.pos, "category", "misc")
                    showPosition: root.showPositions
                }
            }
        }
    }

    // Center Controls (Rotary Knob, Joystick, etc. if present in layout)
    Row {
        id: centerControls
        visible: root.layoutData && root.layoutData.centerControls && root.layoutData.centerControls.length > 0
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        spacing: 12

        Repeater {
            model: root.layoutData && root.layoutData.centerControls ? root.layoutData.centerControls : []

            Item {
                id: controlDelegate
                required property var modelData
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                width: knob.visible ? knob.implicitWidth : (joystickCluster.visible ? joystickCluster.width : 0)
                height: knob.visible ? knob.implicitHeight : (joystickCluster.visible ? joystickCluster.height : 0)

                RotaryKnob {
                    id: knob
                    visible: controlDelegate.modelData && controlDelegate.modelData.type === "rotary_knob"
                    keymapModel: root.keymapModel
                    sensorIndex: (controlDelegate.modelData && controlDelegate.modelData.sensorIndex !== undefined) ? controlDelegate.modelData.sensorIndex : 0
                    pressKeyPos: (controlDelegate.modelData && controlDelegate.modelData.pressPos !== undefined) ? controlDelegate.modelData.pressPos : 34
                    showPosition: root.showPositions
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                }

                Item {
                    id: joystickCluster
                    visible: controlDelegate.modelData && controlDelegate.modelData.type === "joystick_5way"
                    width: 114
                    height: 124
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                    Label {
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("JOYSTICK")
                        font.pointSize: 6.5
                        font.bold: true
                        font.letterSpacing: 1.5
                        color: palette.placeholderText
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
                            position: (controlDelegate.modelData && controlDelegate.modelData.upPos !== undefined) ? controlDelegate.modelData.upPos : 6
                            primaryText: keyProp(position, "primaryLabel", "UP")
                            secondaryText: keyProp(position, "secondaryLabel", "")
                            tooltipText: keyProp(position, "tooltip", "")
                            category: "nav"
                            showPosition: root.showPositions
                        }

                        // LEFT
                        KeyCap {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            width: 34
                            height: 34
                            position: (controlDelegate.modelData && controlDelegate.modelData.leftPos !== undefined) ? controlDelegate.modelData.leftPos : 19
                            primaryText: keyProp(position, "primaryLabel", "LFT")
                            secondaryText: keyProp(position, "secondaryLabel", "")
                            tooltipText: keyProp(position, "tooltip", "")
                            category: "nav"
                            showPosition: root.showPositions
                        }

                        // CENTER / ENTER
                        KeyCap {
                            anchors.centerIn: parent
                            width: 34
                            height: 34
                            position: (controlDelegate.modelData && controlDelegate.modelData.centerPos !== undefined) ? controlDelegate.modelData.centerPos : 20
                            primaryText: keyProp(position, "primaryLabel", "OK")
                            secondaryText: keyProp(position, "secondaryLabel", "")
                            tooltipText: keyProp(position, "tooltip", "")
                            category: "nav"
                            showPosition: root.showPositions
                        }

                        // RIGHT
                        KeyCap {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            width: 34
                            height: 34
                            position: (controlDelegate.modelData && controlDelegate.modelData.rightPos !== undefined) ? controlDelegate.modelData.rightPos : 21
                            primaryText: keyProp(position, "primaryLabel", "RGT")
                            secondaryText: keyProp(position, "secondaryLabel", "")
                            tooltipText: keyProp(position, "tooltip", "")
                            category: "nav"
                            showPosition: root.showPositions
                        }

                        // DOWN
                        KeyCap {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            width: 34
                            height: 34
                            position: (controlDelegate.modelData && controlDelegate.modelData.downPos !== undefined) ? controlDelegate.modelData.downPos : 35
                            primaryText: keyProp(position, "primaryLabel", "DWN")
                            secondaryText: keyProp(position, "secondaryLabel", "")
                            tooltipText: keyProp(position, "tooltip", "")
                            category: "nav"
                            showPosition: root.showPositions
                        }
                    }
                }
            }
        }
    }

    // Right Half
    Item {
        id: rightHalf
        width: root.layoutData && root.layoutData.rightHalf && root.layoutData.rightHalf.width ? root.layoutData.rightHalf.width : 350
        height: root.layoutData && root.layoutData.rightHalf && root.layoutData.rightHalf.height ? root.layoutData.rightHalf.height : 280
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        // Matrix columns
        Row {
            spacing: 6
            anchors.top: parent.top

            Repeater {
                model: root.layoutData && root.layoutData.rightHalf && root.layoutData.rightHalf.columns ? root.layoutData.rightHalf.columns : []
                Column {
                    id: rightCol
                    required property var modelData
                    spacing: 6
                    y: modelData.offsetY !== undefined ? modelData.offsetY : 0

                    Repeater {
                        model: rightCol.modelData.keys || []
                        KeyCap {
                            required property int modelData
                            position: modelData
                            primaryText: keyProp(modelData, "primaryLabel", "" + modelData)
                            secondaryText: keyProp(modelData, "secondaryLabel", "")
                            tooltipText: keyProp(modelData, "tooltip", "")
                            category: keyProp(modelData, "category", "misc")
                            showPosition: root.showPositions
                        }
                    }
                }
            }
        }

        // Right Thumb Cluster
        Row {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            spacing: 6

            Repeater {
                model: root.layoutData && root.layoutData.rightHalf && root.layoutData.rightHalf.thumbCluster ? root.layoutData.rightHalf.thumbCluster : []
                KeyCap {
                    required property var modelData
                    position: modelData.pos
                    rotation: modelData.rotation !== undefined ? modelData.rotation : 0
                    primaryText: keyProp(modelData.pos, "primaryLabel", "" + modelData.pos)
                    secondaryText: keyProp(modelData.pos, "secondaryLabel", "")
                    tooltipText: keyProp(modelData.pos, "tooltip", "")
                    category: keyProp(modelData.pos, "category", "misc")
                    showPosition: root.showPositions
                }
            }
        }
    }
}
