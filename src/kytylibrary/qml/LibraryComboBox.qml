import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: control
    property bool translateOptions: true
    implicitHeight: 38
    hoverEnabled: true
    leftPadding: 12
    rightPadding: 30
    contentItem: Text {
        text: control.translateOptions ? qsTranslate("Library", control.displayText) : control.displayText
        font: control.font
        color: control.enabled ? "#F7F8FB" : "#777F8E"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 8
        color: control.hovered ? "#303030" : "#242424"
        border.color: control.activeFocus ? "#B87950" : "#454545"
    }
    indicator: Canvas {
        x: control.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        width: 12; height: 8
        onPaint: {
            const c = getContext("2d")
            c.clearRect(0, 0, width, height)
            c.strokeStyle = "#D3A07A"; c.lineWidth = 1.5
            c.beginPath(); c.moveTo(1, 1); c.lineTo(6, 6); c.lineTo(11, 1); c.stroke()
        }
    }
    delegate: ItemDelegate {
        required property int index
        required property var modelData
        width: control.width
        highlighted: control.highlightedIndex === index
        hoverEnabled: true
        contentItem: Text {
            text: control.translateOptions ? qsTranslate("Library", String(modelData)) : String(modelData)
            color: "#F7F8FB"
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: parent.highlighted || parent.hovered ? "#493528" : "#242424"
        }
    }
    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 280)
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle { color: "#242424"; radius: 8; border.color: "#B87950" }
    }
}
