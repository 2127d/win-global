# Gwenview override

Workspace for a presentation fork of Gwenview 26.08.1, matching the installed
`gwenview-26.08.1-1.fc44.x86_64` package. The presentation fork adds a clean welcome page and a thumbnail sidebar for
recent images, while retaining the native viewing and editing controls.

## Layout

- `gwenview-26.08.1/`: modified upstream sources with original licenses, translations,
  image loading, editing, metadata, and file operations preserved.
- `override.json`: baseline version, source URL, archive digest, and status.
- `upstream.sha256`: SHA-256 of the downloaded upstream archive.

Source archive:
https://download.kde.org/stable/release-service/26.08.1/src/gwenview-26.08.1.tar.xz

The archive is cached at `.staging/gwenview-source/` in the repository root.
The recorded digest identifies this downloaded baseline; it is not a signature
verification. The extracted regular files were compared byte-for-byte with
the archive.

## Presentation entry points

Gwenview's main interface uses C++/Qt Widgets and KDE XMLGUI.

| File within `gwenview-26.08.1/` | Responsibility |
| --- | --- |
| `app/gwenviewui.rc` | Menus, toolbar actions, ordering, and separators |
| `app/mainwindow.cpp` | Window layout, action wiring, mode changes, and navigation |
| `app/viewmainpage.cpp` | Image viewing page and thumbnail bar |
| `app/browsemainpage.cpp` | Folder browsing and image grid |
| `app/sidebar.cpp` | Sidebar presentation |
| `app/infocontextmanageritem.cpp` | Image information in the sidebar |
| `app/imageopscontextmanageritem.cpp` | Existing image-editing actions |
| `app/fileopscontextmanageritem.cpp` | Existing file-management actions |
| `app/fullscreencontent.cpp` | Fullscreen controls |
| `app/spotlightmode.cpp` | Existing focused viewing mode |
| `app/savebar.cpp` | Modified-image and save UI |

## Welcome page and recent images

Gwenview opens to a welcome page headed **Open your images**, with a vector
image-stack mark and a compact action group just above center. **Open images…**
uses the native multi-file picker; its adjacent shortcut hint follows the actual
configured Open action. **Browse a folder…** opens a directory picker and then
Gwenview's normal Browse mode. Supported image drags add an accent outline,
subtle tint, and **Drop to open** guidance. Leaving, dropping, or hiding the page
clears that state; unsupported drags do not highlight the target.
The welcome page sits in a rounded, floating panel matching the viewer's
7-pixel inset, 16-pixel corners, and theme surface color. Browse mode uses the
same panel around its folder navigation, thumbnail grid, filters, and status
controls, with inner padding. Its folder header exposes
native Sort, View, and Filters menus; header actions collapse to icons in smaller
windows. Rounded thumbnail cards distinguish hover, selection, and keyboard focus
without replacing Gwenview's thumbnail generation or file operations. The native
location bar collapses breadcrumbs and still supports F6 editing. Filter cards
stack their fields when narrow and scroll independently in a bounded area. Empty
folders and filtered-out results explain how to continue. Oversized thumbnails
temporarily fit the viewport and regain their preferred size when space returns;
only the preferred size is saved. Panel gutters are slightly tighter, with 10px
between the viewer and thumbnail strip and a 6px floating-control gap.

Fullscreen image controls use separate floating pills for the description/count,
view controls, navigation/slideshow, zoom, rotation, and pin/options/exit. Soft
shadows, fine rims, 44px surfaces and rounded selection states match the normal
floating bars. A shared, quarter-resolution scene capture adds blur behind the
fullscreen islands and preview tray without blurring their text, icons or image
previews. Captures coalesce at 32ms, stop offscreen, and reuse unchanged frames
instead of repainting continuously. The groups stack in a narrow image pane. Native previous/next,
slideshow, zoom, fit, actual size, rotation, sidebar, Browse and exit actions keep
their shortcuts and enabled states. Controls, sidebar and dialogs retain the
desktop's light/dark palette; the image canvas keeps its fullscreen background.
Thumbnail previews have their own rounded floating panel and visible selection borders. The
optional thumbnail row spans the panel, defaults to 120px, and keeps at least
104px of height so selection controls and the scrollbar leave a usable preview.
Its preferred height is bounded by available screen space. A pin keeps controls visible; otherwise the top edge
reveals them, and keyboard focus, menus and dialogs protect ongoing interaction.
Fullscreen exit cancels hiding and restores the cursor, including rapid toggles.

