# Installed desktop

## Qt Quick styling for menu commands (installed 2026-10-13)

Installed the rebuilt native application-style plugin with receipt-preserving
`sudo python3 tools/install-switches.py --native`. Supported applications now
activate the verified shared Quick overlay during native style initialization,
covering direct executable launches and KDE's KCMLauncher API. The Qt control
style is selected before QML loading; the Plasma shell and greeters are excluded.
Close and reopen affected applications; menu widgets and Plasma need no restart.

Direct System Monitor, Discover and stock System Settings previews passed, as
did an actual KCMLauncher invocation with user wrappers excluded. Settings was
tested in a disposable Wayland compositor. Merkuro's shared surfaces also pass;
its separate embedded calendar layout continues to use the existing wrapper.
The startup regression checks and six window-material cases pass. Previous
native plugin: `.staging/quick-launch-before-20261013/sequoiaeleven6.so`.

## Qt Quick styling refresh (installed 2026-10-13)

KDE updates changed 50 source files recorded by the installed Quick overlay,
causing its launcher to fall back to standard components. Rebuilt and installed
the Quick component with `python3 tools/install-theme.py install --components
quick-style --skip-deps`. All 255 current source fingerprints match, the installed
wrapper selects the overlay, and the Quick ownership receipt validates.
Fully close and reopen affected applications from the application menu.

Component refresh validation now preserves edits in unselected components; an
unrelated lock-screen edit had blocked this refresh. Incoming files and files
retired by the selected component still receive edit protection. Other component
receipt entries were verified unchanged. Previous overlay and receipt:
`.staging/quick-repair-before-20261013/`.

## Native View and Power menus (installed 2026-10-06)

Start menu 1.0.5 replaces both generic submenus with native Plasma dialogs.
View opens 4px below its control at the same width. Power opens 4px above its
button with their right edges aligned. Both inherit the theme's rounded mask,
shadow, transparency and compositor blur. The View button retains its fitted
hover/focus frame. Arrow keys and Enter select options; Escape returns focus
to the invoking button. Closing Start also closes its submenus, and opening
a submenu no longer dismisses the parent window on focus transfer.

The offscreen verifier passes, including actual button clicks, positions,
view selection, Escape and parent/child dismissal. The new
`python3 tools/verify-start-menu-native.py` repeats the checks in a disposable
Wayland compositor and verifies native blur-region requests. Rendered submenu
captures are `preview/start-view-menu.png` and `preview/start-power-menu.png`.
The verifier avoids PySide logging callbacks during threaded QML compilation
and Wayland dispatch, which can deadlock the Python test harness.

Updated both widget archives, verified the installed package against source,
and restarted Plasma successfully. Previous installed package:
`.staging/start-menu-popups-before-20261006-195038/org.sequoia.startmenu/`.

## Start menu View dropdown (installed 2026-10-06)

Updated the Start menu to 1.0.4. The View selector now draws its normal, hover,
pressed and focus theme frames on the same bounds, removing Plasma's 6px outer
hover/focus expansion. Its size, theme artwork, keyboard selection and wheel
selection are retained. The Start menu verifier and offscreen hover/focus/open
captures passed; keyboard selection changed the configured view successfully.
Refreshed both widget archives, installed the package and reloaded Plasma.
Previous package: `.staging/start-menu-view-before-20261006-193654/org.sequoia.startmenu/`.

## Start menu responsiveness (installed 2026-10-06)

Updated `org.sequoia.startmenu` to 1.0.3. Closing now hides the complete native
popup immediately; Plasma owns the window effect instead of a separate slide
that exposed the empty background. The home page is retained across normal
open/close cycles, and navigation resets only after hiding. Hidden app views
skip tile creation, capped collections instantiate only their displayed tiles,
and category cards load at most four preview icons.

`python3 tools/verify-start-menu.py` passes application search/launch, page
navigation, settings, placement, reduced motion, native popup dismissal,
12 close/reopen cycles retaining the home page, rapid toggles during navigation,
and a 500-row collection with bounded tile creation. These popup checks run
offscreen and do not measure compositor animation timing. The captured preview
was also inspected. Refreshed `dist/widgets/start-menu.tar.gz` and the Start menu
member of `dist/sequoia-widgets.tar.gz`; other members are unchanged.

Installed with `kpackagetool6 --type Plasma/Applet --upgrade`, verified installed
files against source, and restarted `plasma-plasmashell.service`. Plasma is
active/running with zero automatic restarts and no Start menu QML errors in its
startup journal. Backup of the previous installed package:
`.staging/start-menu-before-20261006-192918/org.sequoia.startmenu/`.

## Optional Global Theme desktop layout (2026-10-06)

