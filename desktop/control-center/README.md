# Sequoia Eleven Control Center

The panel icon's slider knobs glide inward on hover, swap sides when the popup
opens, and return when it closes. Timing follows Plasma's animation-speed setting,
including disabled animations.

The dashboard has Android-style Quick Settings with an **Edit tiles** button.
Edit, System Settings, Lock, and Power live in a fixed glass island at the bottom
of the dialog. In edit mode it holds Cancel and Done. The tile viewport fades at
its scrolling edges, with enough bottom clearance to reveal the final tile fully.
Tap a toggle to change it, or hold it to open its native details. In edit mode,
controls cannot activate accidentally. Drag a tile to reorder it, use its corner
handle to resize it, or click the handle to cycle supported sizes. The toolbar
also provides a keyboard-accessible size picker, move buttons, and Remove.
Removed items appear under **Add tiles**; removing a tile does not disable its
native tray applet. Enable additional applets in System Tray settings to expose
them in the picker. Native application tray entries retain their existing click,
context-menu, middle-click, and scroll actions outside editing.

**Done** saves a versioned layout in this tray instance's `quickSettingsLayout`
configuration entry. **Cancel**, Escape, and closing the popup discard the draft.
**Reset layout** restores the defaults in the draft, so Cancel can undo it.
Missing app/device entries retain their saved positions for when they return.
Malformed settings fall back to defaults, while an intentionally empty layout
stays empty. Settings from unrelated tray instances are not changed.

The snapping grid has six columns at normal widths and four on narrow popups,
with 10px gaps and a maximum grid width of 482px. It packs tiles without overlaps.
Simple toggles support 1×1, 2×1 and 3×1; 1×1 uses an icon with a tooltip. Output
also supports 4×1. Native app tiles additionally support 2×2. Sound is limited to
3×1, 4×1 and 6×1, Display to 3×2, 6×2 and 6×3, and Media to 2×2, 3×2 and 4×2.
Wide spans adapt to narrow popups without rewriting the saved size. Sliders never
shrink to icon-only controls. Multiple displays scroll within their tile. The
defaults preserve the compact Media tile, matching Display/Media corners, and
pill-shaped quick controls. Additional apps stay available in the lower grid.

Run `python3 tools/verify-quick-settings.py` for real QML interaction tests,
including pointer drag/resize, keyboard actions, persistence, cancellation,
compact controls, all supported sizes, and responsive packing. Render the editor
with `python3 tools/preview-control-center.py --edit` (also supports `--dark`,
`--large-font`, `--rtl`, `--width`, `--height`, `--scroll-end`, and `--output`). These checks use private services and
fixtures and do not change the user's device settings.

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

The default layout follows each item's System Tray visibility setting live.
Enabled items set to Show when relevant or Show only in popup are eligible;
Always show and Never show (disabled) are excluded from that default. Always
show all starts with an empty default dashboard. The editor picker includes
enabled panel-only applets too; a saved custom layout explicitly chooses which
of those enabled items also appear in Quick Settings. Audio's setting controls both Volume and
Microphone. Existing purpose-built controls appear only once in the default; other applets,
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
Their cards and the Control Center navigation header share the dashboard's glass
material: translucent fills, soft reflections, bright rims, and subtle shadows.
Button hover, pressed, selected, and keyboard-focus backgrounds are pill-shaped.
The common glass components are opt-in, so other popup consumers keep their style.

Audio offers Output spotlight, Twin channels, and Mixer desk layouts in the
volume applet. The Layout picker in the bottom island remembers the selection
through the applet’s `audioLayout` configuration entry. Twin channels uses vertical
output/input faders; Mixer desk exposes each stream’s current audio destination
and lets it move between compatible devices without changing the default output.
All layouts share live audio state, volume limits, and mute/feedback behavior.
Its shared tray
header retains Back navigation; the selected output and microphone use glass
cards with native default-device selectors, mute buttons, and volume controls.
App volumes expands playback and recording streams, with native per-stream
options for routing. Sound settings and additional controls remain on a floating
bottom island; the scroll edges fade behind it. Device options retain native
ports/profiles, and the More menu exposes microphone testing, amplification, and
virtual devices. Existing audio models, global mute actions, feedback, and config
migration remain in the native main component. The media-player applet is separate.
Layout, More, device options, and device selectors share rounded glass menus with
pill highlights and selection checkmarks. Device options use the native audio
objects for routing, ports, and profiles; long menus scroll within the popup.
`tools/verify-audio-spotlight.py` tests interaction routing, external updates,
hotplug, volume limits, empty states, and narrow layouts using simulated devices.

