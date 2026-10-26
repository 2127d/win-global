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
controls, with inner padding and edge-to-edge fullscreen. Its folder header exposes
native Sort, View, and Filters menus; header actions collapse to icons in smaller
windows. Rounded thumbnail cards distinguish hover, selection, and keyboard focus
without replacing Gwenview's thumbnail generation or file operations. The native
location bar collapses breadcrumbs and still supports F6 editing. Filter cards
stack their fields when narrow and scroll independently in a bounded area. Empty
folders and filtered-out results explain how to continue. Oversized thumbnails
temporarily fit the viewport and regain their preferred size when space returns;
only the preferred size is saved. Panel gutters are slightly tighter, with 10px
between the viewer and thumbnail strip and a 6px floating-control gap.

Fullscreen image controls share a rounded floating surface with a soft shadow,
a single-line image description and count, and separate navigation and image
control groups. The groups stack in a narrow image pane. Native previous/next,
slideshow, zoom, fit, actual size, rotation, sidebar, Browse and exit actions keep
their shortcuts and enabled states. Icons follow the fullscreen palette, and
thumbnail previews have rounded corners and visible selection borders. The
optional thumbnail row spans the panel, with its preferred height bounded by
available screen space. A pin keeps controls visible; otherwise the top edge
reveals them, and keyboard focus, menus and dialogs protect ongoing interaction.
Fullscreen exit cancels hiding and restores the cursor, including rapid toggles.

The sidebar has a live recent-image count and an overflow menu for choosing a
thumbnail grid or a list with filenames and parent folders. That view preference
survives restarts. The same menu exposes removal of the selected history entry
and clearing history; these commands do not delete image files. Grid tiles and
list rows have distinct hover, selection, and keyboard-focus states.

Three equal footer tabs show their icons above **Recent**, **Information**,
and **Tools** labels when space permits, switching to centered icons with tooltips
when the sidebar is narrow. The Recent history symbol is drawn independently of
the installed icon theme, and shortcut mnemonic markers are never displayed. The selected tab has a softly raised capsule, with a visible
keyboard focus ring. The sidebar anchors the footer while the page content scrolls. Before opening an image, Information
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

The top-right pill expands left and shares the native toolbar's actions,
including their menus, shortcuts, and enabled states. It follows toolbar
customization while omitting layout-only spacers. Floating icon buttons hide
the extra dropdown-arrow indicator while keeping their menus available. Start and Browse keep KDE's
normal toolbar visibility preference and hide the image-view pill. Returning to
image view restores the pill collapsed if the native toolbar remains hidden.
Fullscreen hides the main toolbar and its pill; leaving fullscreen restores the
previous choice. Spotlight retains its floating controls and restores the
previous bars when leaving that mode.

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
The expanded panel matches the toggle's 44-pixel height and circular end radius.
The panel and toggle share a blurred glass finish with a soft vertical
sheen and a highlighted rim, adapting to light and dark palettes. The image
scene behind both pieces is sampled and blurred separately from the controls,
refreshing as images, zoom, or the viewing area change. Controls have consistent
gaps. Buttons use opaque theme-colored fills and defined borders, with
accent-colored selected states and distinct hover and disabled appearances.
Buttons use inset pill corners to follow the toolbar capsule; the zoom track
and thumb are rounded as well. The editable zoom dropdown, arrow, and popup
use the native system style. Toolbar text, control surfaces, and monochrome
icons refresh on system theme changes without restarting, including disabled
and selected states and an already-open zoom menu. Repeated clicks and mid-animation reversals preserve the slide;
disabled desktop animations switch directly to the requested state.
Zoom, Fit, image count, sidebar, thumbnails, and comparison synchronization use
the existing controls. Opening the toolbar does not resize the image. A narrow
pane can scroll the controls horizontally. Checking **Show Status Bar** moves
the same controls back below the image and removes the pill styling, reserving
normal layout space for the native bar.
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