All six Sequoia Eleven variants now contain a standalone desktop-layout script.
The preset captures the current 34px top menu bar and 64px floating, window-dodging
dock, their widget order, launcher pins, clock and tray settings. Personal paths
and monitor IDs are omitted; Downloads resolves the destination account's folder.
Select **Desktop layout** when applying a Global Theme to recreate the preset.
Appearance-only changes preserve the current desktop.

The Global Theme installer supplies missing Start Menu, Downloads, Separator,
Kpple Menu and Window Title Fork packages, preserves separately installed copies,
and tracks its own widget files for removal. Native widget forks remain separate.
Start Menu source is restored under `plasma/plasmoids/` from the existing archive;
its build/export tools now use that source location.

Validation: all six themes applied successfully through Plasma's actual
`loadLookAndFeelDefaultLayout` interface in an isolated two-monitor Wayland session.
Checks cover panel geometry, primary-screen placement, floating/hiding behavior,
widget order, pins, Downloads, tray visibility and clock settings, plus repeated
theme switching without duplicate panels. Installer ownership/refresh/removal
checks, the theme installer suite and GUI suite pass (optional full native-build
integration tests remain opt-in). See `tools/verify-desktop-layout.py --native`.

Installed the updated Global Theme component for the current account and verified
all six layout scripts match the source and have installation receipts. The live
layout was not applied or reset. Existing separately installed widgets and the
locally modified lock-screen wallpaper fader were preserved.

## Custom login theme prepared (2026-09-27)

The design is implemented in `login-screen/` and included in the unified installer
and uninstaller. Isolated native-greeter previews, interaction/fallback tests, and
installation/removal tests pass. This change has **not** been installed to the live
login manager. Run `./install.sh` as the desktop user, then reboot to load the
new presentation. Native authentication remains in Plasma Login Manager.


## Task-manager motion and widget collection (installed 2026-09-23)

Installed floating Appearance/Behavior cards and an Animations page with live
preview, enable switch, hover zoom/lift styles, intensity/speed, launch bounce,
press feedback, attention pulse and native reorder controls. Configuration uses
Plasma's real schema, Apply and per-widget storage through a private task-manager
resource namespace. The normal text task manager retains its native configuration.

The four custom widgets and their rebuild/install/restore scripts are stored in
`~/sandbox/widgets`. `tools/export-widgets.py` refreshes the collection and saves
older managed exports. `tools/install-widgets.py --install --restart` builds all
patches first, backs up existing overrides and installs them per user. It leaves
panel layout and pinned launchers intact. Portability requirements and limitations
are documented in `plasma/WIDGETS.md` and the exported README.

Native hover, configuration loading/Apply and all three settings pages pass.
Motion fixtures pass zoom/lift, inward direction, press, startup, speed and disabled
motion checks. Native Control Center visibility and detail navigation pass.
The bundle independently rebuilt and passed fresh install, upgrade backup and
restore checks with disposable XDG directories. All exported hashes match, and
the task-manager snapshot matches the installed copy.

Plasma restarted successfully: active/running, zero automatic restarts, and all
four widget instances still present. Backup:
`~/.local/share/sequoia-eleven/widget-backups/20260923-202050-053724/`.
Restore using `python3 tools/install-widgets.py --restore BACKUP --restart`.

## Notification polish (installed 2026-09-23)

Live banners and notification history now share clearer title/body typography,
24px header icons, subdued app labels/timestamps, inset circular header controls,
and rounded tinted action buttons. Banner content has consistent padding, the
stack uses a 10px gap and 24px edge clearance, and critical alerts retain their
colored indicator. History cards have additional vertical breathing room.

The overlay routes notification views to a local copy of KDE's extracted Globals
singleton so its popup factory loads the styled components. Native notification
models, timers, actions, replies, jobs and DND logic are retained. Also corrected
the shared title component's unqualified job-state reference. Compatibility
continues to be guarded by the notification plugin fingerprint.

Verified real notification delivery in a private D-Bus session: one styled
banner, action/reply dismissal, close, critical alerts, long-text bounds,
fixture progress layout, automatic expiry and DND/history. Light, Dark and
larger-text RTL previews pass; Control Center's full detail navigation passes.
Captures are `preview/notifications-*.png`. Native extracted KDE code still
reports a pre-existing jobError QString/int conversion warning on plain messages;
tested notification flows work. Installed through `tools/install-control-center.py`
and reloaded Plasma.
Previous overlay: `.staging/before-notification-polish-20260923-175024/`.

## Tahoe Trash artwork (installed 2026-09-23)

The stock Trash applet forces `-symbolic` icons in panels. Its version-checked
QML overlay now requests `user-trash` or `user-trash-full`, allowing the active
Sequoia Tahoe light/dark icon theme to supply the existing shaded bin artwork.
Native opening, dragging, emptying, and contents tracking are retained.
The Trash plugin is included in the overlay's compatibility fingerprints.

