// Visual layout and palette adapted from SharpEmu (GPL-2.0-or-later).
// Copyright (C) 2026 SharpEmu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0C0C0C"
    property bool options: false
    property bool consoleOpen: false
    property real consoleHeight: 240
    property bool listMode: false
    readonly property bool compact: height < 760
    readonly property real gameContentHeight: rail.visible ? rail.implicitHeight + 28 + detailsContent.implicitHeight + 50 + 48 : 180
    readonly property real consoleMaximumHeight: Math.max(80, contentLayout.height - navigation.height - footer.implicitHeight - 54 - (root.options ? 240 : gameContentHeight))
    property string query: ""
    property var filteredGames: library.games.filter(game => (game.name + " " + game.titleId).toLowerCase().indexOf(query.toLowerCase()) >= 0)
    onFilteredGamesChanged: {
        if (!filteredGames.some(entry => entry.path === library.selected.path))
            library.selectGame(filteredGames.length ? filteredGames[0].path : "")
    }
    property var game: library.selected
    property bool hasGame: Boolean(game.path)
    function showLibrary() {
        if (settingsPage.dirty) { navigationDialog.open(); return }
        options = false
    }
    function showOptions(global) {
        if (options && settingsPage.dirty) return
        settingsPage.load(global)
        options = true
    }
    function moveSelection(delta) {
        if (!filteredGames.length) return
        const index = Math.max(0, Math.min(filteredGames.length - 1, rail.currentIndex + delta))
        library.selectGame(filteredGames[index].path)
        rail.positionViewAtIndex(index, ListView.Contain)
    }
    Shortcut { sequence: "Ctrl+L"; onActivated: root.consoleOpen = !root.consoleOpen }
    Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "F5"; enabled: !library.running; onActivated: library.rescan() }
    Shortcut { sequence: "Escape"; enabled: root.options; onActivated: root.showLibrary() }

    Connections {
        target: library
        function onShowConsole() { root.consoleOpen = true }
        function onLogUpdated() { if (autoScroll.checked) logView.positionViewAtEnd() }
    }

    Image {
        anchors.fill: parent
        source: root.options ? "" : (root.game.backdrop || "")
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: 1920
        opacity: status === Image.Ready ? 0.5 : 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }
    Rectangle {
        anchors.fill: parent
        visible: !root.options
        gradient: Gradient {
            GradientStop { position: 0; color: "#600C0C0C" }
            GradientStop { position: 0.55; color: "#B80C0C0C" }
            GradientStop { position: 1; color: "#FA0C0C0C" }
        }
    }
    Rectangle { anchors.fill: parent; visible: root.options; color: "#1B1B1B" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 44; color: "#070707"
            MouseArea {
                objectName: "titleDragArea"
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                property point pressPosition
                property bool moving: false
                onPressed: mouse => { pressPosition = Qt.point(mouse.x, mouse.y); moving = false }
                onPositionChanged: mouse => {
                    if (pressed && !moving &&
                        Math.abs(mouse.x - pressPosition.x) + Math.abs(mouse.y - pressPosition.y) > 8) {
                        moving = windowChrome.beginWindowMove()
                    }
                }
                onDoubleClicked: windowChrome.toggleMaximized()
            }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; spacing: 12
                Rectangle {
                    implicitWidth: 22; implicitHeight: 22; radius: 8; color: "#B87950"
                    Text { anchors.centerIn: parent; text: "K"; font.bold: true; color: "#1C1510"; font.pixelSize: 14 }
                }
                Label { text: "KytyPS5"; color: "#F7F8FB"; font.weight: Font.DemiBold; font.pixelSize: 14 }
                Label { text: qsTr("LIBRARY"); color: "#777F8E"; font.pixelSize: 12; font.letterSpacing: 1.2 }
                Rectangle {
                    implicitWidth: buildLabel.implicitWidth + 20; implicitHeight: 28
                    radius: 8; color: "#30271F"; border.color: "#B87950"
                    Row {
                        id: buildLabel; anchors.centerIn: parent; spacing: 5
                        Label { text: library.version.split(":")[0]; color: "#D3A07A"; font.pixelSize: 13; font.bold: true }
                        Label { anchors.verticalCenter: parent.verticalCenter; text: library.version.split(":").slice(1).join(":").trim(); color: "#D3A07A"; font.pixelSize: 10 }
                    }
                }
                Item { Layout.fillWidth: true }
                Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: library.running ? "#66F2A3" : "#777F8E" }
                Label { text: qsTranslate("Library", library.status); color: "#A7A7AF"; font.pixelSize: 12; elide: Text.ElideRight; Layout.maximumWidth: root.width * 0.3 }
                Row {
                    Layout.leftMargin: 10
                    WindowButton {
                        objectName: "minimizeButton"
                        actionName: "minimize"
                        onClicked: windowChrome.minimizeWindow()
                    }
                    WindowButton {
                        objectName: "maximizeButton"
                        actionName: "maximize"
                        restored: windowChrome.maximized
                        onClicked: windowChrome.toggleMaximized()
                    }
                    WindowButton {
                        objectName: "closeButton"
                        actionName: "close"
                        onClicked: windowChrome.closeWindow()
                    }
                }
            }
        }
        ColumnLayout {
            id: contentLayout
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.leftMargin: 32; Layout.rightMargin: 32; Layout.topMargin: 22; Layout.bottomMargin: 16
            spacing: 18
            RowLayout {
                id: navigation
                Layout.fillWidth: true; spacing: 12
                LibraryButton {
                    text: qsTr("Library"); glyph: "disc"; outlined: !root.options; font.pixelSize: 18
                    opacity: root.options ? 0.75 : 1
                    onClicked: root.showLibrary()
                }
                LibraryButton {
                    text: qsTr("Options"); glyph: "gear"; outlined: root.options; font.pixelSize: 18
                    opacity: root.options ? 1 : 0.75
                    onClicked: root.showOptions(true)
                }
                Item { Layout.fillWidth: true }
                LibraryButton {
                    visible: !root.options; text: root.listMode ? qsTr("Covers") : qsTr("List")
                    glyph: root.listMode ? "grid" : "list"
                    onClicked: root.listMode = !root.listMode
                }
                TextField {
                    id: search
                    visible: !root.options
                    Layout.preferredWidth: 230; implicitHeight: 38
                    placeholderText: qsTr("Search games…")
                    color: "#F7F8FB"; placeholderTextColor: "#777F8E"
                    selectByMouse: true; font.pixelSize: 13
                    Accessible.name: qsTr("Search games")
                    onTextChanged: root.query = text
                    background: Rectangle { radius: 8; color: "#B0161616"; border.color: parent.activeFocus ? "#B87950" : "#303030" }
                }
                LibraryButton {
                    visible: !root.options; text: qsTr("+ Add folder"); enabled: !library.running && !library.scanning
                    onClicked: library.addFolder()
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                SettingsPage { id: settingsPage; objectName: "settingsPage"; anchors.fill: parent; visible: root.options }
                ColumnLayout {
                    id: gameArea
                    anchors.fill: parent; visible: !root.options; spacing: 16
                    ListView {
                        id: rail
                        objectName: "gameRail"
                        Layout.fillWidth: true
                        implicitHeight: root.listMode ? Math.min(Math.max(0, root.filteredGames.length * 72 - 6), root.compact ? 138 : 210) : (root.compact ? 114 : 194)
                        Layout.minimumHeight: implicitHeight
                        Layout.maximumHeight: implicitHeight
                        Layout.preferredHeight: implicitHeight
                        Layout.topMargin: 28
                        visible: root.filteredGames.length > 0
                        orientation: root.listMode ? ListView.Vertical : ListView.Horizontal
                        spacing: root.listMode ? 6 : 16
                        clip: true
                        model: root.filteredGames
                        currentIndex: root.filteredGames.findIndex(entry => entry.path === root.game.path)
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.horizontal: ScrollBar { policy: root.listMode ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded }
                        ScrollBar.vertical: ScrollBar { policy: root.listMode ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                        focus: true
                        Keys.onRightPressed: { root.moveSelection(1) }
                        Keys.onLeftPressed: { root.moveSelection(-1) }
                        Keys.onDownPressed: { root.moveSelection(1) }
                        Keys.onUpPressed: { root.moveSelection(-1) }
                        Keys.onReturnPressed: library.start()
                        delegate: Item {
                            id: tile
                            required property var modelData
                            width: root.listMode ? rail.width - 12 : (root.compact ? 104 : 152)
                            height: root.listMode ? 66 : (root.compact ? 110 : 184)
                            property bool selected: modelData.path === root.game.path
                            Rectangle {
                                anchors.fill: parent; radius: 8
                                color: root.listMode ? (tile.selected ? "#30271F" : "#B0161616") : "transparent"
                                border.color: root.listMode && tile.selected ? "#B87950" : "transparent"
                            }
                            Rectangle {
                                id: coverFrame
                                width: root.listMode ? 54 : (root.compact ? 90 : 152); height: width
                                y: root.listMode ? 6 : 0; x: root.listMode ? 6 : 0
                                radius: 8; color: "#202025"
                                border.width: tile.selected ? 3 : 1
                                border.color: tile.selected ? "#D3A07A" : "#353535"
                                Text {
                                    anchors.centerIn: parent
                                    text: tile.modelData.name.substring(0, 1)
                                    color: "#777F8E"; font.pixelSize: root.listMode ? 24 : 54
                                }
                                Image {
                                    anchors.fill: parent; anchors.margins: 5
                                    source: tile.modelData.cover
                                    asynchronous: true; fillMode: Image.PreserveAspectFit
                                    sourceSize.width: 320; sourceSize.height: 320
                                }
                            }
                            Text {
                                x: root.listMode ? 76 : 0; y: root.listMode ? 12 : (root.compact ? 94 : 159)
                                width: root.listMode ? parent.width - 310 : parent.width
                                text: tile.modelData.name; color: tile.selected ? "#F7F8FB" : "#A7A7AF"
                                font.pixelSize: root.listMode ? 14 : 12
                                font.weight: tile.selected ? Font.DemiBold : Font.Normal
                                horizontalAlignment: root.listMode ? Text.AlignLeft : Text.AlignHCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.listMode; x: 76; y: 36
                                text: tile.modelData.titleId + "   ·   " + tile.modelData.version
                                color: "#777F8E"; font.pixelSize: 12
                            }
                            Text {
                                visible: root.listMode
                                anchors.right: parent.right; anchors.rightMargin: 18; anchors.verticalCenter: parent.verticalCenter
                                text: qsTranslate("Library", tile.modelData.status); color: "#A7A7AF"; font.pixelSize: 12
                            }
                            MouseArea {
                                anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton
                                hoverEnabled: true
                                onClicked: mouse => {
                                    library.selectGame(tile.modelData.path)
                                    rail.forceActiveFocus()
                                    if (mouse.button === Qt.RightButton) gameMenu.popup()
                                }
                                onDoubleClicked: library.start()
                            }
                            Accessible.role: Accessible.ListItem
                            Accessible.name: modelData.name
                            Accessible.selected: selected
                            Accessible.onPressAction: library.selectGame(modelData.path)
                        }
                    }
                    ScrollView {
                        id: gameDetails
                        Layout.fillWidth: true
                        Layout.minimumHeight: detailsContent.implicitHeight
                        Layout.preferredHeight: detailsContent.implicitHeight
                        Layout.maximumHeight: detailsContent.implicitHeight
                        visible: root.hasGame && root.filteredGames.length > 0
                        contentWidth: availableWidth
                        clip: true
                        ColumnLayout {
                            id: detailsContent
                            width: gameDetails.availableWidth; spacing: root.compact ? 8 : 14
                            Rectangle { Layout.preferredWidth: 44; implicitHeight: 3; color: "#B87950"; radius: 1 }
                            Label {
                                objectName: "gameTitle"
                                text: root.game.name || ""
                                font.pixelSize: root.compact ? 26 : 42; font.weight: Font.DemiBold
                                color: "#F7F8FB"; wrapMode: Text.WordWrap
                                maximumLineCount: 2; elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 10
                                Repeater {
                                    model: [root.game.titleId || "Unknown title ID",
                                            root.game.version ? qsTr("Version %1").arg(root.game.version) : "",
                                            root.game.firmware ? qsTr("Firmware %1").arg(root.game.firmware) : "",
                                            root.game.status && root.game.status !== "Unknown" ? qsTr("Compatibility: %1").arg(qsTranslate("Library", root.game.status)) : qsTr("Compatibility: Unknown"),
                                            root.game.custom ? "Custom settings" : ""].filter(x => x.length > 0)
                                    Rectangle {
                                        required property string modelData
                                        implicitWidth: badge.implicitWidth + 20; implicitHeight: 27
                                        radius: 8; color: "#D0161616"; border.color: "#333333"
                                        Label { id: badge; anchors.centerIn: parent; text: qsTranslate("Library", modelData); color: "#A7A7AF"; font.pixelSize: 11 }
                                    }
                                }
                            }
                            Label {
                                visible: Boolean(root.game.comment)
                                text: root.game.comment || ""
                                color: "#A7A7AF"; wrapMode: Text.WordWrap
                                Layout.fillWidth: true; maximumLineCount: 3; elide: Text.ElideRight
                            }

                        }
                    }
                    RowLayout {
                        visible: root.hasGame && root.filteredGames.length > 0
                        Layout.minimumHeight: 50
                        Layout.maximumHeight: 50
                                spacing: 12; Layout.topMargin: 4
                                LibraryButton {
                                    objectName: "startButton"
                                    text: library.running ? (library.stopping ? qsTr("Stopping…") : qsTr("Stop")) : qsTr("Start")
                                    glyph: library.running ? "stop" : "play"
                                    primary: !library.running; danger: library.running
                                    outlined: library.running
                                    implicitWidth: 154; implicitHeight: 46; font.pixelSize: 15
                                    enabled: library.running ? !library.stopping : library.ready && !library.scanning
                                    onClicked: library.running ? library.stop() : library.start()
                                }
                                LibraryButton { text: ""; glyph: "more"; implicitWidth: 46; implicitHeight: 46; onClicked: gameMenu.popup() }
                                LibraryButton { text: qsTr("Console"); glyph: "console"; outlined: root.consoleOpen; implicitHeight: 46; onClicked: root.consoleOpen = !root.consoleOpen }
                            }
                    Item { visible: root.hasGame; Layout.fillHeight: true }
                    ColumnLayout {
                        visible: root.filteredGames.length === 0
                        Layout.fillWidth: true; Layout.fillHeight: true
                        Item { Layout.fillHeight: true }
                        Label {
                            text: library.scanning ? qsTr("Scanning your games…") : library.games.length ? qsTr("No matching games") : qsTr("Your library starts here")
                            color: "#F7F8FB"; font.pixelSize: 28; font.weight: Font.DemiBold
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: library.games.length ? qsTr("Try a different title or serial ID.") : qsTr("Add a folder containing your games to get started.")
                            color: "#777F8E"; font.pixelSize: 14; Layout.alignment: Qt.AlignHCenter
                        }
                        LibraryButton {
                            text: qsTr("+ Add game folder"); primary: true; visible: !library.games.length
                            Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 12
                            enabled: !library.running; onClicked: library.addFolder()
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
            Rectangle {
                id: consolePanel
                objectName: "consolePanel"
                visible: root.consoleOpen
                Layout.fillWidth: true
                Layout.minimumHeight: 80
                Layout.maximumHeight: root.consoleMaximumHeight
                Layout.preferredHeight: Math.min(root.consoleHeight, root.consoleMaximumHeight)
                color: "#E8111111"; radius: 8; border.color: "#333333"
                MouseArea {
                    objectName: "consoleResizeHandle"
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    height: 8; z: 2; hoverEnabled: true
                    cursorShape: Qt.SizeVerCursor
                    property real initialY
                    property real initialHeight
                    onPressed: mouse => {
                        initialY = mapToItem(root, mouse.x, mouse.y).y
                        initialHeight = consolePanel.height
                    }
                    onPositionChanged: mouse => {
                        if (pressed) {
                            const delta = initialY - mapToItem(root, mouse.x, mouse.y).y
                            root.consoleHeight = Math.max(80, Math.min(root.consoleMaximumHeight, initialHeight + delta))
                        }
                    }
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top
                        anchors.topMargin: 2; width: 44; height: 3; radius: 1.5
                        color: parent.containsMouse || parent.pressed ? "#B87950" : "#454545"
                    }
                }
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 8
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        Label { text: qsTr("Console"); color: "#F7F8FB"; font.pixelSize: 14; font.weight: Font.DemiBold }
                        TextField {
                            Layout.fillWidth: true; implicitHeight: 32
                            placeholderText: qsTr("Filter logs…"); color: "#F7F8FB"; placeholderTextColor: "#777F8E"
                            onTextChanged: library.filterLog(text)
                            background: Rectangle { color: "#202020"; radius: 8 }
                        }
                        CheckBox { id: autoScroll; text: qsTr("Auto-scroll"); checked: true; palette.windowText: "#A7A7AF" }
                        LibraryButton { text: qsTr("Copy"); implicitHeight: 32; onClicked: library.copyLog() }
                        LibraryButton { text: qsTr("Export"); implicitHeight: 32; onClicked: library.exportLog() }
                        LibraryButton { text: qsTr("Clear"); implicitHeight: 32; onClicked: library.clearLog() }
                        LibraryButton { text: "×"; implicitHeight: 32; onClicked: root.consoleOpen = false }
                    }
                    ListView {
                        id: logView
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: library.logModel
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Text {
                            required property string line
                            width: logView.width - 14
                            text: line; textFormat: Text.PlainText
                            font.family: "Roboto Mono"
                            font.pixelSize: 12
                            color: /error|failed|crash/i.test(line) ? "#FF858D" : /warn/i.test(line) ? "#EAC477" :
                                   line.indexOf("[Library]") === 0 ? "#D3A07A" : "#B6BCC7"
                            wrapMode: Text.WrapAnywhere
                        }
                        Label {
                            visible: logView.count === 0; anchors.centerIn: parent
                            text: qsTr("Process output will appear here."); color: "#777F8E"
                        }
                    }
                }
            }
            RowLayout {
                id: footer
                Layout.fillWidth: true
                Label {
                    Layout.leftMargin: -8
                    text: library.games.length + (library.games.length === 1 ? qsTr(" game") : qsTr(" games"))
                    color: "#777F8E"; font.pixelSize: 11
                }

            }
        }
    }
    // Let the window manager resize from every edge/corner, including on Wayland.
    Repeater {
        model: [Qt.TopEdge, Qt.BottomEdge, Qt.LeftEdge, Qt.RightEdge,
                Qt.TopEdge | Qt.LeftEdge, Qt.TopEdge | Qt.RightEdge,
                Qt.BottomEdge | Qt.LeftEdge, Qt.BottomEdge | Qt.RightEdge]
        MouseArea {
            required property int modelData
            readonly property bool leftEdge: Boolean(modelData & Qt.LeftEdge)
            readonly property bool rightEdge: Boolean(modelData & Qt.RightEdge)
            readonly property bool topEdge: Boolean(modelData & Qt.TopEdge)
            readonly property bool bottomEdge: Boolean(modelData & Qt.BottomEdge)
            x: leftEdge ? 0 : rightEdge ? root.width - 8 : 8
            y: topEdge ? 0 : bottomEdge ? root.height - 8 : 8
            width: leftEdge || rightEdge ? 8 : root.width - 16
            height: topEdge || bottomEdge ? 8 : root.height - 16
            z: 100
            enabled: !windowChrome.maximized
            visible: enabled
            hoverEnabled: true
            cursorShape: (leftEdge && topEdge) || (rightEdge && bottomEdge) ? Qt.SizeFDiagCursor :
                         (rightEdge && topEdge) || (leftEdge && bottomEdge) ? Qt.SizeBDiagCursor :
                         leftEdge || rightEdge ? Qt.SizeHorCursor : Qt.SizeVerCursor
            onPressed: windowChrome.beginWindowResize(modelData)
        }
    }
    component GameMenuItem: MenuItem {
        id: menuEntry
        implicitWidth: 264
        implicitHeight: 40
        leftPadding: 12
        rightPadding: 12
        hoverEnabled: true
        font.pixelSize: 14
        font.weight: Font.Medium
        contentItem: Text {
            text: menuEntry.text
            font: menuEntry.font
            color: menuEntry.enabled ? "#FFFFFF" : "#929292"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 8
            color: menuEntry.down ? "#65432D" : menuEntry.highlighted ? "#493326" : "transparent"
            border.width: 1
            border.color: menuEntry.visualFocus ? "#E59455" : "transparent"
        }
    }
    component GameMenuSeparator: MenuSeparator {
        topPadding: 6
        bottomPadding: 6
        leftPadding: 12
        rightPadding: 12
        contentItem: Rectangle {
            implicitWidth: 240
            implicitHeight: 1
            color: "#414141"
        }
    }
    Menu {
        id: gameMenu
        popupType: Popup.Item
        implicitWidth: 280
        padding: 8
        spacing: 2
        margins: 12
        font.pixelSize: 14
        background: Rectangle {
            color: "#202020"
            radius: 14
            border.width: 1
            border.color: "#E59455"
        }
        palette.window: "#242424"
        palette.base: "#242424"
        palette.text: "#F7F8FB"
        palette.windowText: "#F7F8FB"
        palette.buttonText: "#F7F8FB"
        palette.highlight: "#493528"
        palette.highlightedText: "#F7F8FB"
        GameMenuItem { text: qsTr("Game settings"); enabled: root.hasGame && !library.running; onTriggered: root.showOptions(false) }
        GameMenuItem { text: qsTr("Open game folder"); onTriggered: library.action("folder") }
        GameMenuItem { text: qsTr("Copy path"); onTriggered: library.action("copyPath") }
        GameMenuItem { text: qsTr("Copy title ID"); onTriggered: library.action("copyTitleId") }
        GameMenuSeparator {}
        GameMenuItem { text: qsTr("View trophies"); enabled: library.canViewTrophies; onTriggered: library.action("trophies") }
        GameMenuItem { text: qsTr("Cheats (experimental)"); visible: library.canPatch; height: visible ? implicitHeight : 0; enabled: !library.running; onTriggered: library.action("patches") }
        GameMenuItem { text: qsTr("Edit compatibility"); visible: library.localCompatibility; height: visible ? implicitHeight : 0; onTriggered: compatibilityDialog.open() }
        GameMenuSeparator {}
        GameMenuItem { text: qsTr("Clear custom settings"); enabled: Boolean(root.game.custom) && !library.running; onTriggered: library.action("reset") }
        GameMenuItem { text: qsTr("Remove save data…"); enabled: !library.running; onTriggered: library.action("saves") }
    }
    Dialog {
        id: navigationDialog
        anchors.centerIn: parent
        title: qsTr("Unsaved changes")
        standardButtons: Dialog.Discard | Dialog.Cancel
        palette.window: "#242424"; palette.windowText: "#F7F8FB"
        Label { text: qsTr("Discard your unsaved settings?"); color: "#F7F8FB" }
        onDiscarded: { settingsPage.dirty = false; root.options = false }
    }
    Dialog {
        id: compatibilityDialog
        anchors.centerIn: parent; width: Math.min(500, root.width - 60)
        title: qsTr("Local compatibility")
        standardButtons: Dialog.Save | Dialog.Cancel
        palette.window: "#242424"; palette.windowText: "#F7F8FB"
        onOpened: {
            compatibilityStatus.currentIndex = root.game.statusIndex || 0
            compatibilityComment.text = root.game.comment || ""
        }
        ColumnLayout {
            width: parent.width
            LibraryComboBox { id: compatibilityStatus; model: ["Unknown", "In game", "Logo", "Doesn't boot", "Main menu"]; Layout.fillWidth: true }
            TextField { id: compatibilityComment; placeholderText: qsTr("Comment"); Layout.fillWidth: true }
        }
        onAccepted: library.setCompatibility(compatibilityStatus.currentIndex, compatibilityComment.text)
    }
}

