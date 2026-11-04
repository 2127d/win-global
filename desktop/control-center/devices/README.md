# Disks & Devices

The Sequoia device page uses the installed `DeviceFilterControl`, `StateInfo`
and per-device `ActionsInfo` models. `DeviceItem.qml` translates their existing
roles into `DeviceCard.qml`; actions are dispatched with KDE's own identifiers.
There are no new disk operations or replacements for Solid's operation logic.

The three filters retain the existing removable/non-removable/all preferences.
Each card presents its name, type, native status and emblem, capacity (including
zero free space), default action and first application action. Expanding More
actions reveals every remaining native action. Busy or missing devices disable
operation buttons. Long unmounts retain the don't-unplug warning, and native
failure messages and filesystem-check/repair states remain visible.

The compact footer provides authorized automount settings, safely removing all
removable devices, and the existing popup-on-insertion preference. The original
context menu remains available. Cards and filters align with the tray header.
The content height grows with devices and expanded tools, independently of the
assigned viewport; long lists scroll within the tray's screen-aware cap.

Validation uses fixture action models, never operations on real storage:

- `python3 tools/verify-devices.py --output /tmp/disks-devices.png`
- Add `--render-only --dark --large --rtl` for the alternate layout preview.
- `python3 tools/verify-tray-sizing.py --output /tmp/tray-sizing`

The native tray test reads actual device rows in a private session, checks
independent sizes across eleven widgets, and verifies focus-loss and Escape
closing. It never triggers a device action.
