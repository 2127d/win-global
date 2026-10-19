# Sequoia Eleven Control Center

The panel icon's slider knobs glide inward on hover, swap sides when the popup
opens, and return when it closes. Timing follows Plasma's animation-speed setting,
including disabled animations.

The existing Plasma tray's main popup becomes a compact dashboard. Its default
297 × 451 logical-pixel content area is approximately 321 × 475 pixels including
Plasma's frame, matching the supplied reference. Sequoia System Tray settings
include a **Size** page for the dashboard's saved width and height. Width ranges
from 260 to 1600 logical pixels and height from 240 to 1600; the popup is capped
to the current screen. Smaller heights keep the controls scrollable. Apply saves
the dimensions, and Reset to Default Size restores the original size. Native detail pages
expand independently and Back restores the configured dashboard. Network, Bluetooth,
notifications, PulseAudio/PipeWire, PowerDevil brightness and MPRIS media use the
installed KDE QML backends. No polling shell commands or simulated controls are
used in the installed dashboard. Detail arrows open the existing native applets.

Dashboard contents follow each item's System Tray visibility setting live.
Enabled items set to Show when relevant or Show only in popup are eligible;
Always show and Never show (disabled) are excluded. Always show all excludes
all tray entries from the dashboard. Audio's setting controls both Volume and
Microphone. Existing purpose-built controls appear only once; other applets,
application tray icons, and background apps use matching rounded cards directly
in the dashboard, with native activation and context menus. There is no separate
Tray items menu. Automatic items retain KDE's normal panel behavior; use Always
show for a panel-only item, or Show only in popup for a Control Center-only item.

The installer extracts the installed tray's own QML, retaining its copyright and
license notices, and patches its dashboard integration, expander and detail styling. The
original applet hosting, status icons, actions, and tray configuration remain. The
native hidden grid stays offscreen to preserve applet ownership and lifecycle.
New code in this directory is MIT licensed; extracted KDE code keeps its original
licenses. `Preview.qml` is a deterministic test fixture and is not installed.

The native KDE media popup has a dedicated player view in `media/`: labeled,
scrollable player tabs (including automatic selection), a softly tinted artwork
card, 20px rounded thumbnails, track metadata, and a bottom transport card with
seek position, elapsed/remaining time, shuffle, previous, prominent play/pause,
next and repeat controls. Tabs follow KDE's MPRIS model, so compatible browsers
and desktop players appear and disappear automatically. Artwork keeps its aspect
ratio; unsupported controls are disabled and unknown durations do not leave an
empty timeline. Player changes cancel pending seeks. The compact panel
representation and dashboard thumbnail retain their existing rendering.

The remaining detail menus use shared surfaces in `details/`: rounded navigation
buttons, inset toolbars, softly tinted content panels, card rows and pill tabs.
The shared tray header has 4px vertical margins around its controls. Headers and
main content panels use soft, theme-aware shadows instead of outlines, leaving
the outer translucent popup background unchanged. Nested controls retain their
outlines and keyboard focus indicators.
View-switching tabs use `DetailTabBar` to suppress Plasma's separate SVG
selection highlight, so hover and checked fills follow each button's pill shape.
Expandable device, network and printer rows likewise draw selection on their
rounded card surface. Audio rows draw keyboard focus within their inset cards.
Audio, brightness/color, battery/power, Bluetooth, networks, notifications,
clipboard, removable devices, printers and camera activity retain their native
models and actions. Clipboard search, favorites, editing and barcode views still
use Klipper; nested Back returns to its history before returning to the dashboard.
Scrollbar visuals are hidden throughout the active menu, including dynamically
created views, while scrolling and native focus handling remain enabled.
Other applets receive the common header and content surface. Their own content
and application-owned tray context menus retain their native layouts.

Notification banners and history share a quieter app header, 24px app icons,
crisp semibold titles, regular message text, inset close/settings controls, and
rounded tinted action buttons. Banners use the theme's native rounded surface,
with 24px edge clearance and 10px between stacked notifications. History cards
have 12px vertical padding. Light and dark appearances follow the desktop theme.

`tools/control_center_notifications.py` refines the extracted native views and
routes their Globals references to one local copy of KDE's singleton, allowing
live banners to use the same components as history. Native delivery, grouping,
timeouts, inhibition, inline replies, job controls and file actions remain in
KDE's implementation. `tools/verify-notifications.py` sends synthetic messages
only through a private D-Bus session and checks delivery, duplicate banners,
actions, replies, dismissal, long text, progress layout, expiry and DND/history.
Use `--dark --large --rtl` to inspect other layouts.

Weather uses its own popup components in `weather/`: a current-conditions card,
separate padded Day/Night forecast islands, detail cards, notice cards, and pill
tabs. Daily-only providers retain high/low temperatures on a single card. The
existing KDE forecast service, station settings and unit converters supply all
data. Its taller layout is capped to the display, and long content scrolls with
hidden scrollbars. Temperature zero and zero humidity remain valid readings.

Run `python3 tools/verify-weather-popup.py` for deterministic weather previews;
`--dark --large --rtl` and `--day-only` exercise other layouts. These tests check
forecast tables, card padding, details (including zero values), long notices,
page switching and loading/unavailable states without making weather requests.
The native `--details` test also checks the weather entry point and Back.

