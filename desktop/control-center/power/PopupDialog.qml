// SPDX-License-Identifier: LGPL-2.0-or-later
// Native PowerDevil and battery models, presented through Sequoia's power view.
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kcmutils as KCMUtils
import org.kde.config as KConfig

PlasmaExtras.Representation {
    id: dialog
    property var model
    property bool pluggedIn
    property int chargeStopThreshold
    property bool isManuallyInhibited
    property bool isManuallyInhibitedError

    property int remainingTime

    property bool profilesInstalled
    property string activeProfile
    property string activeProfileError
    property var profiles

    property bool isTlpInstalled

    // List of currently requested power management inhibitions (applications that
    // are blocking sleep and screen locking). Not all are necessarily active.
    //
    // type: [{
    //  appName: string,
    //  prettyName: string,
    //  icon: string,
    //  reason: string,
    //  behaviors: array<string>, // what's inhibited: "idle", "sleep", both, or any future inhibition types
    //  active: bool,
    //  allowed: bool,
    // }]
    property var requestedInhibitions: []
    property bool inhibitsLidAction

    property string inhibitionReason
    property string degradationReason
    // type: [{ name: string, icon: string, profile: string, reason: string }]
    required property var profileHolds

    signal inhibitionChangeRequested(bool inhibit)
    signal inhibitionAllowedRequested(string appName, string reason, bool allowed)
    signal activateProfileRequested(string profile)

    objectName: "sequoiaDetailPage"
    padding: 0
    background: null
    collapseMarginsHint: true
    implicitWidth: 418
    implicitHeight: powerView.implicitHeight
    property string errorMessage: ""
    onActiveProfileErrorChanged: if (activeProfileError) errorMessage = qsTr("Could not change the power mode. Try again.")
    onIsManuallyInhibitedErrorChanged: if (isManuallyInhibitedError) errorMessage = qsTr("Could not change sleep and screen locking. Try again.")
    QtObject {
        id: controller
        readonly property string activeProfile: dialog.activeProfile
        readonly property var profiles: dialog.profiles || []
        readonly property bool profilesInstalled: dialog.profilesInstalled
        readonly property bool isTlpInstalled: dialog.isTlpInstalled
        readonly property string inhibitionReason: dialog.inhibitionReason
        readonly property string degradationReason: dialog.degradationReason
        readonly property var profileHolds: dialog.profileHolds
        readonly property bool isManuallyInhibited: dialog.isManuallyInhibited
        readonly property var requestedInhibitions: dialog.requestedInhibitions
        readonly property bool inhibitsLidAction: dialog.inhibitsLidAction
        readonly property string errorMessage: dialog.errorMessage
        readonly property bool settingsAllowed: KConfig.KAuthorized.authorizeControlModule("kcm_powerdevilprofilesconfig")
        readonly property bool energyAllowed: KConfig.KAuthorized.authorizeControlModule("kcm_energyinfo")
        function activateProfile(profile) {
            if (!profiles.includes(profile) || (profile === "performance" && inhibitionReason)) return;
            dialog.errorMessage = "";
            dialog.activateProfileRequested(profile);
        }
        function setInhibited(value) { dialog.errorMessage = ""; dialog.inhibitionChangeRequested(value); }
        function settings() { if (settingsAllowed) KCMUtils.KCMLauncher.openSystemSettings("kcm_powerdevilprofilesconfig"); }
        function energy() { if (energyAllowed) KCMUtils.KCMLauncher.openInfoCenter("kcm_energyinfo"); }
    }
    PowerView {
        id: powerView
        anchors.fill: parent
        controller: controller
        batteryCards: Component {
            ColumnLayout {
                spacing: 12
                Repeater {
                    id: batteryRepeater
                    model: dialog.model
                    BatteryItem {
                        required property var model
                        Layout.fillWidth: true
                        batteryPercent: model.Percent
                        batteryCapacity: model.Capacity
                        batteryEnergy: model.Energy
                        batteryPluggedIn: model.PluggedIn
                        batteryIsPowerSupply: model.IsPowerSupply
                        batteryChargeState: model.ChargeState
                        batteryPrettyName: model.PrettyName
                        batteryType: model.Type
                        remainingTime: dialog.remainingTime
                        pluggedIn: dialog.pluggedIn
                        chargeStopThreshold: dialog.chargeStopThreshold
                    }
                }
            }
        }
        inhibitionDetails: Component {
            InhibitionItem {
                detailsOnly: true
                requestedInhibitions: dialog.requestedInhibitions
                isManuallyInhibited: dialog.isManuallyInhibited
                isManuallyInhibitedError: false
                inhibitsLidAction: dialog.inhibitsLidAction
                pluggedIn: dialog.pluggedIn
                onInhibitionAllowedRequested: (name, reason, allowed) => dialog.inhibitionAllowedRequested(name, reason, allowed)
            }
        }
    }
}
