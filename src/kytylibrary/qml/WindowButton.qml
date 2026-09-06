import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    required property string actionName
    property bool restored: false
    implicitWidth: 46
    implicitHeight: 44
    hoverEnabled: true
    HoverHandler { id: pointerHover }
    Accessible.name: actionName === "maximize" ? (restored ? qsTr("Restore window") : qsTr("Maximize window")) :
                     actionName === "minimize" ? qsTr("Minimize window") : qsTr("Close window")
    ToolTip.visible: pointerHover.hovered
    ToolTip.delay: 600
    ToolTip.text: Accessible.name
    background: Rectangle {
        color: pointerHover.hovered || control.down
               ? (control.actionName === "close" ? "#C42B1C" : "#383838") : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: "#D3A07A"
    }
    contentItem: Item {
        Canvas {
            id: icon
            anchors.centerIn: parent
            width: 14
            height: 14
            onPaint: {
                const c = getContext("2d")
                c.clearRect(0, 0, width, height)
                c.strokeStyle = "#F7F8FB"
                c.lineWidth = 1
                c.beginPath()
                if (control.actionName === "minimize") {
                    c.moveTo(2, 10.5); c.lineTo(12, 10.5)
                } else if (control.actionName === "close") {
                    c.moveTo(2, 2); c.lineTo(12, 12)
                    c.moveTo(12, 2); c.lineTo(2, 12)
                } else if (control.restored) {
                    c.rect(2.5, 4.5, 7, 7)
                    c.moveTo(4.5, 3); c.lineTo(4.5, 2.5); c.lineTo(11.5, 2.5)
                    c.lineTo(11.5, 9.5); c.lineTo(10, 9.5)
                } else {
                    c.rect(2.5, 2.5, 9, 9)
                }
                c.stroke()
            }
            Connections { target: control; function onRestoredChanged() { icon.requestPaint() } }
        }
    }
}