Thumbnail loading checks the visible/prefetch region before fetching file details,
retains a bounded nearby cache, and starts queued work after 80ms. Browser grids
and the fullscreen strip preserve touchpad pixel deltas. Direct scrolling stops
the strip's selection-centering animation instead of competing with it.
`tools/verify-gwenview.py --thumbnail-scroll` exercises a 2,048-image folder in
normal and fullscreen modes, including scroll endpoints and the enlarged strip.

Fullscreen Browse uses the same floating surfaces and action buttons as the
fullscreen image view. Navigation islands align with the header's left edge,
with the location island directly beside them or on a separate left-aligned row
when narrow. The breadcrumb uses the same 44px pill surface and shadow space as
the neighboring controls, with its inner field vertically centered. Its background
has capsule ends. Its island sizes to
the path, grows with deeper locations, and caps at the available width; editing
with F6 temporarily uses the full available space. Normal Browse uses the same
rounded, content-sized breadcrumb field. Folder navigation animates the island's
width for 200ms with an InOutQuad curve and a fixed left edge. Rapid navigation
retargets from the current width. Sidebar/window resizing and F6 editing take
precedence immediately; hidden islands and disabled desktop motion skip the
animation. The row height stays fixed throughout. The navigator paints the inner
text field itself as a capsule, with a theme-colored surface and focus rim,
instead of clipping a rectangular native frame.

Folder navigation in normal and fullscreen Browse adds a 200ms, 28px gallery
slide. Parent/back navigation reverses its direction, including RTL layouts.
Only one viewport-sized frame is retained during the transition; directory
listing and thumbnail delivery continue, with no per-frame thumbnail painting
or whole-folder capture. The effect is disabled and its frame released when
finished. Clicks, keys, scrolling, resizing, leaving Browse and reduced-motion
settings bypass or dismiss the slide immediately. Separate title and location surfaces accompany grouped
view and folder-navigation pills. Its navigation area contains the folder title, native
Sort/View/Filters menus, a persistent sidebar toggle, View, folder Back/Forward,
Up, and Exit Fullscreen. Folder history uses KUrlNavigator and also accepts
Alt+Left/Alt+Right while the browser has focus. A narrow pane places the location
bar below navigation, so opening or resizing the sidebar never squeezes out the
path or exit button. The gallery has its own rounded surface, bounded filters,
and thumbnail controls; the sidebar toggle remains available when the status
bar is hidden. Location and thumbnail icons follow the active palette. Escape
and the exit button restore the normal browser layout and native toolbar.

The native Properties dialog retains KDE's pages, file editing, permissions,
checksums and Apply/Cancel behavior. Rounded tabs, padded fields, calm button
states and balanced margins follow the active theme. Opening uses a 200ms
slide/fade of one cached frame inside the native window, so it also works on
Wayland without moving the window. Input cancels the entrance immediately;
resizing/closing clears it, and disabled desktop animation skips it entirely.

Sidebar show/hide uses a coordinated 180ms ease-out. The sidebar outline in the
button stays steady while its chevron folds smoothly to the new direction.
The sidebar's live contents move at their full width, so its text and footer
do not compress and opening never waits for a screenshot capture. Rapid clicks reverse from the current
position. Completed toggles preserve the preferred width and native splitter
constraints; resizing and mode changes finish immediately. Reduced-motion
settings skip the slide. Both layout directions are supported.

The sidebar has a live recent-image count and an overflow menu for choosing a
thumbnail grid or a list with filenames and parent folders. That view preference
survives restarts. The same menu exposes removal of the selected history entry
and clearing history; these commands do not delete image files. Grid tiles and
list rows have distinct hover, selection, and keyboard-focus states.

