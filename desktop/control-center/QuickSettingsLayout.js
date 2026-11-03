.pragma library
// SPDX-License-Identifier: MIT
// Persistent logical sizes are independent of the current popup width.
function sizes(kind) {
    if (kind === "display") return [{w:3,h:2}, {w:6,h:2}, {w:6,h:3}];
    if (kind === "volume") return [{w:3,h:1}, {w:4,h:1}, {w:6,h:1}];
    if (kind === "media") return [{w:2,h:2}, {w:3,h:2}, {w:4,h:2}];
    if (kind === "output") return [{w:1,h:1}, {w:2,h:1}, {w:3,h:1}, {w:4,h:1}];
    if (kind === "tray") return [{w:1,h:1}, {w:2,h:1}, {w:3,h:1}, {w:2,h:2}];
    return [{w:1,h:1}, {w:2,h:1}, {w:3,h:1}];
}
function nearest(kind, w, h) {
    const choices = sizes(kind);
    let best = choices[0], distance = Infinity;
    for (const size of choices) {
        const d = Math.pow(size.w - w, 2) + Math.pow(size.h - h, 2);
        if (d < distance) { best = size; distance = d; }
    }
    return {w: best.w, h: best.h};
}
function defaults(catalog) {
    return catalog.filter(c => c.enabledByDefault !== false).map(c => ({id:c.id, w:c.w, h:c.h}));
}
function parse(json, catalog) {
    if (!json) return defaults(catalog);
    try {
        if (json.length > 32768) return defaults(catalog);
        const data = JSON.parse(json);
        if (data.version !== 1 || !Array.isArray(data.tiles)) return defaults(catalog);
        const seen = new Set(), result = [];
        for (const item of data.tiles.slice(0, 64)) {
            if (!item || typeof item.id !== "string" || !item.id.length || item.id.length > 256
                || seen.has(item.id)) continue;
            seen.add(item.id);
            const spec = catalog.find(c => c.id === item.id);
            const w = Number.isFinite(item.w) ? Math.max(1, Math.min(6, Math.round(item.w))) : 3;
            const h = Number.isFinite(item.h) ? Math.max(1, Math.min(3, Math.round(item.h))) : 1;
            // Keep unavailable app IDs, so reconnecting a device/app restores its place.
            const size = spec ? nearest(spec.kind, w, h) : {w:w, h:h};
            result.push({id:item.id, w:size.w, h:size.h});
        }
        return result;
    } catch (_) { return defaults(catalog); }
}
function serialize(tiles) {
    return JSON.stringify({version:1, tiles:tiles.map(t => ({id:t.id,w:t.w,h:t.h}))});
}
function span(tile, spec, columns) {
    let w = tile.w;
    if (columns === 4 && (spec.kind === "toggle" || spec.kind === "output" || spec.kind === "tray") && w === 3) w = 2;
    if (columns === 4 && (spec.kind === "volume" || spec.kind === "output") && w === 4) w = 2;
    return {w:Math.min(columns,w), h:tile.h};
}
function pack(tiles, catalog, columns) {
    const occupied = [], result = [];
    for (const tile of tiles) {
        const spec = catalog.find(c => c.id === tile.id);
        if (!spec) continue;
        const size = span(tile, spec, columns);
        let found = false;
        for (let row = 0; row < 192 && !found; ++row) {
            for (let col = 0; col <= columns - size.w && !found; ++col) {
                let fits = true;
                for (let y = row; y < row + size.h; ++y)
                    for (let x = col; x < col + size.w; ++x)
                        if (occupied[y * columns + x]) fits = false;
                if (!fits) continue;
                for (let y = row; y < row + size.h; ++y)
                    for (let x = col; x < col + size.w; ++x) occupied[y * columns + x] = true;
                result.push({id:tile.id, col:col, row:row, w:size.w, h:size.h, spec:spec,
                             savedW:tile.w, savedH:tile.h});
                found = true;
            }
        }
    }
    return result;
}
function move(tiles, id, targetId, after) {
    const result = tiles.map(t => ({id:t.id,w:t.w,h:t.h}));
    const from = result.findIndex(t => t.id === id);
    if (from < 0 || id === targetId) return result;
    const item = result.splice(from, 1)[0];
    let to = result.findIndex(t => t.id === targetId);
    if (to < 0) to = result.length;
    else if (after) ++to;
    result.splice(to, 0, item);
    return result;
}