The Qt entry-point helper is scoped to the `plasma-plasmashell.service` process.
The launcher verifies SHA-256 hashes of every extracted applet plugin, copied
Klipper view and its plugin, libPlasma and libPlasmaQuick
before enabling it. If any differ, it launches the normal shell. It removes its
preload environment at Qt startup, so launched applications do not inherit it.
The service override is `~/.config/systemd/user/plasma-plasmashell.service.d/50-sequoia-control-center.conf`.

Build a staged copy without configuring the desktop:

```sh
python3 tools/install-control-center.py --stage --prefix .staging/control-center-data
```

Verification:

```sh
python3 tools/preview-control-center.py --dark
python3 tools/preview-control-center.py --large-font --rtl
python3 tools/preview-control-center.py --live
python3 tools/verify-control-center.py
python3 tools/verify-control-center.py --details
python3 tools/verify-control-center.py --details --dark --large --rtl
python3 tools/verify-control-center.py --details --graphics
python3 tools/verify-widget-archives.py --folder .staging/widget-archives --tray-size
python3 tools/verify-media-popup.py
python3 tools/verify-media-popup.py --dark --large
python3 tools/verify-media-popup.py --native
```

The fixture checks Space-key activation of six controls, keyboard volume changes,
and synchronization after an external volume update. The live preview only reads
device state. The native test creates a temporary configuration and private D-Bus
session, loads the actual tray, opens its dashboard, navigates to the native audio
applet, and returns home. It also changes temporary visibility settings for a
custom control and Clipboard, verifies all four choices and Always show all,
and opens Clipboard through its new card. It briefly displays a Wayland window and exits. The test
uses a separate Akonadi instance and restricts the enabled tray applets. No real
network, audio, Bluetooth, power, or session actions are triggered by these tests.

The `--details` test opens all twelve menus, checks their surface palette and size,
captures each page, and returns home. It also opens a sample Clipboard barcode
and exercises its native Back action without changing the clipboard. A private
D-Bus service name enables the power applets without starting PowerDevil.
Configuration, data, state and cache are isolated. The generic test platform's
application palette and font are set explicitly to match the selected appearance.
Captures use `preview/details-{light,dark-large-rtl}-APPLET.png`.
Add `--graphics` to render the panel shadows with the normal scene graph backend;
the default software renderer checks layout and navigation without shadows.

The media test supplies two mock MPRIS services on a private D-Bus, exercises
selection, playback, seeking, shuffle/repeat, player removal and the empty state,
and captures a preview. `--native` opens the staged redesign through the real
KDE system tray and checks tabs, artwork and Back navigation. The mock controls
only write to temporary logs; they never touch an actual media player.

The native test uses immediate process exit after verification to avoid an upstream
`plasmawindowed` shutdown crash where tray objects outlive the font database. The
test helper is never installed. Plasma's clipboard also requires a real display,
so Wayland is the default platform for native integration tests.

Verified on the installed Plasma 6.7.5 stack: dashboard loading, native audio
details and Back navigation, light/dark rendering, large text, RTL, keyboard
activation, unavailable devices, and live backend initialization. Fixture renders
are in `preview/control-center-dark.png` and
`preview/control-center-light-large-rtl.png`; native and live captures are separate.

Actual hardware switches and lock/power actions are wired to native services but
are deliberately not activated by the automated checks. On this desktop, the
brightness control is shown only when PowerDevil exposes a controllable display.

Merkuro Contacts uses a version-checked overlay under `contacts/`, preserving
its native ContactsModel, search proxy, AddresseeWrapper and avatar provider.
The list has a separate search surface, inset rows, alphabetical groups and
muted avatars. Profiles have padded information cards and call, message, email
and QR actions. Native Back walks from code to profile to the filtered list;
page changes fade using the desktop animation duration. No address-book data
is modified by the overlay. The exact Merkuro entry point is the only additional
URL redirected by the bridge.

`python3 tools/verify-contacts-popup.py` renders sample contacts and checks search,
keyboard activation, wrapping, QR sharing and search retention after Back; use
`--dark --large --rtl` for the alternate layout. `verify-control-center.py
--contacts` checks native loading and nested navigation with an isolated Akonadi
instance. The full `--details` run includes Contacts.


The icons-only task manager uses a private Qt resource namespace for its entry
point, configuration model and extended schema. Only its native plugin ID is
redirected. Its three configuration pages retain Plasma's Apply/Defaults flow;
Appearance and Behavior preserve native controls inside inset rounded cards.
`taskmanager/DockMotion.qml` supplies both dock motion and the settings preview.
The existing native reorder motion respects the new animation switches and speed.

Verify with `python3 tools/verify-dock.py` (also `--dark --large --rtl`) and
`python3 tools/verify-dock-motion.py`. These check live settings loading/Apply,
all configuration pages, hover styles/intensity, cancellation, press feedback,
startup motion and disabled motion. Native loading tests use isolated settings
and a private D-Bus session. See `../WIDGETS.md` for collection export/installation.