Verified the native panel representation in isolated light and dark previews,
and resolved both empty/full icons from each theme. Captures:
`preview/trash-light.png` and `preview/trash-dark.png`. Installed and restarted
Plasma, with the prior overlay at `.staging/before-trash-artwork-20260923-173333/`.

## Downloads dock icon sizing (installed 2026-09-23)

Removed the Plasma ToolButton's individual edge padding, which was still
shrinking the folder despite `padding: 0`. The icon now uses up to 48px within
the existing dock button. Native rendering and real button/fan interaction
checks pass. Installed and reloaded Plasma; backup:
`.staging/before-downloads-fan-20260923-172826/`.

## Clearer Downloads labels (installed 2026-09-23)

Filename labels now remain horizontal while icons retain the fan's tilt. Text
uses the regular UI font, native rendering and full opacity over a slightly
stronger backing. Final row positions and label dimensions align to whole
pixels. The shared folding scale and curved placement remain in use.

Reviewed fixture and native renders; existing file, keyboard, limit, blur and
bidirectional animation checks pass. Installed with backup
`.staging/before-downloads-fan-20260923-172521/` and restarted Plasma.

## Downloads fan motion and blur removal (installed 2026-09-23)

The application style previously enabled blur on every transparent Quick window,
including the otherwise clear fan. Added a `sequoiaNoBlur` window property that
opts this popup out. Opening now staggers the rows from a shared origin at the
folder; closing reverses their travel and shrinks them into that origin. The
popup stays alive until the fold completes, including Escape, folder-button
toggle, focus loss and file/folder activation. File launches wait for the fold.

The system style requires administrator authentication, so the installer deploys
the built plugin under `~/.local/share/sequoia-eleven/downloads-style/styles/`
and adds `60-sequoia-downloads-style.conf` to the Plasma user service. The plugin
is replaced atomically. Remove that drop-in and reload/restart the user service
to return to the system style. Backup: `.staging/before-downloads-fan-20260923-172236/`.

Fixture checks verify intermediate motion in both directions and final positions.
The native test intercepts compositor blur requests, verifies blur is disabled,
and confirms Escape leaves the popup visible during folding before hiding it.
The visible folder icon, actual button click and existing file/navigation checks
also pass. Plasma was restarted to load the installed widget and style.

## Restore Downloads folder button (installed 2026-09-23)

The standalone fan had only a compact representation, leaving its panel item
blank and without a usable click target. The full representation now contains
the dock button with the existing Sequoia Downloads folder SVG and fixed layout
bounds. The SVG is packaged with the widget so small icon-theme fallbacks cannot
replace the folder with a monochrome download symbol. The fan anchors to that displayed button. Its focus scope and activation
handler now give keyboard focus to the file rows when the popup becomes active.

The native regression check now verifies rendered blue folder pixels and sends
an actual mouse click to the visible button. It no longer invokes the popup
function directly. Folder rendering, button opening, visible file rows,
transparent backdrop and Escape dismissal all pass together. Installed and
restarted Plasma successfully.

## Compact reference-style Downloads fan (installed 2026-09-23)

Reworked the fan to follow the supplied reference: 40px theme icons on the
right, short individually backed labels on the left, 44px row pitch, a curved
and progressively tilted stack, and “See more in File Manager” at its top.
The stack is capped at ten files/folders (fewer on short displays). A native
transparent Plasma Dialog removes the former large rectangular popup frame.
It anchors to Downloads and closes on focus loss or Escape. Native file opening,
live folder updates and the current icon theme remain in use.

Verified ten-item cap, ordering, encoded URLs, keyboard controls, top-action
placement, live updates, empty state and short-screen sizing. The native test
checks an exposed window, visible file rows, transparent corner pixels and
Escape dismissal. Its temporary package suppresses external file launches;
focus-loss dismissal is disabled only in the capture test so desktop activity
cannot interrupt it. Installed and restarted Plasma: active/running, zero
automatic restarts. Previous version and configuration:
`.staging/before-downloads-fan-20260923-171149/`.

## Downloads fan (installed 2026-09-23)

Replaced the Downloads quick-launch widget with `org.sequoia.downloads` in the
same dock position before Trash. The popup shows up to eight recent files,
newest nearest the dock, with curved placement and rotated file-type icons.
Click/Enter/Space opens the selected entry; Up/Down navigates, Escape dismisses,
and Open in File Manager opens the complete Downloads folder. Folder changes
update the view automatically; hidden files are omitted and short displays
show fewer entries. The native Plasma popup owns focus and dismissal.

Temporary-file checks pass for ordering, URL encoding, keyboard activation,
live updates, empty state and short-screen sizing. The native Wayland popup
passes model/delegate, visible-row and exposed-window checks and is captured in
`preview/downloads-native.png`. No real downloads were opened or modified by
the tests. Installed the final QML and restarted Plasma successfully.

