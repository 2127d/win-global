# System Settings navigation

This application's sources and launcher live in `apps/settings/`. Run the
commands below from the repository root; shared build and installation tools
remain in `tools/`.

This is a presentation fork of KDE System Settings 6.8.2. It replaces the long
navigation tree with eight category cards, a stable sidebar, focused category
overviews, global search, and an alphabetical **All Settings** view. Existing
KDE settings modules, their controls, authorization, Apply/Reset/Defaults/Help,
and unsaved-change dialogs use the original KDE code.

The catalog is discovered on startup. Every authorized, non-hidden module for
the current platform has a destination, including distribution/external modules
whose category is unknown. KDE's platform and authorization restrictions still
apply. The original Quick Settings page is pinned beneath Home in the sidebar,
including its light/dark theme controls. It also remains in System & Accounts.
New modules appear after reopening the application.

Search matches the installed modules' translated names, descriptions, keywords,
and stable identifiers. Ctrl+F focuses search; Alt+Left returns to the overview.
All Settings and search include inapplicable hardware pages; category views also
include them by default, with KDE's existing Show Inapplicable Pages menu action
available. A module's controls have not been rewritten.

The sidebar uses spacing instead of divider lines. The native QWidget page
header also omits its bottom divider. Every hosted settings page, including
Quick Settings, has a single inset, rounded surface with a soft shadow around the
header, content, and action buttons. Page, form, and footer separators are
hidden; the native controls and preview contents are retained. The presentation
is shared by all categories, including pages opened from search or a direct
link, dynamically opened subpages, and the in-app host for external modules.

## Build and install

Build dependencies are the Qt 6 and KDE Frameworks development packages listed
in `systemsettings-v6.8.2/CMakeLists.txt`, plus CMake, Ninja, a C++20 compiler, ECM,
gettext, and Plasma Activities. On the Fedora 44 development machine the extra
packages beyond the theme's existing dependencies were:

```
kf6-kcrash-devel kf6-kitemviews-devel kf6-kitemmodels-devel kf6-kio-devel
kf6-kxmlgui-devel kf6-kdbusaddons-devel kf6-kjobwidgets-devel plasma-activities-devel
```

From the repository root:

```sh
python3 tools/build-settings-navigation.py
python3 tools/verify-settings-navigation.py
python3 tools/install-settings-navigation.py --binary .staging/settings-build/bin/systemsettings
```

Each build refreshes CMake's dependency cache so paths from a previous temporary
SDK cannot leak into a normal system build. Pass `--sdk` again when using an SDK.

The installer places the executable under
`$XDG_DATA_HOME/sequoia-eleven/settings-navigation/` (normally
`~/.local/share/sequoia-eleven/settings-navigation/`) and a launcher at
`~/.local/bin/systemsettings`. A separate **System Settings (Sequoia Eleven)**
application-menu entry launches it by absolute path, even if this bin directory
is absent from PATH. For the ordinary `systemsettings` command, this bin directory
must precede `/usr/bin` in the desktop session's PATH. The theme's application launcher invokes
`systemsettings` through PATH, so its desktop entry and ownership receipts do not
need modification. The separate menu entry belongs to the makeover's own receipt.
The theme's existing Quick overlay is reused when available.
Close System Settings before reopening it: KDE's single-instance service would
otherwise reactivate the already running version.

This is a separate, opt-in installation from `install.sh`. It does not modify
`/usr/bin/systemsettings`, the installed KCMs, or personal settings. Its receipt
backs up a pre-existing user launcher and restores it on removal:

```sh
python3 tools/install-settings-navigation.py --remove
```

Alternate destinations use `--prefix DATA_DIRECTORY --bin-prefix BIN_DIRECTORY`
for both installation and removal. The runtime launcher fingerprints the stock
KDE executable. After a KDE System Settings package update it opens the stock
application until this fork has been updated, rebuilt, and reinstalled. Direct
invocations of `/usr/bin/systemsettings` also remain available; close the running
instance first.

## Verification

`tools/verify-settings-navigation.py` starts a private virtual Wayland compositor
and private D-Bus session with isolated configuration, data, cache, state, and
runtime directories. It never applies settings to the live account. The test
checks the catalog against the stock app's `--list` output on the same platform,
category/search coverage, native QML and QWidget modules, cancellation of
unsaved changes, real card clicks, search clearing, compact layout, and reversible
installation. Optional variants:

```sh
python3 tools/verify-settings-navigation.py --dark --third-party --startup kcm_colors
python3 tools/verify-settings-navigation.py --rtl --scale 1.5
python3 tools/verify-settings-navigation.py --capture-group appearance --skip-install-check
python3 tools/verify-settings-navigation.py --capture-group all --skip-install-check
```

