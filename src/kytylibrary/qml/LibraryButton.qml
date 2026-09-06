import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control
    property string glyph: ""
    property bool primary: false
    property bool danger: false
    property bool quiet: false
    property bool outlined: false
    implicitHeight: 40
    implicitWidth: Math.max(40, contentItem.implicitWidth + 30)
    hoverEnabled: true
    opacity: enabled ? 1 : 0.4
    font.pixelSize: 13
    font.weight: Font.DemiBold
    contentItem: Item {
        implicitWidth: label.implicitWidth + (control.glyph.length ? ((control.glyph === "disc" || control.glyph === "gear") ? 30 : 24) : 0)
        implicitHeight: label.implicitHeight
        Row {
            anchors.centerIn: parent
            spacing: 10
            Canvas {
                id: symbol
                visible: control.glyph.length > 0 && control.glyph !== "disc" && control.glyph !== "gear"
                width: visible ? 14 : 0; height: 14
                anchors.verticalCenter: parent.verticalCenter
                onPaint: {
                    const c = getContext("2d")
                    c.clearRect(0, 0, width, height)
                    c.fillStyle = control.danger ? "#FF858D" : control.primary ? "#1C1510" : "#F7F8FB"
                    c.strokeStyle = c.fillStyle
                    c.lineWidth = 1.5
                    c.beginPath()
                    switch (control.glyph) {
                    case "play": c.moveTo(3, 1); c.lineTo(12, 7); c.lineTo(3, 13); c.closePath(); c.fill(); break
                    case "stop": c.fillRect(3, 3, 9, 9); break
                    case "list": for (let y = 3; y <= 11; y += 4) { c.moveTo(1, y); c.lineTo(13, y) } c.stroke(); break
                    case "grid": for (let x = 1; x < 10; x += 7) for (let y = 1; y < 10; y += 7) c.strokeRect(x, y, 5, 5); break
                    case "console": c.moveTo(1, 3); c.lineTo(5, 7); c.lineTo(1, 11); c.moveTo(7, 11); c.lineTo(13, 11); c.stroke(); break
                    case "more": for (let x = 2; x <= 12; x += 5) { c.beginPath(); c.arc(x, 7, 1.4, 0, Math.PI * 2); c.fill() } break
                    }
                }
                Connections { target: control; function onGlyphChanged() { symbol.requestPaint() } }
            }
            Image {
                visible: control.glyph === "disc" || control.glyph === "gear"
                source: control.glyph === "disc" ? "qrc:/icons/library-disc.svg" : "qrc:/icons/library-gear.svg"
                sourceSize.width: 20; sourceSize.height: 20
                width: 20; height: 20
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
            }
            Text {
                id: label
                text: control.text
                color: control.danger ? "#FF858D" : control.primary ? "#1C1510" : "#F7F8FB"
                font: control.font
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    background: Rectangle {
        radius: 8
        color: control.primary ? (control.hovered ? "#C68A61" : "#B87950") :
               control.down ? "#303030" : control.hovered ? "#262626" :
               control.quiet ? "#202020" : "#1B1B1B"
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? "#D3A07A" : control.outlined ? "#B87950" : control.primary || control.quiet ? "transparent" : "#333333"
    }
    Accessible.name: text.length ? text : glyph === "more" ? qsTr("Game actions") : glyph
}