Original shortcut backup:
`.staging/before-downloads-fan-20260923-165858/plasma-org.kde.plasma.desktop-appletsrc`.
The later `170155` backup holds the first installed version. Stop Plasma before
restoring its configuration; the unused Downloads Stack package can remain.

## Dock and menu-bar refinement (installed 2026-09-23)

Installed the task-manager overlay with 10% icon hover zoom and 4px running
indicators. The native dock is now 68px high with normal icon spacing, preserving
all pinned launchers. Downloads and native Trash follow a subtle separator.
The 32px menu bar uses the bold app-title widget immediately before the global
menu, a single expanding spacer and a regular Adwaita Sans 10pt clock. Secondary
tray controls now appear in Control Center. Color Picker and Recent Files remain
as standalone widgets. No desktop fallback menu or background app was added.

Native dock hover/reset and Control Center visibility/navigation checks pass.
Saved and live widget order match, original launcher URLs are unchanged, and
Plasma is active/running with zero automatic restarts since the final restart
at 16:46:46. Capture: `preview/mac-panels-installed.png`.

One installation-time crash occurred at 16:46:21 while changing configuration,
after the helper library had been overwritten in place. The stack enters
KConfig synchronization from the scripting API; causation is not established.
The shell was restarted and subsequent configuration and layout checks passed.
Use the existing installer (directory replacement) for future library updates.
The offscreen native tray test also failed; its supported Wayland test passed.

Backups: `.staging/before-dock-overlay-20260923-164621/control-center` and
`.staging/before-mac-panels-20260923-164621/` (both panel configuration files).
To restore, stop `plasma-plasmashell.service`, restore those two files into
`~/.config`, replace the Control Center directory with its backup, then start
the service. Replace the directory rather than overwriting a loaded `.so`.

## Reference-style Control Center icon (installed 2026-09-22)

Matched the supplied two-switch glyph: an outlined upper capsule with a left
knob and a solid lower capsule with a transparent right knob. Vector paths
scale uniformly, inherit the icon color, and let the panel tint show through.
Checked light/dark rendering at small panel sizes and native tray/navigation
loading. Installed and reloaded Plasma. Previous overlay: `.staging/before-reference-icon-20260922-175639/control-center`.

## Polished Contacts menu (installed 2026-09-22)

Merkuro Contacts now has an inset search field, a rounded list surface, softer
avatars, alphabetical groups, subtle hover feedback and a contact count.
Profiles use separate padded cards for phone numbers, email, addresses and
birthdays; long values wrap. Call, SMS, email and barcode actions retain their
native data. QR, Data Matrix and Aztec sharing have a dedicated card. Short fades
follow the desktop animation duration, and the shared compact header stays fixed.
Back walks from sharing to profile to the list while preserving its search.

Light, dark/large/RTL and hardware-rendered dark previews pass filtering,
keyboard activation, profile field wrapping, sharing and Back checks. All twelve
native menu checks pass. The native profile navigation test supplies sample data
directly; an invalid synthetic item ID was found to crash Merkuro's fetch job.
No address-book data or external communication actions are changed by the tests.
Installed files and fingerprints match the staged overlay; Plasma was reloaded.

Previous overlay: `.staging/before-contacts-cards-20260922-175128/control-center`.

## Consistent compact menu headers (installed 2026-09-22)

The shared header row now caps its height at its natural content height instead
of stretching with taller pages such as Weather Report. Normal-size headers
remain approximately 26px high, while larger fonts can still increase their
natural height. Centered text and aligned card edges are preserved. All eleven
native menu and Back-navigation checks pass; the weather capture confirms the
compact header. Installed and reloaded Plasma.

Previous overlay: `.staging/before-fixed-header-20260922-174050/control-center`.

## Weather floating cards (installed 2026-09-22)

Replaced the crowded weather popup with an inset current-conditions card,
separate Day and Night forecast cards, pill tabs, a grid of detail cards and
individual notice cards. Content has 12–24px interior spacing and long notices
scroll inside the popup. The weather page gets its own 520px-wide, 640px-high
preferred minimum area, capped to the display; Back restores the dashboard.
Daily-only providers retain both high and low temperatures on one forecast card.
Current conditions, feels-like readings, wind, units, notices and source credit
remain connected to KDE's native forecast service and existing location settings.
Zero readings are retained and unavailable numeric values are omitted.

Light, dark/large/RTL, daily-only, long-notice and loading/unavailable previews
pass. Card bounds checks verify interior spacing; native tray integration and
Back navigation pass. Installed files match the staged overlay, and Plasma
restarted without Control Center QML errors. Weather source files live under
`plasma/control-center/weather`; its plugin joins the version fingerprint list.
Sample captures are `preview/weather-light-{forecast,details,notices}.png`.

Previous overlay: `.staging/before-weather-cards-20260922-173530/control-center`.

## Hidden menu scrollbars (installed 2026-09-22)

