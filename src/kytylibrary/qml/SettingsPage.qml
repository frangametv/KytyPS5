import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: page
    property bool globalSettings: true
    property string category: "System"
    property var fields: []
    property var draft: ({})
    property string message: ""
    property bool dirty: false
    signal backRequested()
    function load(global) {
        globalSettings = global
        fields = library.settings(global)
        let values = {}
        for (let field of fields) values[field.key] = field.value
        draft = values
        category = global ? "Library" : "System"
        message = ""
        dirty = false
    }
    function setValue(key, value) {
        let values = Object.assign({}, draft)
        values[key] = value
        draft = values
        dirty = true
        message = ""
    }
    function reset() {
        let values = {}
        for (let field of library.defaults()) values[field.key] = field.value
        draft = values
        dirty = true
        message = "Defaults restored. Save to apply."
    }
    RowLayout {
        anchors.fill: parent
        spacing: 32
        ColumnLayout {
            Layout.preferredWidth: 190
            Layout.fillHeight: true
            spacing: 6
            Label {
                text: page.globalSettings ? qsTr("PREFERENCES") : qsTr("GAME OPTIONS")
                color: "#777F8E"; font.pixelSize: 11; font.letterSpacing: 1.4
                Layout.bottomMargin: 16
            }
            Repeater {
                model: page.globalSettings ? ["Library", "System", "Graphics", "Logging", "Advanced", "Controller", "About"] :
                                             ["System", "Graphics", "Logging", "Advanced"]
                LibraryButton {
                    required property string modelData
                    text: qsTranslate("Library", modelData)
                    primary: page.category === modelData
                    quiet: !primary
                    Layout.fillWidth: true
                    onClicked: page.category = modelData
                }
            }
            Item { Layout.fillHeight: true }
            Label {
                text: page.globalSettings ? qsTr("Defaults for your games.\nIndividual games can override them.") :
                                           qsTr("Overrides for the selected game.")
                color: "#777F8E"; font.pixelSize: 12; wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16
            Label {
                text: page.globalSettings ? qsTranslate("Library", page.category) : (library.selected.name || qsTr("Game")) + " · " + qsTranslate("Library", page.category)
                font.pixelSize: 28; font.weight: Font.DemiBold; color: "#F7F8FB"
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Label {
                visible: !page.globalSettings
                text: library.selected.custom ? qsTr("This game uses custom settings.") : qsTr("Currently inheriting global settings.")
                color: "#A7A7AF"; font.pixelSize: 13
            }
            ScrollView {
                id: scroll
                Layout.fillWidth: true; Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: scroll.availableWidth
                    spacing: 8
                    Repeater {
                        model: page.fields
                        SettingField {
                            required property var modelData
                            field: modelData
                            currentValue: page.draft[modelData.key]
                            visible: page.category === modelData.section
                            enabled: !library.running
                            onEdited: value => page.setValue(modelData.key, value)
                        }
                    }
                    ColumnLayout {
                        visible: page.category === "Library"
                        Layout.fillWidth: true
                        spacing: 14
                        Label { text: qsTr("Interface language"); color: "#F7F8FB"; font.pixelSize: 16 }
                        LibraryComboBox {
                            objectName: "uiLanguageSelector"
                            translateOptions: false
                            Layout.preferredWidth: 250
                            model: library.uiLanguages.map(language => language.label)
                            currentIndex: library.uiLanguages.findIndex(language => language.code === library.uiLanguage)
                            onActivated: library.uiLanguage = library.uiLanguages[currentIndex].code
                            Accessible.name: qsTr("Interface language")
                        }
                        Label {
                            text: qsTr("Game folders")
                            color: "#F7F8FB"; font.pixelSize: 16
                        }
                        Label {
                            text: qsTr("Folders are scanned recursively for eboot.bin. Removing a folder only removes it from this library.")
                            color: "#A7A7AF"; wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        Repeater {
                            model: library.folders
                            Rectangle {
                                required property string modelData
                                required property int index
                                Layout.fillWidth: true; implicitHeight: 64; color: "#161616"; radius: 8
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: 12
                                    Label {
                                        text: modelData; color: "#C0C5CF"; elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                    LibraryButton {
                                        text: qsTr("Remove"); danger: true; enabled: !library.running
                                        onClicked: library.removeFolder(index)
                                    }
                                }
                            }
                        }
                        RowLayout {
                            LibraryButton { text: qsTr("+ Add folder"); primary: true; enabled: !library.running; onClicked: library.addFolder() }
                            LibraryButton { text: qsTr("Rescan"); enabled: !library.running && !library.scanning; onClicked: library.rescan() }
                        }
                    }
                    ColumnLayout {
                        visible: page.category === "Controller"
                        Layout.fillWidth: true; spacing: 16
                        Label { text: qsTr("Keyboard & controller"); color: "#F7F8FB"; font.pixelSize: 18 }
                        Label {
                            text: qsTr("Configure the global input mapping used when starting a game.")
                            color: "#A7A7AF"; wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                        LibraryButton { text: qsTr("Configure input mapping"); primary: true; enabled: !library.running; onClicked: library.action("input") }
                    }
                    ColumnLayout {
                        visible: page.category === "About"
                        Layout.fillWidth: true; spacing: 16
                        Label { text: "KytyPS5"; color: "#F7F8FB"; font.pixelSize: 30; font.bold: true }
                        Label { text: library.version; color: "#C0C5CF"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Label { text: qsTr("Library design inspired by SharpEmu. Built with Qt Quick."); color: "#777F8E"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Label { text: library.settingsFile; color: "#777F8E"; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
                        LibraryButton { text: qsTr("Open settings folder"); onClicked: library.action("openSettings") }
                        CheckBox {
                            visible: library.updatesSupported
                            text: qsTr("Check for updates on startup")
                            checked: library.checkUpdates
                            palette.windowText: "#F7F8FB"
                            onToggled: library.checkUpdates = checked
                        }
                        LibraryButton { visible: library.updatesSupported; text: qsTr("Check for updates"); onClicked: library.action("updates") }
                    }
                }
            }
            Label {
                text: qsTranslate("Library", page.message)
                visible: text.length > 0
                color: "#D9B08D"; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            RowLayout {
                visible: ["System", "Graphics", "Logging", "Advanced"].indexOf(page.category) >= 0
                Layout.fillWidth: true
                LibraryButton { text: qsTr("Restore defaults"); enabled: !library.running; onClicked: page.reset() }
                Item { Layout.fillWidth: true }
                LibraryButton { text: qsTr("Discard"); enabled: page.dirty; onClicked: page.load(page.globalSettings) }
                LibraryButton {
                    text: qsTr("Save changes"); primary: true; enabled: page.dirty && !library.running
                    onClicked: {
                        const error = library.saveSettings(page.globalSettings, page.draft)
                        page.message = error.length ? error : "Settings saved."
                        if (!error.length) page.dirty = false
                    }
                }
            }
        }
    }
}