The third-party fixture supplies an external module in an unregistered category
to verify fallback discovery and activation. Screenshots and inventories are
written to `preview/settings-navigation-*`. Qt's offscreen backend is not used
for the GUI itself: the installed touchpad module expects a Wayland/X11 backend.
Verification opens representative pages; it does not exercise every hardware
operation or privileged change on every module.
The category capture option opens every module in the chosen category; for
Appearance it also captures Fonts in a compact window and opens Window
Decorations' titlebar-button subpage without changing any values.

## Source and maintenance

`systemsettings-v6.8.2/` was obtained from:
https://invent.kde.org/plasma/systemsettings/-/archive/v6.8.2/systemsettings-v6.8.2.tar.gz

Upstream copyright headers, translations, and LICENSES are retained. The new
navigation code uses GPL-2.0-or-later. Changes are confined to:

- `app/SettingsNavigation.{h,cpp}`: discovered catalog, groups, and search.
- `app/qml/Main.qml` and `IntroPage.qml`: sidebar and overview presentation.
- `app/SidebarMode.{h,cpp}`: connect the overview to KDE's existing ModuleView.
- `app/SettingsBase.cpp`: retain otherwise uncategorized modules.
- `app/ModuleView.{h,cpp}`: omit the painted header divider and give all modules
  a rounded surface with transparent page layers and
  hidden separators.
- CMake files: include the navigation source and omit the unused KRunner build.

Module loading and change-handling logic, `MenuItem.cpp`, module metadata
discovery/authorization, and all installed settings modules are unchanged. The builder checks the installed KDE
version. For upgrades, review the upstream changes to module hosting and
navigation, port this patch, and rerun coverage and unsaved-change checks before
re-enabling the fork.

## Appearance Hub preview (separate, reversible test)

The preview adds a coordinated Appearance overview and glass category overviews,
while continuing to discover and open the original KDE modules. It does not
replace the existing Settings launcher. Quick Switcher is intentionally deferred.

Build and install:

```sh
python3 tools/build-settings-navigation.py --build-dir .staging/settings-hub-build
python3 tools/install-settings-hub-preview.py
~/.local/bin/settings-appearance-preview
```

The application menu entry is **Appearance Hub Preview**. The launcher checks the
installed KDE binary fingerprint and opens stock Settings after a KDE upgrade
until the preview is rebuilt. Normal System Settings remains available throughout.
The preview uses its own window-geometry group and a separate D-Bus instance.

The preview uses the existing native glass window decoration, with its titlebar
above the content. The sidebar is an inset, rounded island with a 10 px gutter.
Overview islands use mostly opaque glass (roughly 84–95% for the current tints)
and a 3 px beveled edge; the primary window remains glass/transparent. The island
rims watch the existing Glass Rim Intensity setting without changing opacity.
Native window actions retain KDE's unsaved-change guard. Search, All Settings, third-party category fallback, authorization, and
native Apply/Reset/Defaults/Help remain in KDE's existing implementation.

Appearance Hub’s panels, action pills, menus, mode/theme selectors and icon tiles
now use the [shared component library](../../desktop/components/library/README.md).
The local QML names are adapters; edit the canonical library to change appearance.

The desktop preview and Solid / Transparent / Liquid Glass cards use real rendered
frames from the installed KWin, Plasma, Gwenview, and Weather. A private D-Bus session,
Wayland compositor, and temporary XDG configuration clone the primary desktop's
wallpaper, panels, icons, fonts, application style, and decoration. An unchanged
selection preserves custom components and colors; staged choices use the same
material/color recipe as Apply. Gwenview and Weather sit side by side, matching
the reference scene. Recent images, saved weather locations, window state, and
forecast caches are copied into the private session; Weather uses its Flat view.
These are cached snapshots, not a feed of currently open windows.
Both image sizes have antialiased rounded corners and preserve the full frame.
The Appearance scroll view reserves its vertical scrollbar gutter in both text
directions. Its viewport width stays constant when overflow changes, preventing
aspect-ratio previews from oscillating between wider/taller and narrower/shorter
layouts. `--layout-only --screen-width 3440 --screen-height 1440` in the UI verifier
checks width stability at 51 window heights around that threshold.
Configuration changes invalidate the cache; Refresh recaptures changing wallpaper
or content. While rendering, a loading indicator replaces the image; failures
show an error rather than an invented fallback. Closing Settings cancels the
private renderer. Live desktop configuration is never written by the renderer.