Three equal footer tabs show their icons above **Recent**, **Information**,
and **Tools** labels when space permits, switching to centered icons with tooltips
when the sidebar is narrow. The Recent history symbol is drawn independently of
the installed icon theme, and shortcut mnemonic markers are never displayed. The selected tab has a softly raised capsule, with a visible
keyboard focus ring. The sidebar anchors the footer while the page content scrolls. Page changes slide horizontally for 200ms with InOutQuad easing; the footer remains fixed, rapid changes retarget immediately, and disabled desktop animations switch instantly. Before opening an image, Information
and Tools explain what they will show; opening an image reveals the native
metadata and operations. Returning to the welcome page restores these empty
states, so stale image details are never presented as current.

Information and Tools use softly outlined section cards and consistent headers.
Metadata pairs wrap above one shared page scrollbar; rating, tags, and a compact
description editor follow below. Long tags elide with their full text available
on hover, and rating stars follow the active palette. The old nested information splitter is replaced
by this vertical flow. Tool rows wrap long action labels and retain native
QAction behavior, tooltips, menus, and keyboard focus. Both pages scroll above
the fixed footer at narrow widths. The empty states have matching accent symbols.
The welcome buttons share the main panel's horizontal center; the opening
shortcut sits independently to the right of the primary action.


The sidebar uses Gwenview's existing thumbnail provider and recent-file history.
Recent images appear as equally sized, center-cropped square previews with
slightly rounded corners and compact gaps. Columns adapt to the sidebar width.
Filenames and paths are available on hover; opening a tile displays the original
full image.
Reopening an image moves it to the top; history survives a restart and respects
Settings → Remember history. Its context menu can remove an entry or clear
history without deleting image files. Existing saved folder-tab selections
migrate to the Recent tab. A single rounded, inset panel surrounds the header,
content, and Recent/Information/Tools buttons. The panel uses the theme
surface color, including dark mode, and keeps native tab selection behavior.

The image picker accepts multiple files. Multiple dropped/selected images use the
same temporary-link approach as upstream's multi-file command line, with unique
link names for files from different directories that have identical names.
Next/previous and editing use the native viewer. History records original image
paths, not temporary links; links are kept until the application closes.

## Rounded viewer and floating controls

In image view, KDE's **Show Toolbar** and **Show Status Bar** checkboxes select
between the normal bars and the custom floating controls independently. Both
bars are shown by default. With a checkbox checked, its original native bar is
visible and the corresponding glass pill is hidden. Unchecking it enables the
auto-hiding pill for that bar in image view. Checking it again restores the
native layout, spacing, palette, icons, and control styling. The choices persist
across restarts using Gwenview's existing settings.

Switching among Home, Browse and image view uses a quick 200 ms slide. Each page is
captured once, so animation frames move cached images without repeatedly painting
the thumbnail grid. The destination becomes active immediately; further input
cancels even a pending transition, rapid switches never queue, and disabled desktop
animations switch directly. Home's slide continues while its sidebar opens,
resizing the cached frames instead of recapturing or cancelling the transition.

Toolbar arrows and Left/Right image navigation slide for 200ms in normal and
fullscreen view, with direction reversed for Previous and RTL layouts. The old
frame stays visible until the next image is ready. Two viewport-sized snapshots
are captured once; animation frames do not decode, rescale or repaint the source
image. The native viewport geometry and floating controls remain unchanged.
Rapid navigation replaces the pending slide, while interaction, sidebar/window
resizing and leaving the viewer dismiss it immediately. Slow loads return to the
native loading indicator after 600ms. Reduced motion skips the transition; compare
mode, slideshow playback and the legacy OpenGL animation mode retain native
behavior. `tools/verify-gwenview.py --image-slide` checks toolbar/keyboard navigation,
48-megapixel images, cached frame bounds, responsiveness, rapid changes and sidebar
resizing in both modes; add `--dark` for the dark palette.

