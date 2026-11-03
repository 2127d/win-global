// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import org.kde.plasma.components as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

FocusScope {
    id: startMenu
    required property var configuration
    required property var backend
    readonly property bool submenuOpen: viewMenu.visible || powerMenu.visible
    property bool opened: false
    property bool launching: false
    property string page: "home"
    property string categoryTitle: ""
    property var categoryModel: null
    property var pendingModel: null
    property int pendingIndex: -1
    property string launchError: ""
    readonly property int motionDuration: configuration.animationsEnabled && Kirigami.Units.longDuration > 1 ? configuration.animationDuration : 0
    readonly property int feedbackDuration: motionDuration > 0 ? 100 : 0
    readonly property color textColor: Kirigami.Theme.textColor
    readonly property color secondaryColor: Qt.alpha(Kirigami.Theme.textColor, 0.60)
    readonly property color borderColor: Qt.alpha(Kirigami.Theme.textColor, 0.08)
    readonly property color hoverColor: Qt.alpha(Kirigami.Theme.highlightColor, 0.15)
    readonly property color tileColor: Qt.alpha(Kirigami.Theme.textColor, 0.04)
    readonly property color accentColor: Kirigami.Theme.highlightColor
    signal settingsRequested()
    signal dismissRequested()
    implicitWidth: configuration.menuWidth
    implicitHeight: configuration.menuHeight
    clip: true

    onOpenedChanged: {
        if (opened) {
            launchError = "";
            search.text = "";
            Qt.callLater(function() { if (startMenu.opened) search.forceActiveFocus(); });
        } else {
            viewMenu.closeMenu(false);
            powerMenu.closeMenu(false);
            searchDelay.stop();
            backend.query = "";
            // Let the native window hide before resetting navigation. Keep an
            // existing home page alive across the usual open/close cycle.
            Qt.callLater(function() {
                if (startMenu.opened || startMenu.page === "home") return;
                pages.replace(pageComponent, { destination: "home" }, QQC2.StackView.Immediate);
                startMenu.page = "home";
                startMenu.categoryModel = null;
            });
        }
    }
    function navigate(destination) {
        if (pages.busy || page === destination || launching) return;
        page = destination;
        // Replace animates in the same leftward direction for forward and Back.
        pages.replace(pageComponent, { destination: destination }, QQC2.StackView.ReplaceTransition);
    }
    function showCategory(model, title) {
        categoryModel = model;
        categoryTitle = title;
        navigate("category");
    }
    function goBack() {
        if (search.text.length) search.clear();
        else if (page !== "home") navigate("home");
        else dismissRequested();
    }
    function launch(model, index) {
        if (launching || !model) return;
        launching = true;
        pendingModel = model;
        pendingIndex = index;
        launchTimer.restart();
    }
    Timer {
        id: launchTimer
        interval: startMenu.configuration.shrinkOnLaunch ? Math.max(1, startMenu.motionDuration / 2) : 1
        onTriggered: {
            const accepted = startMenu.pendingModel.trigger(startMenu.pendingIndex, "", null);
            startMenu.pendingModel = null;
            startMenu.pendingIndex = -1;
            startMenu.launching = false;
            if (accepted) {
                if (startMenu.configuration.closeOnLaunch) startMenu.dismissRequested();
            } else startMenu.launchError = qsTr("This item could not be opened. It may no longer be available.");
        }
    }
    Keys.onEscapePressed: event => { goBack(); event.accepted = true; }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Left && (event.modifiers & Qt.AltModifier)) {
            goBack(); event.accepted = true;
        }
    }

    // PlasmaCore.Dialog supplies the theme's background, rounded mask and shadow.
    Item {
        id: surface
        width: parent.width; height: parent.height
        enabled: startMenu.opened

        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 84
                QQC2.TextField {
                    id: search
                    objectName: "startSearch"
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 20; rightMargin: 20 }
                    height: 44
                    leftPadding: 42; rightPadding: 42
                    placeholderText: qsTr("Search for apps, settings, and documents")
                    color: startMenu.textColor
                    placeholderTextColor: startMenu.secondaryColor
                    font.pixelSize: 13
                    background: MenuSurface {
                        radius: height / 2
                        elevated: true
                        focused: search.activeFocus
                        motionDuration: startMenu.feedbackDuration
                    }
                    selectByMouse: true
                    Accessible.name: placeholderText
                    onTextChanged: searchDelay.restart()
                    onAccepted: if (backend.searchResults && backend.searchResults.count > 0) startMenu.launch(backend.searchResults, 0)
                    Kirigami.Icon { source: Qt.resolvedUrl("../icons/search.svg"); isMask: true; color: startMenu.secondaryColor; x: 16; anchors.verticalCenter: parent.verticalCenter; width: 18; height: 18 }
                    MenuAction {
                        visible: search.text.length > 0
                        anchors.right: parent.right; anchors.rightMargin: 4; anchors.verticalCenter: parent.verticalCenter
                        width: 34; height: 34; symbol: "close"; iconOnly: true
                        text: qsTr("Clear search")
                        motionDuration: startMenu.feedbackDuration
                        onClicked: search.clear()
                    }
                }
            }
            Timer { id: searchDelay; interval: 120; onTriggered: backend.query = search.text.trim() }
            Text {
                visible: startMenu.launchError.length > 0
                Layout.fillWidth: true; Layout.leftMargin: 32; Layout.rightMargin: 32
                text: startMenu.launchError; color: startMenu.textColor; wrapMode: Text.Wrap
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true
                QQC2.StackView {
                    id: pages
                    objectName: "startPages"
                    anchors.fill: parent
                    visible: !search.text.trim().length
                    initialItem: pageComponent
                    replaceEnter: Transition {
                        NumberAnimation { property: "x"; from: pages.width; to: 0; duration: startMenu.motionDuration; easing.type: Easing.InOutCubic }
                    }
                    replaceExit: Transition {
                        NumberAnimation { property: "x"; from: 0; to: -pages.width; duration: startMenu.motionDuration; easing.type: Easing.InOutCubic }
                    }
                }
                QQC2.ScrollView {
                    anchors.fill: parent
                    anchors.leftMargin: 28; anchors.rightMargin: 28
                    visible: !!search.text.trim().length
                    contentWidth: availableWidth
                    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
                    QQC2.ScrollBar.vertical: MenuScrollBar {}
                    Column {
                        width: parent.width
                        spacing: 14
                        Text { text: qsTr("Search results"); color: startMenu.textColor; font.bold: true; font.pixelSize: 14 }
                        AppCollection { width: parent.width; menu: startMenu; sourceModel: backend.searchResults; populated: !!search.text.trim().length; listStyle: true }
                        Text {
                            width: parent.width
                            visible: !backend.searchResults || backend.searchResults.count === 0
                            text: qsTr("No results yet. Try an application, setting, or file name.")
                            color: startMenu.secondaryColor; wrapMode: Text.Wrap
                        }
                    }
                }
            }
            MenuSurface {
                objectName: "startAccountIsland"
                panel: true
                Layout.fillWidth: true; Layout.preferredHeight: 64
                Layout.leftMargin: 20; Layout.rightMargin: 20
                Layout.topMargin: 10; Layout.bottomMargin: 14
                elevated: true
                motionDuration: startMenu.feedbackDuration
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12
                    spacing: 12
                    Kirigami.ShadowedImage {
                        id: avatar
                        objectName: "startUserAvatar"
                        Layout.preferredWidth: 34; Layout.preferredHeight: 34
                        source: backend.avatar
                        fillMode: Image.PreserveAspectCrop
                        radius: width / 2
                        color: Kirigami.Theme.alternateBackgroundColor
                        Accessible.name: backend.userName
                        Kirigami.Icon {
                            anchors.fill: parent
                            anchors.margins: 5
                            source: "user-identity"
                            visible: avatar.status !== Image.Ready
                        }
                    }
                    Text { visible: configuration.showAccountName; text: backend.userName; color: startMenu.textColor; font.pixelSize: 13; font.weight: Font.Medium; elide: Text.ElideRight; Layout.maximumWidth: 230 }
                    Item { Layout.fillWidth: true }
                    MenuAction {
                        objectName: "startSettingsButton"
                        symbol: "configure"; iconOnly: true; raised: true
                        implicitHeight: 38
                        text: qsTr("Configure Start menu")
                        motionDuration: startMenu.feedbackDuration
                        onClicked: startMenu.settingsRequested()
                    }
                    MenuAction {
                        id: powerButton
                        symbol: "system-shutdown"; iconOnly: true; raised: true
                        implicitHeight: 38
                        selected: powerMenu.visible
                        text: qsTr("Power and session")
                        motionDuration: startMenu.feedbackDuration
                        objectName: "startPowerButton"
                        onClicked: powerMenu.openAt(powerButton, 0)
                    }
                }
            }
        }
    }

    // These dropdowns are separate native surfaces. A click in the parent
    // must dismiss them even when the clicked item does not take focus.
    MouseArea {
        objectName: "startDropdownDismissArea"
        anchors.fill: parent
        z: 100
        enabled: startMenu.submenuOpen
        acceptedButtons: Qt.AllButtons
        onPressed: {
            viewMenu.closeMenu(true);
            powerMenu.closeMenu(true);
        }
        onWheel: wheel => {
            viewMenu.closeMenu(true);
            powerMenu.closeMenu(true);
            wheel.accepted = true;
        }
    }

    function submenuDismissed(returnFocus) {
        if (!opened) return;
        if (returnFocus) {
            startMenu.Window.window.requestActivate();
        } else {
            Qt.callLater(function() {
                if (startMenu.opened && !startMenu.submenuOpen && !startMenu.Window.window.active)
                    startMenu.dismissRequested();
            });
        }
    }
    AnchoredMenu {
        id: viewMenu
        objectName: "startViewMenu"
        menuWidth: anchor ? anchor.width : 140
        model: [{display: qsTr("Category")}, {display: qsTr("List")}, {display: qsTr("Grid")}]
        onTriggered: index => configuration.appView = index
        onDismissed: returnFocus => startMenu.submenuDismissed(returnFocus)
    }
    AnchoredMenu {
        id: powerMenu
        objectName: "startPowerMenu"
        above: true
        model: backend.power
        onTriggered: index => {
            backend.power.trigger(index, "", null);
            startMenu.dismissRequested();
        }
        onDismissed: returnFocus => startMenu.submenuDismissed(returnFocus)
    }

    component Heading: RowLayout {
        property string title
        property string buttonText: ""
        property string buttonSymbol: "go-next"
        signal activated()
        width: parent.width
        Text { text: parent.title; color: startMenu.textColor; font.pixelSize: 15; font.weight: Font.DemiBold; Layout.fillWidth: true }
        MenuAction {
            visible: parent.buttonText.length > 0
            text: parent.buttonText
            trailingSymbol: parent.buttonSymbol
            motionDuration: startMenu.feedbackDuration
            onClicked: parent.activated()
        }
    }
    component EmptyHint: Text {
        width: parent.width
        color: startMenu.secondaryColor; font.pixelSize: 12; wrapMode: Text.Wrap
        topPadding: 10; bottomPadding: 18
    }
    Component {
        id: pageComponent
        QQC2.ScrollView {
            id: pageView
            property string destination: "home"
            readonly property bool showPinned: destination === "home" || destination === "pinned"
            readonly property bool showRecent: (destination === "home" && configuration.showRecommended) || destination === "recommended"
            readonly property bool showApps: (destination === "home" && configuration.showAllApps) || destination === "apps"
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            QQC2.ScrollBar.vertical: MenuScrollBar {}
            Column {
                width: pageView.availableWidth
                padding: 0
                Column {
                    x: 20; width: parent.width - 40
                    spacing: 20
                    topPadding: 4
                    bottomPadding: 20
                    Heading {
                        visible: pageView.destination !== "home"
                        title: pageView.destination === "pinned" ? qsTr("All pinned apps") : pageView.destination === "recommended" ? qsTr("All recommended") : pageView.destination === "category" ? startMenu.categoryTitle : qsTr("All apps")
                        buttonText: qsTr("Back"); buttonSymbol: "go-previous"
                        onActivated: startMenu.navigate("home")
                    }
                    MenuSection {
                        motionDuration: startMenu.feedbackDuration
                        visible: pageView.showPinned
                        width: parent.width; spacing: 8
                        MenuAction { visible: pageView.destination === "home" && !configuration.showAllApps; text: qsTr("All apps"); trailingSymbol: "go-next"; motionDuration: startMenu.feedbackDuration; onClicked: startMenu.navigate("apps") }
                        Heading { visible: pageView.destination === "home"; title: qsTr("Pinned"); buttonText: qsTr("Show all"); onActivated: startMenu.navigate("pinned") }
                        AppCollection {
                            width: parent.width; menu: startMenu; sourceModel: backend.favorites
                            populated: pageView.showPinned
                            limit: pageView.destination === "home" ? configuration.pinnedRows * configuration.gridColumns : -1
                        }
                        EmptyHint { visible: backend.favorites.count === 0; text: qsTr("Make this space yours. Right-click any app and choose Pin to Start.") }
                    }
                    MenuSection {
                        motionDuration: startMenu.feedbackDuration
                        visible: pageView.showRecent
                        width: parent.width; spacing: 8
                        Heading { visible: pageView.destination === "home"; title: qsTr("Recommended"); buttonText: qsTr("Show all"); onActivated: startMenu.navigate("recommended") }
                        AppCollection {
                            width: parent.width; menu: startMenu; sourceModel: backend.recent
                            populated: pageView.showRecent
                            limit: pageView.destination === "home" ? configuration.recommendationCount : -1
                            listStyle: true; columnCount: 2
                        }
                        EmptyHint { visible: backend.recent.count === 0; text: qsTr("Recently used apps and files will appear here when activity history is enabled in System Settings.") }
                    }
                    Column {
                        visible: pageView.showApps
                        width: parent.width; spacing: 12
                        RowLayout {
                            width: parent.width
                            Text { text: qsTr("All apps"); color: startMenu.textColor; font.pixelSize: 15; font.weight: Font.DemiBold; Layout.fillWidth: true }
                            MenuAction { visible: pageView.destination === "home"; text: qsTr("Show all"); trailingSymbol: "go-next"; motionDuration: startMenu.feedbackDuration; onClicked: startMenu.navigate("apps") }
                            MenuAction {
                                id: viewSelector
                                objectName: "appViewSelector"
                                text: viewMenu.model[configuration.appView].display
                                symbol: ["category", "view-list-details", "view-list-icons"][configuration.appView]
                                trailingSymbol: "go-down"
                                raised: true
                                selected: viewMenu.visible
                                implicitWidth: 150
                                Accessible.name: qsTr("All apps view: %1").arg(text)
                                Accessible.role: Accessible.ComboBox
                                motionDuration: startMenu.feedbackDuration
                                onClicked: viewMenu.openAt(viewSelector, configuration.appView)
                                Keys.onDownPressed: viewMenu.openAt(viewSelector, configuration.appView)
                                Keys.onUpPressed: viewMenu.openAt(viewSelector, configuration.appView)
                                WheelHandler {
                                    onWheel: event => {
                                        if (!event.angleDelta.y) return;
                                        configuration.appView = Math.max(0, Math.min(2,
                                            configuration.appView + (event.angleDelta.y < 0 ? 1 : -1)));
                                        event.accepted = true;
                                    }
                                }
                            }
                        }
                        CategoryGrid { visible: configuration.appView === 0; populated: pageView.showApps && configuration.appView === 0; width: parent.width; menu: startMenu }
                        AppCollection { visible: configuration.appView !== 0; populated: pageView.showApps && configuration.appView !== 0; width: parent.width; menu: startMenu; sourceModel: backend.apps; listStyle: configuration.appView === 1 }
                    }
                    AppCollection { visible: pageView.destination === "category"; populated: pageView.destination === "category"; width: parent.width; menu: startMenu; sourceModel: startMenu.categoryModel; listStyle: configuration.appView === 1 }
                }
            }
        }
    }
}