The actual preview window explicitly requests the compositor backdrop effect
while the native decoration supplies the outer window rim.
**Preview desktop** explains
the scope before applying the installed Sequoia surface recipe. The guardian
restores the previous values after 15 seconds, Revert, or a disconnected parent.
**Keep** retains the look; **Apply** keeps the staged look directly after the
helper verifies it. **Undo last applied look** in the more menu restores its
previous values. Apply and Reset enable only when there are staged changes.
An interrupted pending transaction is recovered on the next preview launch.
Restoration only touches keys still equal to the trial's values, preserving later
manual edits. Receipts live in `$XDG_STATE_HOME/sequoia-eleven/appearance-hub`.

The trial changes widget style, window/header tint alpha, decoration library,
Plasma theme, menu opacity and the V6-glass/blur effect flags. Selecting Light or
Dark also applies the matching Sequoia palette and disables automatic switching;
those palette keys are journaled and restored by Revert/Undo. A material-only
change preserves the existing RGB palette. Both preserve wallpaper, panel/widget
layout, fonts, icons, window-button
placement, decoration scale and glass-rim intensity. It does not claim universal
coverage: GTK/Electron/custom-rendered apps and per-app overrides can retain their
own appearance, and some running applications need reopening. Light/dark,
individual components, theme downloads and layout changes remain accessible via
Quick Settings and the native Global Theme and other component pages.

To remove the preview and undo its last owned material test:

```sh
python3 tools/install-settings-hub-preview.py --remove
```

Validation commands:

```sh
python3 tools/verify-settings-navigation.py --hub --overlay --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
python3 tools/verify-settings-navigation.py --hub --capture-group all --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
python3 tools/verify-settings-navigation.py --hub --dark --rtl --scale 1.5 --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
python3 tools/test-appearance-material.py
```

GUI checks run under an isolated Wayland compositor and isolated XDG homes.
Material regression checks use the real configuration writer and journal, with
desktop notification calls mocked; they cover all three materials, timeout,
Revert, Keep/Undo, parent disconnection, signals, hard-kill recovery, failed
application, and later manual edits. The actual live notification/refresh APIs
were also checked without changing stored settings. Full desktop material changes
remain an explicit action in the preview.

The revised overview pages use bundled, palette-aware outline icons and grouped
native controls. Home has eight category tiles; Power has a scalable battery
illustration; Accessibility has a readable text sample and topic shortcuts;
System & Accounts groups security, language, maintenance and preferences.
Power and Accessibility topic shortcuts open the complete native module rather
than promising an unsupported deep link. Unclassified settings remain visible.

Material provider replacement unloads the outgoing blur provider before loading
the incoming one: V6 Glass rejects loading while standard Blur is still active.
The helper checks the resulting provider set; the preview renews its rounded
blur request after asynchronous effect replacement. Color transparency and
glass rim intensity remain independent.

Real compositor verification (requires the installed V6 effect and qsb):