The active menu container hides native scrollbar visuals, including dynamically
created controls and third-party applet views such as Contacts. Scrollbar
visibility/geometry and scrolling behavior remain intact so native keyboard
focus scrolling continues to work. Bindings restore when the menu is detached.
All ten native detail/navigation checks pass and the audio preview confirms
hidden scrollbars. Installed and reloaded Plasma. Previous overlay:
`.staging/before-hidden-scrollbars-20260922-172023/control-center`.

## Shorter Control Center headers (installed 2026-09-22)

Reduced header buttons to a 24px minimum height, vertical padding to 2px and
outer vertical margins to 1px. Normal-size headers are now approximately 26px
high, with centered titles and aligned widths retained. Shared action icons
use 18px sizing. Native preview and navigation checks pass; installed and
reloaded Plasma. Previous overlay:
`.staging/before-shorter-header-20260922-171808/control-center`.

## Centered Control Center titles (installed 2026-09-22)

Header titles fill the row height and explicitly center text vertically, with
zero vertical text padding. Removed inherited heading background vertical
insets so the surface and content share the same center. Native dashboard and
navigation checks pass; installed and reloaded Plasma. Previous overlay:
`.staging/before-header-centering-20260922-171625/control-center`.

## Aligned Control Center header widths (installed 2026-09-22)

The shared header now takes its left/right insets from the active menu body
(or dashboard), replacing Plasma’s outward heading insets. Header controls
follow those same edges. All ten native menu checks pass; visually confirmed
matching panel edges, installed and reloaded Plasma. Previous overlay:
`.staging/before-header-width-20260922-171433/control-center`.

## Compact Control Center headers (installed 2026-09-22)

Reduced shared header vertical padding from 6px to 2px per side and button
padding from 8px to 4px, with 18px icons and a smaller corner radius. Reduced
the gap below the header from 12px to 8px. All ten native menu/navigation checks
pass. Installed and reloaded Plasma. Previous overlay:
`.staging/before-compact-header-20260922-171230/control-center`.

## Control Center detail menus (installed 2026-09-22)

Installed matching rounded headers, inset toolbars, soft content panels, card
rows and pill tabs for audio, brightness/color, battery/power, Bluetooth,
networks, notifications, clipboard, removable devices, printers and camera
activity. Other hosted applets share the new navigation header and fallback
surface. KDE's native models, actions and nested navigation remain in use.
Clipboard history views are copied locally; application-owned tray context
menus and notification toasts retain their native layouts.

Native menu opening, surface palette, sizing and Back checks pass in light mode
and dark mode with 13pt text and RTL layout. Clipboard barcode Back, dashboard
visibility settings and the existing media popup regression checks also pass.
The installed overlay matches the staged build and Plasma has been reloaded.
A brightness Repeater guard prevents an upstream null-neighbor error while
multiple display sliders are being constructed.

Source fingerprints now cover every extracted applet and copied Klipper view.
The pre-change overlay and service override are backed up in:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-detail-menus-20260922-170557
```

## Merkuro Calendar (installed 2026-09-21)

Added Merkuro 26.08.1 (Fedora RPM) to the Quick overlay. Its sidebar now uses one
continuous opaque floating surface with 12px content insets. Month, hourly/week,
schedule and task pages use inset rounded surfaces and clipped content; menu and
toolbar positions align. The surrounding canvas retains the selected material's
alpha. Both current and legacy calendar desktop entries use the wrapper.

The installer extracts QML from the installed executable at early startup,
before calendar services initialize, and builds a launcher-scoped URL interceptor.
The overlay fingerprints the RPM executable, falling back after updates until
refreshed. No RPM files are edited. Reopen Merkuro from the application menu.

Previous overlay and existing Merkuro launchers are backed up in
`.staging/before-merkuro-islands-20260921-130425`.

## Sequoia Tahoe icons (installed 2026-09-21)

Bundled the installed MacTahoe-blue family as SequoiaTahoe, SequoiaTahoe-light,
and SequoiaTahoe-dark. Small neutral action/status/device glyphs have thicker
vector contours with rounded caps and joins; the close badge has a wider X
cutout. Full-color application artwork and original theme installations remain.
All six global themes now select the corresponding light/dark icon variant.
The artwork installer installs the complete family with local relative links;
the desktop activation tool also selects and refreshes it.

Activated SequoiaTahoe-dark to match the current Dark Transparent color scheme.
Refreshed KDE's icon loaders and Plasma's cached tray icons. Live lookup resolves
to the new theme; the tray capture is `preview/tahoe-icons-tray.png`.
Previous settings and global-theme defaults are backed up in
`.staging/before-tahoe-icons-20260921-124112`.

## Rounded panel blur containment (installed 2026-09-21)

Inset the panel blur mask by one logical pixel beneath the opaque border,
including its rounded corners. The panel interior continues to request blur.
Installed all eight light/dark panel SVG variants and restarted Plasma.
The generator preserves this correction on rebuild.

Previous panel assets and layout configuration are backed up in
`.staging/before-panel-blur-mask-20260921-122257`.

## Restore sidebar backdrop translucency (installed 2026-09-21)

Restored the intermediate sidebar backing that the user confirmed matched the
content: `Platform.Theme.backgroundColor`, including its alpha. This supersedes
the opaque base-color fill below. The floating sidebar remains solid, while the
backing around it permits the window blur to show through. Most-used pages,
preview activity-data isolation, and viewport clipping remain intact.

Settings Light Transparent and Info Center Dark Transparent pass rendering,
scene-color/alpha, and clipping checks; launcher lifecycle checks pass.
The previous overlay is in
`.staging/before-sidebar-blur-restore-20260921-121458/quick-islands`.
Reopen affected applications from the menu to load the restored background.

## Most-used pages and sidebar base color (installed 2026-09-21)

Restored the existing Default activity as current through ActivityManager.
Its current activity had been empty, while its Settings usage history was still
present. A live-session capture confirms six Most Used pages again: Colors,
Global Theme, Application Style, Plasma Style, Icons, and Window Decorations.
The selection is persisted in `~/.local/state/kactivitymanagerdstaterc`.

Quick previews now isolate XDG data and state as well as configuration/cache.
Previously their private activity services could access live usage data/state;
they can no longer overwrite the current activity or contend for its database.

Installed a solid base-color fill behind the floating sidebar in Settings and
Info Center. Light mode outer margins are exactly RGBA (242,243,245,255), or
`#F2F3F5`; dark mode uses (37,38,41,255). Floating surfaces and content material
remain unchanged. Reopen affected apps from the application menu.

