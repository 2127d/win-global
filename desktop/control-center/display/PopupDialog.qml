// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.private.brightnesscontrolplugin
import org.kde.plasma.workspace.dbus as DBus
import org.kde.kcmutils
import org.kde.config as KConfig

PlasmaExtras.Representation {
    id: dialog
    required property ScreenBrightnessControl screenBrightnessControl
    required property KeyboardBrightnessControl keyboardBrightnessControl
    required property NightLightItem.NightLightControl nightLightControl
    property alias spotlight: displayView
    objectName: "sequoiaDetailPage"
    padding: 0; background: null; collapseMarginsHint: true
    implicitWidth: 418; implicitHeight: 600
    DisplayRecords {id:records;displayModel:dialog.screenBrightnessControl.displays}
    KeyboardColorControl {id:keyboardColor}
    QtObject {
        id:backend
        readonly property var displays:records.entries
        readonly property bool keyboardAvailable:dialog.keyboardBrightnessControl.isBrightnessAvailable
        readonly property bool keyboardColorAvailable:keyboardColor.supported
        readonly property var keyboardRecord:({id:"keyboard",name:qsTr("Keyboard Backlight"),brightness:dialog.keyboardBrightnessControl.brightness,maximum:dialog.keyboardBrightnessControl.brightnessMax})
        readonly property bool darkMode:DarkModeControl.darkMode
        readonly property string currentTheme:DarkModeControl.currentTheme
        readonly property string otherTheme:DarkModeControl.otherTheme
        readonly property bool settingsAllowed:KConfig.KAuthorized.authorizeControlModule("kcm_kscreen")
        readonly property bool themeSettingsAllowed:KConfig.KAuthorized.authorizeControlModule("kcm_landingpage")
        property string statusMessage:""
        function setBrightness(id,value) {
            if (!Number.isFinite(value)) return;
            if (id==="keyboard") {
                if (keyboardAvailable && keyboardRecord.maximum>0) dialog.keyboardBrightnessControl.brightness=Math.round(Math.max(0,Math.min(keyboardRecord.maximum,value)));
                return;
            }
            const target=displays.find(d=>d.id===id);
            if (target?.maximum>0) dialog.screenBrightnessControl.setBrightness(target.backendId,Math.round(Math.max(0,Math.min(target.maximum,value))));
        }
        function setDarkMode(enabled) { DarkModeControl.darkMode=enabled; }
        function settings() {if(settingsAllowed) KCMLauncher.openSystemSettings("kcm_kscreen");}
        function themeSettings() {if(themeSettingsAllowed) KCMLauncher.openSystemSettings("kcm_landingpage");}
        function identify() {
            statusMessage="";
            DBus.SessionBus.asyncCall({service:"org.kde.KWin",path:"/org/kde/KWin/Effect/OutputLocator1",iface:"org.kde.KWin.Effect.OutputLocator1",member:"show"},
                () => {}, () => {backend.statusMessage=qsTr("Display identification is unavailable. Open Display settings to locate your screens.");});
        }
    }
    DisplaySpotlightView {
        id:displayView;anchors.fill:parent;controller:backend
        keyboardColorCard: Component {
            KeyboardColorItem {text:qsTr("Keyboard Color");leftPadding:14;rightPadding:14;topPadding:14;bottomPadding:14}
        }
        nightLightCard: Component {
            DisplayNightLight {
                control:dialog.nightLightControl
                settingsAllowed:KConfig.KAuthorized.authorizeControlModule("kcm_nightlight")
                onConfigureRequested:KCMLauncher.openSystemSettings("kcm_nightlight")
                onToggleRequested:if(control.enabled && control.togglable) NightLightInhibitor.toggleInhibition()
            }
        }
    }
}
