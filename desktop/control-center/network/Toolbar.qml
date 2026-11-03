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
    property bool searchExpanded:false
    function revealSearch() {searchExpanded=true;Qt.callLater(()=>{search.forceActiveFocus();search.selectAll();});}
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

    PlasmaExtras.SearchField {
        visible:!toolbar.backOnly && (toolbar.layoutMode!=="spotlight" || toolbar.searchExpanded || text.length>0)
        id:search;objectName:"networkSearch";Layout.fillWidth:true;placeholderText:i18n("Search networks…")
        background:Details.DetailSurface { radius:height/2; property var margins:({left:8,right:8,top:7,bottom:7}) }
        topPadding:7;bottomPadding:7
        enabled:true
        focus:mainWindow.expanded && !Kirigami.InputMethod.willShowOnActive
        onTextChanged:appletProxyModel.setFilterFixedString(text)
    }
    NetworkActionButton { visible:toolbar.backOnly;flat:true;text:i18n("Back to connections");icon.name:"go-previous-symbolic";onClicked:toolbar.backRequested() }
}
