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

// Never use allocated button lengths here: fill mode would feed the current
// panel length back into Fit Content, preventing a dock from shrinking.
function naturalLength(icon, padding, spacing, count, showLauncher) {
    const cells = count + (showLauncher ? 1 : 0);
    return cells * (icon + 2 * padding) + Math.max(0, cells - 1) * spacing;
}
