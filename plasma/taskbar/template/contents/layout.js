// SPDX-License-Identifier: MIT
var panel = new Panel("org.sequoia.taskbar");
panel.location = "bottom";
panel.height = 52;
panel.lengthMode = "fill";
panel.floating = true;
// Launch menu and task buttons are containment-owned QML, not child applets.
// Users can add any Plasma widgets through the normal panel editor.
