// SPDX-License-Identifier: MIT
var panel = new Panel("org.sequoia.taskbar");
panel.location = "bottom";
panel.height = 52;
panel.floating = true;
// Task buttons are containment-owned QML, not child applets.
// Add a start menu or other Plasma widgets through the normal panel editor.