Previous overlay:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-sidebar-base-color-20260921-121051/quick-islands
```

## Sidebar overflow clipping (installed 2026-09-21)

Inset ScrollViews now clip their app-supplied ListView/Flickable contents at the
padded viewport. Previously padding positioned the list, but partially visible
rows could still paint below the rounded panel. The binding follows replacement
content items and restores their original clip setting outside inset surfaces.

Verified scrolled Settings in Dark Transparent and Light Transparent (150%, RTL),
Info Center Light Solid, and Discover Dark Transparent in a shorter 650px window.
Each sidebar viewport clips inside its padding. Existing material, card, and
launcher checks pass. Installed the refreshed overlay; reopen apps from the menu.

Previous overlay:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-sidebar-clipping-20260921-120217/quick-islands
```

## Discover-style Settings sidebar (installed 2026-09-21)

The supplied Discover screenshot is the appearance reference. Settings and Info
Center now paint one opaque, rounded island behind the entire sidebar, including
the search/back header and category pages. This supersedes the transparent
sidebar change below. The shared SideBarItem host owns the surface, so switching
categories does not split or duplicate it. It uses the same Island component as
Discover, with a 5px inset, 14px radius, gradient, and shadow. The content canvas
retains the selected transparent/solid material.

Wayland previews pass for the Keyboard category, dark Transparent at 150% with
RTL layout, and Info Center Light Solid. Validation checks for exactly one
visible full-height sidebar island and opaque pixels across the header/body
join. Transparent content stays at alpha 184/255; sidebar and toolbar are 255.
Launcher installation/refresh/fallback/restoration checks pass.
Discover's card opacity and 46 sidebar highlight containment checks also pass.
Installed component files match the validated overlay. Fully quit and reopen
Settings and Info Center from the application menu to load the change.

Previous overlay:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-discover-sidebar-20260921-115639/quick-islands
```

## Settings sidebar shares the content canvas (installed 2026-09-21)

Corrected the earlier sidebar change, which matched the opaque toolbar instead
of the main page. System Settings and Info Center navigation now expose the
same window canvas as the content, without a separate opaque gradient or shadow.
The same rule covers hosts of CategoriesPage/SubCategoryPage. Navigation spacing
and highlights are preserved; explicitly framed content retains its island.

Installed the refreshed Quick overlay after real Wayland captures confirmed
matching sidebar/content RGBA in light and dark transparent and solid variants.
Discover's existing surface and highlight checks and the launcher checks pass.
Fully quit and reopen affected apps from their application-menu launchers.

Previous overlay:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-sidebar-canvas-20260921-115248/quick-islands
```

## Desktop context menu entries (applied 2026-09-21)

The desktop used the standard context-menu plugin with Add Widgets, Add Panel,
and KRunner explicitly enabled. Applied the reference image's compact action
selection using `python3 tools/apply-desktop-menu.py --restart-shell`. The tool
changes only desktop right-click options, writes through Plasma's configuration
API, and verifies persistence before reloading the shell. Appearance still
follows the selected application theme. Both desktops and both panels were
present after reload, and the running shell reported the corrected options.