The top-right toggle expands left into separate floating glass pills. Adjacent
navigation arrows share a pill, with view modes, editing tools, and window controls
grouped separately. The pills share the native toolbar's actions, menus, shortcuts,
and enabled states. Customization preserves action order, explicit separators,
and embedded widget controls while omitting layout-only spacers. Empty gaps pass
clicks through to the image. Narrow layouts scroll horizontally with enough height
for both the controls and scrollbar. Floating icon buttons hide
the extra dropdown-arrow indicator while keeping their menus available. Start and Browse keep KDE's
normal toolbar visibility preference and hide the image-view pill. Returning to
image view restores the pill collapsed if the native toolbar remains hidden.
Fullscreen hides the main toolbar and its pill; leaving fullscreen restores the
previous choice. Spotlight retains its floating controls and restores the
previous bars when leaving that mode.

Dropdown menus, context menus and submenus use the same cached rounded surface
and soft floating shadow in normal and fullscreen views. Their native actions,
checkmarks, separators, shortcuts, keyboard handling and widget controls remain
intact. Combo popup padding is included in its minimum width so zoom percentages
stay readable; the native list still owns scrolling and selection. Menus follow
the desktop palette, including live palette changes, and avoid a duplicate system
shadow. A 120ms eased slide presents a single cached popup frame without
moving native hit targets. Mouse movement, clicks, keyboard navigation and wheel
input reveal the live menu immediately; closing clears pending animation, and
reduced motion skips it. Menus follow SequoiaEleven's native material rules:
Transparent variants use the desktop Window color at the palette’s opacity; Solid variants
use 100%. The material remains constant during animation and Share-menu refreshes,
and live desktop palette changes apply to all popup types and palette groups.
Combo views do not paint a second background over their container's material.

Transparent menus request native compositor blur inside the rounded body only.
The region follows the brief opening slide and excludes the shadow gutters, so
there is no rectangular blur halo. Plasma's Blur desktop effect owns its enabled
state and strength; Gwenview neither changes those settings nor substitutes an
opaque or software-blurred background when the effect is disabled. Solid menus
use a nonempty off-surface region to retain KDE's surface-destruction tracking.
Do not replace this with `enableBlurBehind(false)`: KWindowSystem 6.31's Wayland
backend drops its lifetime watcher, then retains a new effect object for the old
surface, causing a fatal protocol error on reopening. An empty region means
whole-window blur and must also be avoided. Fullscreen control-pill blur is unchanged.
Monochrome theme menu icons follow the text color while colored icons and
background-color previews retain their original appearance. Sidebar buttons show a 320ms expanding accent outline above the controls;
the wave is input-transparent, restarts on repeated clicks, and skips reduced
motion. `tools/verify-gwenview.py --popup-preview` checks real dropdowns, context
menus, submenus, cached entrance frames, immediate input, reduced motion, native
blur requests, zoom scrolling and the visible sidebar ripple in both view modes;
add `--dark` for the dark palette and `--transparent` to exercise the transparent
desktop theme with translucent menu backgrounds and bounded blur. `--popup-lifecycle` walks the populated
menu catalog and nested menus repeatedly, refreshes Share entries while open,
changes its inherited palette mid-opening, and repeats zoom, context/submenus and
fullscreen settings. It checks theme opacity and the blur region during and after entrance in both
normal and fullscreen modes.

`--native-menus` runs that same audit against actual Wayland popups and pointer
grabs on a private KWin compositor, using EIS input restricted to the isolated
session. It needs the existing libei test SDK (or development headers), KWin and
Qt DBus. `--blur-disabled` starts only that private compositor with Blur off.
The test queries the effect's actual loaded state and toggles it off/on while
Share is open, checking that opacity does not change and no protocol error occurs.
No live desktop input, system-settings changes, or sharing actions are sent. The older focused
`--popup-lifecycle --wayland-socket /absolute/socket` test remains available: it
uses tool-role menus and seeded blur requests to reproduce the KWindowSystem
surface-lifetime failure, independent of pointer-grab handling.