Display offers Display spotlight, Twin displays, and Workspace map from the grid button
in its compact bottom action island. The selection is saved per applet in `displayLayout`.
Twin displays uses two columns (one on narrow panels), with optional proportional
linked brightness across the available displays. Workspace map provides numbered
selection tiles; arrangement and mirroring open Plasma’s screen settings, since the
brightness model does not expose physical geometry. All layouts retain Theme,
Night Light, keyboard controls, and native brightness ranges.

The spotlight layout uses numbered selectors for up to five brightness-capable
screens and a glass dropdown beyond five. One featured slider is visible initially;
All displays expands compact sliders without duplicating the featured slider. Screen
selection follows native display identity, while brightness values stay in each device’s
native range. Changes during a drag retain the original target, including disconnection.
The shared Theme card uses Plasma’s current/alternate themes and Dark Mode control.
Night Light retains its suspension, external inhibition, configuration, and transition
status. Keyboard backlight and accent-color controls appear when supported. Identify
uses KWin’s native output labels; selector numbers are local to the brightness list.
Display settings opens Plasma’s screen configuration for resolution and arrangement.
The fixed bottom island and faded scroll edges keep long lists clear. The tray gives
Display up to 660 pixels of height, bounded by the screen, and restores the dashboard
size on Back. `tools/verify-display-spotlight.py` covers selection, hotplug, dragging,
native ranges, Night Light, theme actions, overflow menus, and narrow layouts.

Networks offers Connection spotlight, Connection desk, and Compact connections from
the bottom island’s three-dot menu, saved per applet in `networkLayout`. Spotlight
features the first active non-VPN connection in a hero card, with secondary connections
inside it, configured VPN profiles in separate cards, and one grouped available-network
card (three rows initially, expandable). Radio cards have On/Off switches; Spotlight
search is accessible from the available-network card or Ctrl+F. Opening a connection
uses a separate native detail page, with no duplicate active rows on Spotlight.
Desk gives Wi-Fi and Ethernet separate
cards; Compact has All / Wi-Fi / VPN filters. Native delegates still own connection,
password, sharing, speed, and detailed information flows. The type filter forwards
password-entry delay updates through the proxy model, and releases expanded entry
locks before switching filters. Search, mobile data, airplane mode, captive portal
login, and native scanning remain available. Settings, Hotspot, and Scan QR stay in
the fixed bottom island. Hotspot retains the handler’s support/active checks and
create/stop behavior; QR uses QRCA and is disabled with an explanation if unavailable.
Saved and hidden network configuration opens Plasma’s connection editor.
`tools/verify-network-layouts.py` checks the layout menu, live summary changes,
action availability, and footer bounds without changing real connections.

Bluetooth uses a Device spotlight layout. The first connected device keeps the
featured card until it disconnects or another device starts playing audio or
transferring files. Connection order is tracked for the applet's lifetime, even
while the popup is closed; connections already present at startup use the system
list order. Top-right device icons show only devices with current computer-related activity
and switch the featured card without changing audio routing. A manual choice lasts
until its activity ends or a new device starts activity. Other devices remain
compact glass pills. Active BlueZ audio transports and active OBEX transfers
supply activity; remote MediaPlayer status alone never counts as activity; read-only
D-Bus snapshots run once per second while the popup is open. Unreported activity
(such as phone-local playback or ordinary mouse movement) cannot trigger promotion.
Battery
values appear only when supplied by the device, including a valid zero reading.
The native power switch sits in the shared tray header; Pair and Settings stay in
a fixed bottom island. Details retains address copying, file transfer, media
controls, and KDE's Forget confirmation. Back and Escape return to the device
list. Connections use KDE's shared pending-call state for busy and failure
feedback, with duplicate requests disabled. `tools/verify-bluetooth-spotlight.py`
checks the presentation using simulated devices and renders light/dark previews;
it does not connect, disconnect, pair, or forget actual devices.
`tools/verify-bluetooth-activity.py` checks native D-Bus decoding and activity
lifecycle against isolated mock BlueZ/OBEX services.
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

The Quick Settings test checks activation of six controls, keyboard volume changes,
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