The original configuration is backed up at:

```
/home/jeffreygustafson/.local/share/sequoia-eleven/backups/desktop-menu-20260921-095050-467363/plasma-org.kde.plasma.desktop-appletsrc
```

## Shared settings sidebars (installed 2026-09-21)

Refreshed the user-local Quick overlay and added Info Center's application-menu
launcher. Its navigation now uses the same rounded, inset surface as System
Settings and the other themed controls. The sidebar rule recognizes the shared
navigation component instead of requiring the System Settings application name.

Verified real Wayland captures of Info Center and System Settings in Dark
Transparent and Light Solid. Canvas alpha is 184/255 for transparent and 255/255
for solid; sidebar and toolbar surfaces remain opaque. Launcher installation,
refresh, fallback, and restoration checks pass, including both settings apps.
Reopen these applications from the application menu to load the change.

Previous overlay and launchers:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-sidebar-match-20260921-093944
```

## Canvas tint matches titlebar (installed 2026-09-20)

Refreshed the user-local Quick overlay so Discover and System Monitor paint the
glass tint as a scene background. This corrects the overly bright canvas from
the translucent native window clear color on this Qt/Wayland setup. The canvas
and titlebar share the same palette color and 72% tint; blur remains enabled
and floating cards remain opaque. Embedded System Settings retains its native
widget background tint.

Reopen affected applications from the application menu. No logout is required.
The previous overlay is backed up at:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-unified-tint-20260920-183120/quick-islands
```

## System Settings and dialog fixes (installed 2026-09-20)

Installed the rebuilt Qt application style and refreshed the user-local Quick
overlay. System Settings' main forms and unframed KCM pages now expose the 72%
glass canvas; navigation panels, headers, cards, and controls remain solid.
Embedded Quick scenes no longer double-paint the window tint. Dialogs, tool
windows, and popups receive matching background painting and blur registration;
Quick blur requests no longer depend on drawing a native control.

The installed plugin SHA-256 matches the staged build. Reopen affected apps
from the application menu to load both changes. No logout is required.

Previous plugin and Quick overlay backup:

```
/home/jeffreygustafson/sandbox/win-global/.staging/before-settings-blur-20260920-181932
```

The backup's `paths.json` records the original installation locations.

## Solid islands over glass (workspace update, 2026-09-20)

The workspace now separates surface opacity from canvas opacity. Transparent
presets keep the 72% window/titlebar tint, with opaque toolbars, sidebars, framed
content, and Quick cards. View/window RGB values still match and frame contrast
remains zero. This supersedes the equal-alpha behavior recorded below.

The Qt plugin is built and staged under `.staging/desktop`. The latest update is
installed as recorded above. To reinstall from the prepared build:

```sh
sudo cmake --install build/desktop
./install.sh
python3 tools/install-quick-islands.py
```

Select a Light or Dark Transparent Global Theme and fully reopen applications.

## Unified backgrounds and outlines (2026-09-20)

All Sequoia Eleven schemes now set **Frames and Outlines** to **0%**. Normal and
alternate view backgrounds match the window backgrounds exactly, including
transparency. The application style, Quick surfaces, and decoration outlines
honor zero frame contrast. Shadows and focus/selection feedback remain available.
The current Sequoia scheme receives the updated values without selecting a
different appearance. Reopen applications to load the rebuilt style; loaded
window-decoration code refreshes after the next login.

## Solid and Transparent variants (2026-09-20)

Installed four additional Global Themes and color schemes: **Sequoia Eleven
Light Solid**, **Light Transparent**, **Dark Solid**, and **Dark Transparent**.
The matching native decorations are **Sequoia Eleven Solid**
(`org.kde.sequoiaeleven.solid`) and **Sequoia Eleven Transparent**
(`org.kde.sequoiaeleven.transparent`). The original entries remain installed.

Select a coordinated set in System Settings → Colors & Themes → Global Theme,
then reopen applications. Transparent uses frosted background surfaces and
titlebars while retaining opaque text and controls; the updated Qt Quick overlay
also supports transparent page/sidebar/toolbar backgrounds. Current theme
selection was preserved during installation.

Native widget and decoration previews passed opacity checks for both palettes
and materials. Discover passed sidebar containment and surface-opacity checks,
including dark RTL at 150% scaling. Transparent Qt widgets also passed a native
Wayland capture at 150% scaling. Existing interaction and installer checks pass.

## Login appearance (2026-09-20)

Plasma Login Manager now uses Sequoia Eleven's system-wide Plasma styles,
colors, Adwaita Sans font, and adaptive wallpaper. Initial synchronization
selected **Sequoia Eleven Light**, matching the current desktop. The enabled
`sequoia-eleven-login-sync.path` watches Jeffrey's desktop settings and updates
the greeter account when switching between Sequoia Eleven Light and Dark.
The initial system service completed with status `0/SUCCESS`.

