# Content-based tray sizing

`ControlPopupSizing.qml` chooses dimensions for the active full representation.
The shared popup applies those dimensions on every applet switch, on reopening,
and when content hints change. Back restores the user's saved Control Center
width and height. The previous window's dimensions are never sizing inputs.

`ControlPopupGeometry.qml` samples content hints after a 16 ms settling delay.
Opening snaps to the current content size; changes while visible animate with
an OutCubic curve and KDE's `Kirigami.Units.longDuration`. Rapid changes retarget
from the current interpolated size. Zero duration disables motion. Closing stops
both animations and discards queued changes, so hidden menus cannot reopen.

Only the geometry component's rounded output drives native Layout constraints.
Live content measurements never resize the window synchronously during layout
polish; that separation prevents the earlier resize loop that blocked input.
Disks & Devices also fades newly filtered content in while the popup resizes.

Custom pages may expose `sequoiaPreferredWidth` and `sequoiaPreferredHeight` in
logical pixels, including their own header, footer and spacing. Compute these
from intrinsic card/layout sizes or list content heights, not the assigned
viewport height. The tray adds its own heading and margins. Existing widgets
fall back to their implicit sizes and native Layout preferences/minimums.

Weather, Bluetooth, Audio, Display, Network, Clipboard, Notifications, Display Configuration, KDE Connect, Disks & Devices, Printers, Dwell Clicker and Vaults supply
content measurements; Power already exposes its natural implicit height.
The shared policy caps detail pages at 680 by 720 logical pixels and the screen
bounds, leaving native scroll areas usable for longer content. Font sizes stay
unchanged. Lazy full representations attach when they finish constructing.

Regression checks:

- `QT_QPA_PLATFORM=offscreen /usr/lib64/qt6/bin/qmltestrunner -input desktop/control-center/tests/tst_popupsizing.qml`
- `QT_QPA_PLATFORM=offscreen /usr/lib64/qt6/bin/qmltestrunner -input desktop/control-center/tests/tst_popupgeometry.qml`
- `python3 tools/verify-tray-sizing.py --output /tmp/sequoia-tray-sizing-check`
- `python3 tools/verify-weather-popup.py --source STAGED_WEATHER --output OUTPUT`
- Repeat the weather check with `--dark --large --rtl --day-only`.

The native check uses a private Wayland/D-Bus session, switches directly among
fourteen widgets, verifies the repeated Weather size, and checks Back navigation,
closing on focus loss, and Escape after reopening.

The native test samples both content and native window heights while switching
Disks & Devices between Removable and Internal filters. For a meaningful size
change, it requires multiple intermediate sizes in both directions, then checks
that the window settles at the content target. It saves those samples as CSV.
