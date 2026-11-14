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

## Appearance Hub Quick Settings

The separate Appearance Hub preview restyles Quick Settings with the shared
Sequoia library while retaining the installed `kcm_landingpage` backend.
A preview-only QML URL interceptor replaces its main presentation, so KDE still
owns theme assignments, automatic day/night switching, animation speed,
file-click behavior, most-used pages, Apply/Reset/Defaults, and unsaved prompts.
The themed footer calls the original ModuleView actions; native footer buttons
remain available internally and are displayed again on other modules.

The top-left circular Back button returns through normal navigation, deferring
the call until its QML click handler finishes because leaving destroys the KCM.
The sidebar Back button remains. Theme pack pickers use KDE's lazy, variant-filtered
model, with previews and keyboard navigation. The page reflows its theme cards
at narrow widths and uses scrolling edge fades without a visible scrollbar.

`ThemePreviewCard`, `ChoiceCard`, and `SliderPill` live in the standalone
`sandbox/apps/sequoia-component-library` package, with interactive catalog samples
for Solid, Transparent, and Glass in Light and Dark. Existing pills, circles,
popups, and highlights are reused.

```sh
python3 tools/verify-settings-navigation.py --hub --composited --overlay --dark --quick-only --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

This isolated check covers native staging/persistence and Reset, light/dark/auto
selection, theme pack assignments, popup toggling and keyboard navigation,
slider keys, unsaved-change cancellation, circular back navigation, and compact
layout. The shared library also checks pointer dragging, disabled choices,
exclusive cards, and all six material/color combinations.

## Appearance Hub Global Theme

The preview also replaces the presentation of KDE's `kcm_lookandfeel` while
keeping its native model, configuration skeleton, and apply/save/remove methods.
Its responsive theme grid uses shared `ThemePreviewCard` controls, with rounded
local previews, wrapping names, and a rounded actions menu. Theme application
retains the simple/detailed component choices and the desktop-replacement warning.
Only the components selected in the confirmation dialog are applied.

Automatic mode retains the light/dark package filters, day/night configuration,
idle delay, Apply, Reset, and Defaults. Save Current Theme keeps KDE's duplicate
name validator, image chooser, screenshot maker, and package creation. Theme
removal still requires confirmation and protects active/system packages; Get New
uses KDE's existing download action and model-refresh callbacks. Full-screen
preview remains available. Local Back first returns from selectors/save to the
main Global Theme page, then leaves through the existing unsaved-change handling.
The sidebar Back button remains available.

The shared library now includes `RoundedPreview`, `SurfaceDialog`, `CheckPill`,
`SpinPill`, and `TextFieldPill`. `ThemePreviewCard` uses `RoundedPreview` for both
single and split artwork, including Quick Settings' Light, Dark, and Automatic
cards. The preview corners use an antialiased 12 px radius in both renderers.

```sh
python3 tools/verify-settings-navigation.py --hub --composited --overlay --dark --global-only --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

This test uses copied theme fixtures and a temporary KDE defaults directory. It
applies only fixture colors, verifies component selection and layout warnings,
checks automatic assignments/idle persistence, saves a temporary theme, removes
only a copied fixture, tests full-screen preview, and checks compact layout and
Back. The library catalog verifies rounded alpha corners, input behavior, and
dialogs across all materials in Light and Dark.

## Appearance Hub Application Style

The preview replaces only the presentation of `kcm_style`. Its native model,
settings skeleton, Qt/Union previews, style configuration, installation, staged
uninstall/undo, and Apply/Reset/Defaults remain owned by KDE. The page uses shared
pill actions, a round Back button, rounded action menus, and a responsive live
preview gallery. The sidebar Back button remains. Icons and toolbar labels use
shared check pills and selectors in a themed dialog. The GTK subpage retains its
native theme model, archive installation, downloads, preview, and removal actions;
its entry follows KDE's GTK configuration service availability.

