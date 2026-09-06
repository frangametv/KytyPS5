import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: row
    required property var field
    required property var currentValue
    signal edited(var value)
    color: "#161616"
    radius: 8
    implicitHeight: 74
    Layout.fillWidth: true
    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 24
        Label {
            text: row.field.label
            color: "#F7F8FB"
            font.pixelSize: 14
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Loader {
            Layout.preferredWidth: row.field.kind === "bool" ? 54 : 250
            Layout.preferredHeight: 38
            sourceComponent: row.field.kind === "bool" ? toggle :
                             row.field.kind === "enum" || row.field.kind === "choiceIndex" ? choice : input
        }
    }
    Component {
        id: toggle
        Switch {
            checked: Boolean(row.currentValue)
            onToggled: row.edited(checked)
            Accessible.name: row.field.label
            indicator: Rectangle {
                width: 44; height: 24; radius: 12
                y: (parent.height - height) / 2
                color: parent.checked ? "#B87950" : "#353535"
                Rectangle {
                    x: parent.parent.checked ? 23 : 3
                    y: 3; width: 18; height: 18; radius: 9; color: "#F7F8FB"
                    Behavior on x { NumberAnimation { duration: 120 } }
                }
            }
        }
    }
    Component {
        id: choice
        LibraryComboBox {
            objectName: "editor_" + row.field.key
            model: row.field.choices
            currentIndex: row.field.kind === "choiceIndex" ? Number(row.currentValue) :
                          Math.max(0, row.field.choices.indexOf(String(row.currentValue)))
            onActivated: row.edited(row.field.kind === "choiceIndex" ? currentIndex : currentText)
            Accessible.name: row.field.label
            palette.button: "#242424"
            palette.buttonText: "#F7F8FB"
            palette.base: "#242424"
            palette.text: "#F7F8FB"
            palette.highlight: "#B87950"
            palette.highlightedText: "#1C1510"
        }
    }
    Component {
        id: input
        TextField {
            objectName: "editor_" + row.field.key
            text: String(row.currentValue)
            color: "#F7F8FB"
            selectByMouse: true
            font.pixelSize: 13
            onTextEdited: row.edited(text)
            Accessible.name: row.field.label
            background: Rectangle {
                radius: 8; color: "#242424"
                border.color: parent.activeFocus ? "#B87950" : "#353535"
            }
        }
    }
}