```sh
python3 tools/verify-settings-navigation.py --hub --overlay --composited --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
python3 tools/verify-settings-navigation.py --hub --overlay --composited --dark --capture-group appearance --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

These tests paint a wallpaper behind the window in a private KWin session,
compare captures with and without Glass to prove actual backdrop contribution,
and exercise Transparent → Solid → Glass → Transparent → Glass through the real
guarded helper. Screenshots include compositor output, not just the translucent
application buffer. They also verify the 78-module catalog, native page hosting,
navigation, and unsaved-change cancellation. No live desktop settings are changed.

The reference Appearance layout keeps material cards and the rendered desktop
on the left, an ordered component list on the right, and a fixed Help / Defaults /
Reset / Preview desktop / Apply footer. Compact windows stack the component panel
without losing the footer. Save look stores named material-and-color-mode presets
in the application's `appearance-looks` data directory; this is not a full desktop
layout export. Global Theme selection/downloads, Auto, Schedule, and app overrides
route to the original KDE modules. Wallpaper and panel-layout preservation are
checked, read-only indicators because this recipe never replaces those surfaces.
The rendered preview includes the installed Plasma desktop and a real Files window.
`tools/verify-desktop-render.py` verifies native rendering and isolation;
`tools/verify-settings-navigation.py --hub --desktop-preview --composited --overlay --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check` checks image loading and rounded clipping.

Light / Dark / Auto share one pill with a 180 ms selection slide and a 120 ms
hover fade. Apply still commits staged changes. The preview observes external
palette/material changes and resets stale selections without applying a new look.
The material writer uses named KConfig files with `--notify` (an absolute filename
does not notify watchers registered for the config name), explicitly retaining
the intended XDG configuration directory. It also refreshes the application style.
Decoration palette and rim settings are watched and reparsed live. Solid and
Transparent share the latest Glass decoration geometry, rim and size controls;
all three retain the same saved Windeco values.

The composited verifier checks the live `Plasma::Theme`, an external native color
scheme change, palette repainting and selected-mode synchronization. The pill is
checked in RTL at 150% scale. `tools/preview-decoration.py --dark --material glass
--live-switch` verifies native dark/light/dark repainting without recreating the
decoration; pass `--plugin-path` to test staged plugins before installation.

Schedule and the theme/preset selector use matching 42-pixel pill controls. The
selector opens a bounded, scrollable overlay six pixels below the button. Its
glass shader samples the actual page beneath it; Transparent uses frosting,
Glass bends the edges, and Solid/software rendering keeps an opaque readable
surface. The overlay avoids Qt's native Wayland combo popup positioning issue
with embedded QQuickWidgets. Its rim follows the same shared intensity setting.
`--popup-only` on the Settings verifier checks alignment, shader contribution,
long lists, keyboard selection, Escape, outside-click dismissal and compact
layout, including RTL at fractional scaling. Preview backdrop callbacks use a
guarded widget pointer so closing the overview cannot dereference a destroyed
anchor during application shutdown.

### Appearance Hub panel settings

The sidebar has two separate floating `HubPanel` surfaces with a 12 px gap:
Appearance icons and Related settings above, and the panel selector plus shared
`S.SettingsGrid` below. Each surface retains 12 px internal padding. A pill selector identifies each live panel by edge,
screen and stable containment ID. The sidebar uses two control columns at its
normal width, retains the icon grid's three columns, and scrolls within the window.
Tiles use the component library's 12 px padding, 8 px gaps and live material theme.

`PanelSettings` calls Plasma's `evaluateScript` asynchronously. `PanelScript.js`
reads and edits the installed panel scripting API: screen/edge, alignment, length
mode, visibility, opacity, floating panel/applets, thickness, custom min/max length
and offset. Changes are immediate, as in KDE Edit Mode; the Appearance Apply/Reset
buttons still apply only the staged theme. Polling while the hub is visible picks
up external edits and added/removed panels. Commands capture the selected panel ID,
coalesce successive slider events, validate values and respect immutable panels.
The live config is read to distinguish Windows Go Below, which Plasma 6.8's
`hiding` getter otherwise reports as Always Visible. There are no direct file writes.

Widget arrangement, focus shortcuts, cloning and removal remain in KDE's native
panel editor, accessible through the grid's **Widgets & more → Edit panel…** tile.
Those actions are not exposed through the public panel scripting properties.
The installed edit-mode reference is
`/usr/share/plasma/shells/org.kde.plasma.desktop/contents/configuration/PanelConfiguration.qml`;
the matching backend is KDE's `plasma-workspace/v6.8.2/shell/scripting/panel.cpp`.

```sh
python3 tools/verify-settings-navigation.py --hub --panels-only --composited --overlay --dark --screen-width 1600 --screen-height 1100 --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

This starts Plasma on a private bus/compositor with temporary XDG homes, creates
two fixture panels and exercises real edits, keyboard selection, popup placement,
external refresh, removed-panel selection and the empty state. Live panels are
only read during development; mutations occur solely in that isolated session.

The view-navigation sidebar uses the component library's `FullIcons` treatment,
extracted from Software Center: Home, Quick Settings and category entries bind
their real selected and hover states. All Settings uses `AppearanceActionPill`
(the library's `ActionPill`) at its content width, horizontally centered at the
bottom of the sidebar. Header and action glyphs retain their existing treatment.

The sidebar Show Menu uses `AppearancePopupMenu` / shared `PopupMenu` with a
20 px rounded, sampled backdrop and the normal material fallback. It maps the
original KDE `QAction` objects into library `MenuItem`s, including visibility,
enabled state and checkmarks. The trigger uses `visible ? close() : open()`;
`CloseOnPressOutsideParent` excludes it from automatic dismissal and prevents
the second-click reopen flash. The existing native menu remains available to
legacy pages. The popup regression verifies repeated pointer toggles, Escape,
outside dismissal and exactly-once dispatch of a native checkable action.

Home, Quick Settings and category page buttons now instantiate the shared
`NavigationItem` directly. Its source lives in
`sandbox/apps/sequoia-component-library/qml/NavigationItem.qml`; the Settings
sidebar only supplies the theme, labels, source glyphs, selected-page bindings
and callbacks. The library owns the 42 px pill geometry, 8 px symmetric vertical
padding, Full Icons, hover/focus surfaces and disabled state.
