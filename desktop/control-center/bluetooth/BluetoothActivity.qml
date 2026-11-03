// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.bluezqt as BluezQt
import org.kde.plasma.workspace.dbus as DBus
import "BluetoothDevices.js" as Devices

Item {
    id: activity
    // Lives with the applet, so connections made while its popup is closed count.
    property bool monitoring: false
    readonly property var connectedDevices: BluezQt.Manager.connectedDevices
    property var connectionOrder: []
    property var playing: ({})
    property var transferring: ({})
    property bool audioPending: false
    property bool transferPending: false
    property int generation: 0
    onConnectedDevicesChanged: updateOrder()
    Component.onCompleted: updateOrder()
    function updateOrder() {
        connectionOrder = Devices.connectionOrder(connectedDevices.map(d => ({id: d.ubi, connected: true})), connectionOrder);
    }
    onMonitoringChanged: {
        ++generation;
        if (monitoring) poll();
        else { playing = {}; transferring = {}; }
    }
    DBus.DBusServiceWatcher {
        id: bluez
        busType: DBus.BusType.System
        watchedService: "org.bluez"
        onRegisteredChanged: { if (!registered) activity.playing = {}; else activity.poll(); }
    }
    DBus.DBusServiceWatcher {
        id: obex
        busType: DBus.BusType.Session
        watchedService: "org.bluez.obex"
        onRegisteredChanged: { if (!registered) activity.transferring = {}; else activity.poll(); }
    }
    // Read-only snapshots, only while the popup is open. Never launch the OBEX
    // service or start a stream/transfer merely to determine device activity.
    function poll() {
        if (!monitoring) return;
        const epoch = generation;
        if (bluez.registered && !audioPending) {
            audioPending = true;
            DBus.SystemBus.asyncCall({service: "org.bluez", path: "/", iface: "org.freedesktop.DBus.ObjectManager", member: "GetManagedObjects"}, reply => {
                audioPending = false;
                if (monitoring && epoch === generation) playing = Devices.playingDevices(reply.value || {});
            }, error => { audioPending = false; if (epoch === generation) playing = {}; });
        }
        if (obex.registered && !transferPending) {
            transferPending = true;
            DBus.SessionBus.asyncCall({service: "org.bluez.obex", path: "/", iface: "org.freedesktop.DBus.ObjectManager", member: "GetManagedObjects"}, reply => {
                transferPending = false;
                if (monitoring && epoch === generation) transferring = Devices.transferringDevices(reply.value || {});
            }, error => { transferPending = false; if (epoch === generation) transferring = {}; });
        }
    }
    Timer { interval: 1000; repeat: true; running: activity.monitoring; onTriggered: activity.poll() }
}
