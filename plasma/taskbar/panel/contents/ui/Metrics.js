// SPDX-License-Identifier: MIT
.pragma library

function iconSize(mode, requested, thickness, padding) {
    const available = Math.max(1, thickness - 2 * padding);
    return Math.max(1, Math.floor(mode === "panel" ? available : Math.min(requested, available)));
}

function buttonLength(mode, icon, padding, spacing, count, available) {
    const minimum = icon + 2 * padding;
    if (mode !== "fill" || count < 1) return minimum;
    return Math.max(minimum, (Math.max(0, available) - Math.max(0, count - 1) * spacing) / count);
}

function offset(alignment, available, content) {
    const spare = Math.max(0, available - content);
    return alignment === "center" ? spare / 2 : alignment === "right" ? spare : 0;
}
