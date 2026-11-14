// SPDX-License-Identifier: MIT
import QtQuick
import "../details" as Details
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

ColumnLayout {
    id:toolbar
    property bool backOnly:false
    property string layoutMode:"spotlight"
    property Item searchHost:null
    function applySearch() {appletProxyModel.setFilterFixedString(layoutMode==="spotlight"?"":search.text);}
    onLayoutModeChanged:Qt.callLater(applySearch)
    function revealSearch() { Qt.callLater(focusSearch); }
    function focusSearch() {
        if (!search.parent) return;
        let viewport = search.parent;
        while (viewport && !(viewport instanceof Flickable)) viewport = viewport.parent;
        if (viewport) {
            const top = search.mapToItem(viewport, 0, 0).y;
            if (top < 8 || top + search.height > viewport.height - 8) {
                const last = viewport.originY + viewport.contentHeight - viewport.height;
                viewport.contentY = Math.max(viewport.originY, Math.min(last, viewport.contentY + top - 8));
            }
        }
        search.forceActiveFocus();
        search.selectAll();
    }
    signal backRequested()
    property bool hasConnections
    property alias searchTextField:search
    readonly property bool displayWifiOffMessage: !wifi.checked && wifi.visible
    readonly property bool displayWifiConnectingMessage: wifi.checked && wifi.visible && search.text.length===0
    readonly property bool displayWwanMessage: !wwan.checked && wwan.visible
    readonly property bool displayplaneModeMessage: plane.checked && plane.visible
    readonly property bool hotspotAvailable: availableDevices.wirelessDeviceAvailable
    readonly property bool hotspotSupported: handler.hotspotSupported
    readonly property bool hotspotActive: handler.hotspotActive
    readonly property bool qrAvailable: qrca.available
    readonly property bool settingsAllowed: mainWindow.kcmAuthorized
    readonly property string hotspotHint: !wifi.checked ? i18n("Cannot create a hotspot because Wi-Fi is disabled.")
        : !handler.hotspotSupported ? i18n("All wireless radios are in use. Disconnect from Wi-Fi or connect another wireless radio.")
        : handler.hotspotActive ? i18n("Disable Hotspot") : i18n("Create Hotspot")
    function toggleHotspot() {
        if (!hotspotSupported) return;
        if (PlasmaNM.Configuration.hotspotConnectionPath) handler.stopHotspot(); else handler.createHotspot();
    }
    function scanQr() { if(qrca.available) qrca.launch(); }
    function settings() { if(settingsAllowed) KCMUtils.KCMLauncher.openSystemSettings(mainWindow.kcm); }
    function scan() { handler.requestScan(); }
    PlasmaNM.QrcaHandler { id:qrca }
    spacing:8
    RowLayout {
        Layout.fillWidth:true; spacing:6;visible:!toolbar.backOnly
        NetworkRadio {
            id:wifi;objectName:"networkWifi";Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:1
            visible:availableDevices.wirelessDeviceAvailable
            enabled:!PlasmaNM.Configuration.airplaneModeEnabled && enabledConnections.wirelessHwEnabled
            title:i18n("Wi-Fi");iconName:"network-wireless-symbolic";checked:enabled && enabledConnections.wirelessEnabled;busy:handler.scanning
            onToggled:handler.enableWireless(!enabledConnections.wirelessEnabled)
        }
        NetworkRadio {
            id:wwan;objectName:"networkMobile";Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:1
            visible:availableDevices.modemDeviceAvailable
            enabled:!PlasmaNM.Configuration.airplaneModeEnabled && enabledConnections.wwanHwEnabled
            title:i18n("Mobile");iconName:"network-mobile-symbolic";checked:enabled && enabledConnections.wwanEnabled
            onToggled:handler.enableWwan(!enabledConnections.wwanEnabled)
        }
        NetworkRadio {
            id:plane;objectName:"networkAirplane";Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:1
            visible:availableDevices.modemDeviceAvailable || availableDevices.wirelessDeviceAvailable
            title:i18n("Airplane");iconName:"network-flightmode-on";checked:PlasmaNM.Configuration.airplaneModeEnabled
            onToggled:{const value=!PlasmaNM.Configuration.airplaneModeEnabled;handler.enableAirplaneMode(value);PlasmaNM.Configuration.airplaneModeEnabled=value;}
            PC.ToolTip {text:plane.checked?i18n("Turn off airplane mode"):i18n("Turn on airplane mode (disables Wi-Fi and Bluetooth)")}
        }
    }

    Details.DetailSearchField {
        focusSequences: [] // The popup also reveals the field when it is scrolled out of view.
        parent:toolbar.searchHost
        visible:!toolbar.backOnly && parent!==null
        width:parent?parent.width:0
        height:implicitHeight
        id:search;objectName:"networkSearch";Layout.fillWidth:true;placeholderText:i18n("Search networks…")
        background:Details.DetailSurface { radius:height/2; property var margins:({left:8,right:8,top:7,bottom:7}) }
        implicitHeight:Math.max(48,contentHeight+30)
        Layout.minimumHeight:implicitHeight
        topPadding:12;bottomPadding:12
        verticalAlignment:TextInput.AlignVCenter
        enabled:true
        focus:mainWindow.expanded && !Kirigami.InputMethod.willShowOnActive
        onTextChanged:toolbar.applySearch()
    }
    NetworkActionButton { visible:toolbar.backOnly;flat:true;text:i18n("Back to connections");icon.name:"go-previous-symbolic";onClicked:toolbar.backRequested() }
}