Changes appear on the next login screen. The display manager was not restarted.
The native login layout and authentication remain provided by Plasma Login
Manager. The login appearance is shared by all users.

Restore the settings and files from before this installation with:

```sh
sudo python3 tools/install-login.py --restore /var/backups/sequoia-eleven-login/20260920-082042-917532
```

The backup also records prior watcher enablement. Restoring removes the new
watcher and returns existing files to their previous contents and ownership.

## Desktop installation history

Sequoia Eleven Dark is active as of 2026-09-19. Sequoia Eleven Light is also
available in System Settings → Global Theme.

The compiled Qt 6 style is `SequoiaEleven`; KWin reports the active decoration as
`org.kde.sequoiaeleven`. Both were built from the separate source copy in
`application-style/SequoiaEleven`. The original `~/Darkly` checkout is preserved.

Restart existing applications to pick up the style. Lock-screen changes apply
on the next lock, and the matching splash applies at login.

The pre-activation settings and local artwork snapshot is:

```
/home/jeffreygustafson/.local/share/sequoia-eleven/backups/20260919-191206-842583
```

To restore it from this project directory:

```sh
python3 tools/restore-desktop.py /home/jeffreygustafson/.local/share/sequoia-eleven/backups/20260919-191206-842583
```

After restoring, log out and back in. The Sequoia Eleven plugins remain installed
but are no longer selected. See README.md for build, reinstall, and coverage notes.

## Typography and avatar update

Nunito upright/italic variable fonts are installed for this user and selected
for the desktop, menus, toolbars, and window titles. The lock-screen avatar is
6 grid units across (25% larger than the previous 4.8-grid-unit version).
The compact password field has a smooth, translucent white border. The clock and splash also use Nunito.
Terminal/editor fixed-width fonts are unchanged.

To undo only this update and return to the first Sequoia Eleven deployment:

```sh
python3 tools/restore-desktop.py /home/jeffreygustafson/.local/share/sequoia-eleven/backups/20260919-192214-438276
```

The lock-screen media card is now 392×156px and automatically follows the active
light/dark palette, including its text and playback controls. Changes appear on
the next lock.

Desktop widgets with standard backgrounds now opt into Plasma wallpaper blur
using rounded SVG masks. Both theme variants include the required assets.

## Qt Quick application islands

Floating Quick/Kirigami surfaces are installed for System Monitor, System
Settings, and Discover. Close and reopen these apps from the application menu.
The user-local desktop entries invoke
`~/.local/share/sequoia-eleven/quick-islands/sequoia-quick`; direct terminal launches
of the original executable do not use this extension unless run through that wrapper.

Refresh after KDE upgrades with `python3 tools/install-quick-islands.py`. When its
source components change, the wrapper uses standard components until refreshed.
Remove this extension and restore the previous desktop entries with
`python3 tools/install-quick-islands.py --remove`. This is separate from the older
full-desktop backup/restore procedure above.

## Smoother UI typography

Adwaita Sans is selected for KDE and GTK UI text at 10pt, with 9pt small labels
and 10pt medium window titles. Rendering uses grayscale antialiasing and slight
hinting. Panel/menu spacing, DPI, and fixed-width font preferences are preserved.
Nunito remains the custom clock/splash display font. Reopen applications to refresh
font rendering; no application-style rebuild is needed.

Restore the settings from before this font update with:

```sh
python3 tools/restore-desktop.py /home/jeffreygustafson/.local/share/sequoia-eleven/backups/fonts-20260919-221106-244888
```

## Desktop repair (2026-09-23)

See [desktop repair findings and validation](DESKTOP-REPAIR.md) for Tahoe app
aliases, Activity restoration, the Contacts login hang fix and preview isolation.

## Control Center icon animation (2026-09-23)

Added hover and popup-state slider motion, using Kirigami animation durations.
Installed the icon and expander binding after verifying intermediate animation
values, endpoints and Wayland renders. The native tray/dashboard/navigation
check passes. Plasma was restarted to load the update.

## Search focus border (2026-09-23)

The native Plasma TextField expands its hover/focus overlay by the SVG frame
margins. Added prefix-specific `hint-no-border-padding` to the line-edit hover,
focus and keyboard-focus frames in both theme variants. The base keeps 6px text
padding. This also corrects other native Plasma text fields using these assets.
The generator preserves the change. Native SearchField renders verify exact
overlay/base alignment in Light and Dark (`preview/search-highlight-*.png`).
Installed the two SVGs with backups under `.staging/before-search-highlight-*`
and restarted Plasma.

KSvg's [frame implementation](https://github.com/KDE/ksvg/blob/master/src/ksvg/framesvg.cpp)
supports this hint per prefix; zero-size margin rectangles are invalid and fall
back to the artwork's border size.