`LivePreviewCard` is a new reusable library component on the Rounded rectangle
catalog page. Hosts provide live content through `previewComponent`; pointer and
keyboard activation select the card, while its circular actions trigger stays
independent. All six material/color combinations use the same implementation.

```sh
python3 tools/verify-settings-navigation.py --hub --composited --overlay --dark --style-only --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

The isolated check covers Qt/Union staging, Apply and persisted settings, Reset,
staged removal/undo of a copied Union fixture, icon/toolbar options, dialog input,
GTK availability gating, material variants, compact layout, and circular navigation.

The broken Light/Dark Liquid Glass **Global Theme packages** have been retired.
The generator now maintains only standalone Glass palettes, so builds cannot
reintroduce those packages. Liquid Glass's compositor effect, native application
style, decorations, Plasma styles, palettes, and material controls remain intact.

## Appearance Hub Plasma Style

`PlasmaStyle.qml` restyles the installed `kcm_desktoptheme` presentation while
retaining native search/color filters, live SVG theme previews, theme selection,
Apply/Reset/Defaults, archive installation and URL drops, download callbacks,
external theme editing, and staged deletion with undo. Active and system styles
remain protected by the native model. The circular Back button uses normal
unsaved-change handling and leaves the sidebar navigation available.

The responsive gallery reuses `LivePreviewCard`, with 360 px maximum card widths.
Search, filter, actions, menus and the footer use shared Sequoia controls. The new
`MessagePanel` library component shows installation success/error feedback, with
information and warning variants in the Rounded rectangle catalog page. It
supports wrapped text, all six material/color variants, and accessible dismissal.

```sh
python3 tools/verify-settings-navigation.py --hub --composited --overlay --dark --plasma-only --binary .staging/settings-hub-build/bin/systemsettings --skip-install-check
```

The test uses copied Plasma style packages and a private configuration directory.
It exercises native search/filter/empty state, selection/Apply/Reset, persistence,
active-theme removal protection, staged removal/undo, installation of a temporary
archive, popup toggling, unsaved navigation, theme variants, and responsive layout.

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
The Appearance scroll view hides its vertical scrollbar while retaining scrolling
and reserves its gutter in both text directions. Its viewport width stays constant
when overflow changes, preventing
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

The Appearance layout uses one vertical scroll: material cards, the desktop
preview and its options, a horizontal appearance-icon pill, an accent-color pill,
and the wide panel-settings island. Help / Defaults / Reset / Preview desktop /
Apply stay in a fixed footer. The icon and accent strips scroll horizontally on
narrow windows; keyboard focus brings an offscreen option into view. Save look stores named material-and-color-mode presets
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

Appearance icons now share a 58 px pill below the desktop preview options,
with 8 px inset padding, 42 px icon targets and a circular Related settings button.
There is no close button; More appearance options can still toggle the strip.
Related settings uses AppearancePillMenuItem, matching Show Menu and More.

The 58 px accent pill follows the icon strip, then panel settings occupy the full
preview width. Accent color and Panel settings titles sit above their respective
surfaces. The panel island keeps 12 px padding and a selector/refresh row.
Its grid uses up to six columns with 160 px minimum cells, 12 px tile
padding and 8 px gaps. Hiding the icon strip does not stop panel updates.

Accent swatches stage a custom color and enable the footer Apply/Reset buttons.
Apply uses KDE’s `plasma-apply-colorscheme --accent-color` command; Reset discards
the pending color. Custom color opens a chooser initialized to the pending or
current accent. It uses the library Panel with a 24 px radius,
ActionPill confirmation buttons and CircleButton color options. The surface follows
solid/transparent/glass and the host light/dark palette, sampling the page behind
it for a rounded, masked backdrop blur. The spectrum supports pointer dragging and arrow
keys; hue and hex entry allow arbitrary opaque colors. Use color stages the
selection for the footer Apply button; Cancel or Escape leaves it unchanged. The content scrolls
inside small windows. More colors opens the native Colors module. The backend
validates the color and effective installed scheme (including KDE's inherited
kdedefaults), handles failures/timeouts without discarding the pending choice, and
preserves Sequoia's alpha for window, header and decoration surfaces. It writes
the chosen custom-accent source/history with KConfig notifications and refreshes
the palette. Apply disables wallpaper-generated accents before applying a custom
color, without changing automatic light/dark mode; failed application restores the
previous accent source. A combined material/mode and accent Apply applies the
accent after the look finishes. Accent application has its own busy state,
separate from timed theme trials. Swatches are disabled while an application or
trial is in progress. The isolated integration check verifies staging/reset,
inherited schemes, automatic-to-custom selection, palette updates, retry behavior,
and combined theme/accent changes.


`PanelSettings` calls Plasma's `evaluateScript` asynchronously. `PanelScript.js`
reads and edits the installed panel scripting API: screen/edge, alignment, length
mode, visibility, opacity, floating panel/applets, thickness, custom min/max length
and offset. Changes are immediate, as in KDE Edit Mode; the Appearance Apply/Reset
buttons apply the staged theme and accent choices. Polling while the hub is visible picks
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
original KDE `QAction` objects into `AppearancePillMenuItem`s, including visibility,
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

Show Menu sits to the right of the sidebar search field at 42 × 42 px, with an
8 px gap. Its rows and More appearance options share `AppearancePillMenuItem`,
which uses the library’s `PillHighlight` for the same glass rim and capsule shape.

The Icon pack card sits immediately below Accent color and above Panel settings.
It uses the shared ThemeSelector pill, five samples from the selected installed
pack (with inheritance fallback), and a CircleButton shortcut to the full Icons
module. The selector and samples share one row with 12 px spacing. The selector
and popup fit the longest installed pack name, measured at the menu's selected
font weight, with control padding. Narrow windows constrain the selector and
allow the sample strip to scroll. Selection applies immediately, validates the installed pack, honors locked
configuration, writes only Icons/Theme and broadcasts KIconLoader changes using
the same cache invalidation path as KDE's Icons KCM. Hidden/internal and unsupported
GNOME-only themes are excluded. The card refreshes installed packs on return to
Appearance and follows external theme changes through the existing config watcher.
Selection is disabled during appearance operations or unapplied material choices.

The sidebar header uses the active icon pack's full-color preferences-system icon.
ScrollEdgeFade masks viewport alpha only where content continues offscreen: 24 px
at the main/sidebar edges, 16 px for icon/accent/sample strips, and 20 px inside
the custom color chooser. Fully revealed ends remain opaque. The original
Flickable retains wheel, touch, focus and keyboard handling; the effect adds no
input surface or scrollbar. The software renderer keeps normal clipped content
without a shader effect. The layout check verifies edge conditions, wheel input,
rendered fading, the header icon and stable preview geometry.


The Colors page is a presentation of the native `kcm_colors` backend. It uses
bounded 360 px LivePreviewCards, SearchField/ThemeSelector filters, the shared
ColorSwatch and ColorDialog, and a deferred CircleButton back action. Native
scheme selection, accent source/value, Apply/Reset/Defaults, scheme editing,
file import, NewStuff events, staged removal and undo remain in KDE. Preview
colors come from each scheme's actual palettes and KDE's accent/tint functions.
The accent panel fits its controls and wraps on narrow windows. Color schemes
start at the upper-left; content scrolls with edge fading and no visible vertical
scrollbar. The old sidebar Back remains.

`tools/verify-settings-navigation.py --hub --composited --overlay --colors-only`
uses copied color schemes and private configuration, data, cache and DBus roots.
It checks native scheme/accent persistence, search/filter/reset, automatic-to-custom
accent selection, custom chooser validation/cancel/accept, active-scheme protection,
removal/undo/import, repeated menu toggle, unsaved Back and responsive layouts in
all six material/mode combinations. No live desktop theme is applied by this test.