Normal toolbar/status pills and fullscreen image/Browse islands share one smooth
surface renderer. Decorative glass, rims and shadows resolve from twice the
device resolution and are cached, including at fractional desktop scaling. Text,
icons and previews remain native and sharp. Shadow interiors use soft alpha
erasure; normal toolbar input masks leave room for the antialiased edge while
keeping gaps between groups clickable. `tools/verify-floating-surfaces.cpp`
checks edge coverage, shadow transparency and cache reuse at 100%, 125%, 150%
and 200% scaling in both palettes. Sidebar and normal thumbnail panels use the
same cached surface treatment. The image viewer keeps its native-resolution
pixels and smooths only four small corner tiles; its viewport paints the image
background explicitly so transparent corners reveal the parent and shadow.
Fullscreen bypasses the corner effect and keeps the full image area. The floating
status bar removes its extra end padding, matching the toolbar's 8px visible gap
to the toggle. `--panel-edges` checks the actual viewer's alpha edge and parent
background in the GUI.

Floating surfaces follow the Weather makeover's elevation treatment: soft,
downward shadows, restrained borders, and calmer glass highlights. Toolbar
pills and their circular toggles have a wider shadow than the sidebar, welcome,
Browse, and thumbnail panels. Shadow layers stay outside the glass and never
blur controls or intercept image gestures. Toggle shadows fade with their
buttons and disappear with native bars or fullscreen. The image viewer and
thumbnail panel cast matching shadows into the page's shared gutters, keeping
their visible edges aligned without reducing the width of the thumbnail surface.

Both collapsed toolbar toggles fade out when the pointer is away from their
corner. Moving within 48 pixels of a toggle smoothly reveals it in 180 ms;
a wider exit boundary avoids flicker at the edge. Expanded toolbars stay visible.
Invisible toggles pass mouse input through to the image, while keyboard focus
reveals them for tab navigation. Disabled desktop animations switch immediately.

The main image canvas has an inset, rounded outline and soft floating shadow
matching the sidebar. Its shadow stays outside the image and disappears in
fullscreen. In
Auto background mode it uses the theme's raised surface color; explicit image
background choices remain available. Fullscreen retains its edge-to-edge view.

With **Show Status Bar** unchecked, the bottom controls become a floating
control at the lower right of the image area. It starts as a circular toolbar
button. Clicking expands the native controls to the left with a 220 ms animation; clicking again collapses them.
The expanded controls form three separate floating pills: sidebar/thumbnail
visibility, image count or comparison synchronization, and Fit/zoom/scale.
Each matches the main toolbar's 44-pixel rhythm, rounded ends, and soft shadow.
The panel toggles use matching 22-pixel icons with descriptive tooltips while
floating; docking restores their original native labels and icon sizes.
Clear gaps pass clicks through to the image. Both bars and their toggles share
a restrained blurred glass finish, a smooth highlight, and a fine rim that
adapt to light and dark palettes. The image
scene behind the pills is sampled and blurred separately from the controls,
refreshing as images, zoom, or the viewing area change. Fully faded controls
skip backdrop rendering. Action-state updates reuse tinted icons without
restyling the whole toolbar, and browser layout notifications are combined
into one update per event-loop turn. Controls have consistent gaps. Transparent
resting buttons let the glass read as one surface per group, with quiet hover
fills, accent-colored selections, and visible keyboard focus. Buttons use
rounded inset corners that follow their enclosing pill; the zoom track
and thumb are rounded as well. The floating zoom field has fully rounded capsule
ends and a stronger local shadow, while retaining native editing and arrow hit
geometry. Docking restores its system styling. The popup retains the system
style: browsing the open list with the wheel, pointer, or keyboard leaves the
image scale unchanged until a click or Enter commits a selection; Escape cancels.
With the list closed, hovering and scrolling steps directly through the full
percentage range. Presets remain sorted when the fit scale changes, and stepping
uses actual values rather than rounded labels. Fractional wheel and pixel-only
trackpad input accumulate into steps. Both the native and floating status bars
use this behavior; fullscreen's separate zoom controls are unchanged.
The floating expand/hide toggles use a visible 320ms outline wave in the current
accent color. The line expands beyond the button into its shadow margin, with no
filled flash, a quiet resting selection, and a separate keyboard-focus ring. Repeated
presses restart one animation; disabled desktop motion skips the ripple.
Toolbar text, control surfaces, and monochrome
icons refresh on system theme changes without restarting, including disabled
and selected states and an already-open zoom menu. Repeated clicks and mid-animation reversals preserve the slide;
disabled desktop animations switch directly to the requested state.
Zoom, Fit, image count, sidebar, thumbnails, and comparison synchronization use
the existing controls. Opening the toolbar does not resize the image. A narrow
pane can scroll the controls horizontally, reserving a separate scrollbar row
so it never overlaps the controls. Checking **Show Status Bar** moves
the same controls back below the image and removes the pill styling, reserving
normal layout space and restoring its original stretch spacing.
The Bird's Eye View navigation box is hidden by default when zoomed in, keeping
the area behind the floating toolbar clear. It can still be enabled from the
View menu.

