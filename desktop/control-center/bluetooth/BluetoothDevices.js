// SPDX-License-Identifier: MIT
.pragma library

function connectionOrder(devices, previous) {
    const connected = devices.filter(d => d.connected).map(d => d.id);
    return previous.filter(id => connected.includes(id))
        .concat(connected.filter(id => !previous.includes(id)));
}

function active(device) { return !!device.connected && (!!device.playing || !!device.transferring); }

// Preserve connection order; manual choices last only while their activity lasts.
function selection(devices, previous, preferredOrder) {
    const order = connectionOrder(devices, preferredOrder.length ? preferredOrder : (previous.order || []));
    const activeIds = order.filter(id => active(devices.find(d => d.id === id)));
    const started = activeIds.filter(id => !(previous.activeIds || []).includes(id));
    let manualId = activeIds.includes(previous.manualId) ? previous.manualId : "";
    if (started.length) manualId = "";
    const featuredId = manualId || started[0]
        || (activeIds.includes(previous.featuredId) ? previous.featuredId : activeIds[0])
        || order[0] || devices[0]?.id || "";
    return {order: order, activeIds: activeIds, featuredId: featuredId, manualId: manualId};
}

// Only an active transport proves audio is flowing through this computer.
// Remote player status can describe phone-local playback or remain stale.
function dbusValue(value) {
    // Qt may preserve a D-Bus variant wrapper inside nested ObjectManager maps.
    return value !== null && typeof value === "object" && value.value !== undefined ? value.value : value;
}

function playingDevices(objects) {
    const result = {};
    for (const path of Object.keys(objects)) {
        const transport = objects[path]["org.bluez.MediaTransport1"];
        if (dbusValue(transport?.State) === "active" && transport.Device) result[String(dbusValue(transport.Device))] = true;
    }
    return result;
}

function transferringDevices(objects) {
    const result = {};
    for (const path of Object.keys(objects)) {
        const transfer = objects[path]["org.bluez.obex.Transfer1"];
        if (dbusValue(transfer?.Status) !== "active") continue;
        const session = objects[String(dbusValue(transfer.Session))]?.["org.bluez.obex.Session1"];
        if (session?.Destination) result[String(dbusValue(session.Destination)).toUpperCase()] = true;
    }
    return result;
}

function battery(value) {
    return typeof value === "number" && Number.isFinite(value) && value >= 0 && value <= 100 ? value : -1;
}