The thumbnail strip occupies a separate rounded island with 8-pixel inner
padding and a 12-pixel gap from the viewer. Its 16-pixel corners and theme
surface match the main canvas and sidebar. The gap remains a resize handle;
horizontal/vertical placement, row counts, scrolling, selection, and the native
Show Thumbnails action are preserved. Fullscreen and spotlight mode hide the
whole island and restore it when returning to the normal view.

## Build and install

Install the dependencies listed by upstream CMake, including optional feature
libraries to match the distribution build. This build retains Baloo, RAW, FITS,
annotation, sharing, color management, and Wayland gesture support.
The graphical installer's Gwenview selection installs these dependencies, including
`kimageannotator-qt6-devel` and `kcolorpicker-qt6-devel` on Fedora. Both development
packages are needed even when the corresponding runtime libraries are installed.
Builds use all available logical CPU cores by default; `BUILD_JOBS` can set an
explicit positive job count.

From the repository root:

```sh
python3 tools/build-gwenview.py
python3 tools/verify-gwenview.py
python3 tools/install-gwenview.py --skip-build
```

An optional `--sdk /path/to/usr` argument accepts a local development prefix.
On this machine the SDK is cached at `.staging/gwenview-sdk/usr`, with
`/tmp/sequoia-gwenview-sdk` linking to that cache for existing build settings.
Its CMake packages and Wayland pkg-config paths must resolve within that prefix;
it is only needed at build time. Output goes to `.staging/gwenview-build/`.

The installer places the executable and its matching `libgwenviewlib.so.5` in
`~/.local/share/sequoia-eleven/gwenview/`, with a `$ORIGIN` library search path.
A per-user `~/.local/bin/gwenview` launcher and desktop entry select this build.
Stock executables, shared libraries, KParts, and file associations are preserved.
The matching system package provides translations and other unchanged resources.
The launcher falls back to the system version when the packaged executable or
Gwenview library changes. Rebuild the fork after package upgrades.

Close existing Gwenview windows and reopen it after installing. To restore the
previous launcher and desktop entry:

```sh
python3 tools/install-gwenview.py --remove
```

Verification runs the actual application on a private session bus with isolated
XDG configuration and disposable images. It checks the welcome page, picker,
drag/drop, same-name multi-file navigation, MRU ordering, recent activation,
native rotate/save, persistence, and disabled history. It also checks private
library loading, argument forwarding, upgrade fallback, and reversible install.
Captures and logs are written to `preview/gwenview/`. Sidebar preview checks also
cover the four toolbar/status-bar combinations, repeated native styling restoration,
mode changes, and persistence of both native and pill choices across restarts.
They also scroll the closed zoom field through every preset in both directions,
exercise fractional wheels, pixel-only trackpads and arbitrary slider scales,
and check popup cancellation, ripple reversal and reduced-motion activation.
`tools/verify-zoom-combobox.cpp` additionally exercises preset rebuilding with
fit scales down to 0.1%, including percentages whose rounded labels coincide.

To capture all three sidebar pages with the project theme:

```sh
python3 tools/verify-gwenview.py --sidebar-preview
python3 tools/verify-gwenview.py --sidebar-preview --dark
python3 tools/verify-gwenview.py --sidebar-preview --transparent
python3 tools/verify-gwenview.py --sidebar-preview --dark --transparent
```

To verify and capture the welcome page and sidebar in both themes:

```sh
python3 tools/verify-gwenview.py --welcome-preview
python3 tools/verify-gwenview.py --welcome-preview --dark
```

These checks exercise opening images and a selected folder, configured shortcut
hints, drag acceptance and cancellation, footer labels and keyboard navigation,
contextual empty states, compact-window scrolling, grid/list switching and
persistence, live counts, and removing/clearing history without deleting images.

To verify the browser at wide, 720×500, and 640×480 window sizes:

```sh
python3 tools/verify-gwenview.py --browser-preview
python3 tools/verify-gwenview.py --browser-preview --dark
```

Browser checks cover native sorting, hidden files, thumbnail details, multiple
selection, all filter controls, empty states, location editing, fullscreen,
status-bar visibility, activation, and preferred thumbnail-size persistence.

To verify fullscreen viewing controls, compact layouts and auto-hide behavior:

```sh
python3 tools/verify-gwenview.py --fullscreen-preview
python3 tools/verify-gwenview.py --fullscreen-preview --dark
```

To verify fullscreen browsing with the sidebar open at 1280×800 and 640×480:

```sh
python3 tools/verify-gwenview.py --fullscreen-browser-preview
python3 tools/verify-gwenview.py --fullscreen-browser-preview --dark
```

These checks cover control visibility, responsive rows, menus, filtering,
selection, thumbnail size, folder history, F6 location editing, status-bar
preferences, sidebar toggling, image/browser transitions and repeated exits.

Live menu material changes watch Plasma palette notifications and atomic
`kdeglobals` replacements. Existing open/hidden popups refresh together; their
rounded blur regions follow the chosen material while KWin controls the Blur
effect. No restart is needed to change themes after loading the updated binary.
Monochrome command icons are tinted without flattening shaded application icons.
Browse breadcrumbs measure KDE's actual controls, ignore retired path buttons,
and preserve their quick width animation. Browse popup buttons retain keyboard
Tab focus without retaining mouse-click focus; the sidebar toggle uses a 22px
icon in a minimum 36×32 target.

Selection changes now preserve the global menu's action list and Open With's
application actions when their contents are unchanged. This avoids redundant
remove/add exports through Plasma's asynchronous menu importer.

The popup lifecycle check also switches light/dark and solid/transparent themes
on open Share, Open With and zoom menus, reopens them, checks hidden submenus,
and verifies stable global-menu command identities. `--global-menu` additionally
runs the real Plasma widget in a private compositor and audits its imported
commands while Gwenview changes images and fullscreen state.

### Menus, dialogs, and preferences

All Gwenview-owned dialogs now share the Sequoia presentation layer, including
KDE dialogs created lazily by menu actions. Properties and tabbed utility windows
use the real native tab bar as an animated segmented pill. The original pages,
keyboard navigation, accessibility, permission editing, checksums and plugin
metadata remain available. Properties opens at a usable 620×580 size.

General, Image View and Advanced preferences have grouped cards and independently
scrollable pages. The existing KConfig bindings, dependency rules, Apply, Defaults
and Cancel behavior are retained. Dialog entrance, page and pill transitions use
200 ms InOutQuad easing; input immediately dismisses a cached entrance frame and
reduced-motion settings skip animation. Crop, color adjustment and red-eye panels
use the same timing instead of the old 250 ms linear slide. Find Action uses the
same rounded material and 200 ms popup motion as the other menus.

Dialog palette changes remain live. Flat monochrome command icons follow their
button text color in both themes, while colored application icons are preserved.
Other Application opens a Gwenview-owned KOpenWithDialog, then passes the chosen
application and original URLs to KDE's launcher. Cancelling never launches it.
External applications, browser links and system services keep their own UI.

Run the menu/dialog walkthrough against disposable images and configuration:

```sh
python3 tools/verify-gwenview.py --dialog-preview
QT_SCALE_FACTOR=1.25 python3 tools/verify-gwenview.py --dialog-preview --dark --transparent
```

The walkthrough inventories all exported menu actions; opens file operations,
Properties and its pages, deletion confirmation, resize, annotation, tags, print
and expanded print options, shortcut and toolbar editors, preferences, About,
bug-report and language dialogs, and Other Application. It also checks nested
advanced permissions, inline editing tools, command search, pill/page motion,
keyboard access, compact layouts, reduced motion, and Apply/Cancel persistence.
Deletion and submission prompts are cancelled. Browser links and launching other
applications are intentionally excluded from this GUI test. Existing popup and
real Plasma global-menu tests remain available via `--popup-preview` and
`--global-menu`.

The file picker's Places splitter gives all window-resize stretch to the file
list. This prevents the transient sidebar compression that occurred before KDE
restored its saved width. Manual divider adjustment and saved widths are retained.
The dialog walkthrough checks intermediate resize events as well as final sizes.

Create New Folder preserves full-color icon artwork at the actual chooser size,
so compact symbolic variants cannot turn the color choices monochrome. Tahoe's
archive and bookmark choices use matching blue folders with a silver zipper and
gold ribbon. Their explicit symbolic variants remain available to small controls.
Run `SEQUOIA_DIALOG_FOLDER_ONLY=1 python3 tools/verify-gwenview.py --dialog-preview`
to check every idle choice, hover/leave, and creation with the selected icon.
Dialog previews load the repository's Tahoe icon theme as well as its palette.

Properties and other dialog pill selectors also paint Qt's overflow arrows as
separate capsules, with 100ms hover/press feedback and palette-aware disabled
states. Native scrolling, auto-repeat and right-to-left layout remain intact.

The Properties filename editor grows and shrinks with its text, leaving unused
row space empty and allowing long names to scroll within narrow windows. Page
changes fade the live contents over the existing dialog material instead of
painting a cached page over an opaque Window-color rectangle.

### Weather styling parity

Gwenview uses the Sequoia Weather overhaul as its presentation reference. Shared
values live in `lib/sequoiadesign.h`: 24px outer surfaces, 22px inset cards, 12px
controls, 13px rows, 13% text-color borders, and 100/200ms feedback/transitions.
These correspond to WeatherSurface.qml, LocationSurface.qml, WeatherAction.qml,
and settings/SettingsComponent.qml in the Weather source.

All native KDE dialogs receive a rounded, theme-aware shell with an independently
centered 18px title and Weather's red close control. The shell preserves dialog
modality, keyboard access, movement and edge resizing. The native file-picker
bridge is left intact; its actual KDE window receives the shell. Settings keep
all KConfig controls, with section headings outside centered 22px cards.

An app-local widget style applies the same controls, focus rings, switch-shaped
checkboxes, menu selections and hover feedback without modifying the desktop
style for other applications. Browser/sidebar/viewer/thumbnail surfaces, editing
panels and fullscreen/HUD controls use the shared radii, flatter materials and
softer shadows. Native image pixels and file operations remain unchanged.

Validation uses the dialog/menu walkthrough in both themes, native Wayland popup
checks at 125%, file-picker resize/reopen checks, and normal/fullscreen previews.

Dialog shells keep their material palette separate from transparency handling:
no transparent QDialog stylesheet may overwrite the Window brush. Resize cursors
reset when entering child content or leaving the dialog. Dialog button-box rules
and HLine separators are suppressed locally. Find Action remains a normal child
overlay and never clears the main window backing surface.

Configure Language presents the original KDE language actions in a bounded native
list (up to 360 logical pixels), with wheel/keyboard navigation and hidden scrollbars.
Inline editing tools use padded 22px cards separated from the image by an 8px gap.
The dialog walkthrough verifies painted background alpha, cursor reset, absence of
separators, bounded language navigation/selection, and the editing-panel padding.
