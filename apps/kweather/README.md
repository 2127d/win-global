# Sequoia Weather

A presentation fork of KDE KWeather 26.08.1, with its matching KWeatherCore
engine bundled privately. The installed `org.kde.kweather` Flatpak remains the
fallback application. The source archives are identified in `override.json`;
`upstream.sha256` matches the digests in that Flatpak's build manifest. Original
copyright notices, licenses and translations remain in both source directories.

The forecast dashboard provides a location sidebar, current conditions, hourly
forecasts, selectable days with temperature ranges, precipitation, humidity,
pressure, UV index, sunrise, sunset and moon phase. Location search, adding,
reordering and deleting locations, unit preferences and About use KDE's existing
models and dialogs. Hover over an hour for its condition and wind speed.

With no saved locations, a centered welcome screen presents a weather illustration,
a short introduction and an accent-colored Add location button. It replaces the
first-launch wizard; Settings remains accessible for appearance and units. The
same screen returns when the last location is removed and follows Flat/Dynamic
styling, including the system palette and transparency in Flat mode.

Settings uses rounded, padded cards with soft shadows and the same animated
switch indicator as `themes/application-style/quick/SwitchIndicator.qml`, bundled
at build time. The switch retains native mouse, keyboard and accessibility input;
the existing unit selectors and persistence remain unchanged.
Desktop Settings, Add Location and Locations dialogs have centered titles and a
red traffic-light close control on the left; its cross appears on hover or keyboard
focus. Desktop About keeps only its back button, returning to Settings to close.
Compact pages retain their existing headers and back navigation.
Location search and management use rounded dialog surfaces, plain search result
rows inside one shared card and padded saved-location cards, accent-focused search
fields and rounded action buttons. The results card spans the search field and
button together, matches the field's corner radius, and scrolls independently
beneath the fixed controls. The
translucent search dialog blurs the underlying app with Qt Quick effects while
keeping its own controls sharp; closing it releases the blur layers. Search has
no header or footer separator, including when results overflow. Location
selection, drag ordering and deletion continue to use the native models.

Dynamic mode uses a layered sky with drifting clouds, sunlight, moonlight,
stars, rain and snow. Motion stops when the window is minimized, when Flat mode
is selected, or when animations are disabled in Settings. Global zero-duration
animations are also respected. The scenery never owns mouse input.

Flat mode uses the system Sequoia palette and rounded inset surfaces. Its window
retains the palette's alpha: opaque for Solid and compositor-blurred for
Transparent. The same dashboard adapts to narrow windows, where locations move
into the native location picker. Both styles can be switched without losing the
selected location or day. New loading and error feedback retains available data
when a refresh fails; absent data is shown as unavailable rather than zero.

Action controls use a private bundle of the Sequoia Tahoe Material Symbols Rounded
artwork, including Settings, About, search, clear, close, back, add, reorder and
delete. The build generates these resources from the shared icon sources, with
their Apache license and checksums; it never substitutes application identities or
weather illustrations. Explicit resource URLs keep controls consistent across icon
themes. The Flat/Dynamic selector uses a raised pill surface with a soft shadow and one
selection capsule that slides and resizes with cubic easing. Its motion follows
KDE animation duration (including disabled animations), supports rapid reversal,
and retains keyboard activation and mutually exclusive selection. Refresh and
Settings use matching circular floating surfaces; forecast cards have soft shadows
behind their rounded backgrounds. Small forecast icons use white foreground ink
in Dynamic mode and system text color in Flat mode, keeping both light and dark
themes legible. The large current-weather illustration retains its colors.

## Build and install

```sh
python3 tools/build-kweather.py
python3 tools/verify-kweather.py --binary .staging/kweather-release/install/bin/kweather
python3 tools/install-kweather.py --skip-build
```

An optional `--sdk /path/to/usr` supplies additional development headers. Builds
require Qt 6.9 or newer, KDE Frameworks 6.26 or newer, ECM, Kirigami Addons,
Qt Quick/Controls, Charts, Positioning, SVG, OpenGL and the KDE libraries declared
by the two CMake projects. On Fedora the makeover is also selectable as
**Weather makeover** in the installer, which installs those dependencies. It is
included in the portable installer sources.

The runtime is installed to `~/.local/share/sequoia-eleven/kweather/`, with a
user launcher and desktop entry for Weather. It uses the host Sequoia styling.
On the first user installation only, existing Flatpak weather preferences and
cached forecasts are copied into native locations if those destinations do not
already exist. Existing native preferences are never overwritten. Future changes
are independent of Flatpak preferences. Uninstallation keeps locations and
preferences and restores any pre-existing user launcher/desktop entry:

```sh
python3 tools/install-kweather.py --remove
```

The launcher checks both the stock Flatpak binary and private runtime payload.
After a Flatpak update, or if the private payload is modified, it opens the
Flatpak until the makeover is rebuilt. The source and installer match version
26.08.1; update the fork and manifest deliberately for later releases.

## Verification

`tools/verify-kweather.py` runs the actual executable with private XDG directories
and a private session bus. Forecast fixtures are generated locally, so captures
in `preview/kweather-*` are interface tests, not live weather reports. Checks
cover forecast loading, unit conversion, day and location selection, native
dialogs, style switching, window alpha, motion preference, responsive sizing and
refresh, including failed refreshes that preserve the last forecast. `--empty`
checks startup without data. `apps/kweather/tests/installation.py` checks runtime
relocation, update fallback, edited-file protection and reversible installation.
Locations use distinct temperature ranges to catch stale forecast rows after
switching cities. Bar geometry is checked at wide and narrow sizes, including
a zero-range day at the upper endpoint of the temperature scale.
`--welcome` checks first launch without saved locations, both welcome styles,
Settings access and the location-search action at wide and compact sizes.
Use `--mode light|dark --material solid|transparent` to test each palette.
Use `--icon-theme SequoiaTahoe-light` to check the installed Tahoe weather symbols.
`--rendered` checks Settings and About on the live Wayland renderer with isolated
data. About shares Settings' theme-aware popup surface and blur, uses a sliding
fade transition with KDE's animation duration, and preserves Settings on return.
It displays upstream version, attribution, licenses and library data from KDE's
AboutData APIs. Compact windows use the same About content in the page stack.
`--compact` checks Settings, About, Manage Locations and Add Location at 390×480,
including real Back and Add buttons, long search results, scrolling to the last
setting, selecting a saved location, and resizing up to the compact breakpoint.
Use it with `--rendered` to check the theme-aware scenery blur behind these pages.
`--choices` exercises inline Forecast Style and unit selectors, including animated
card expansion, stable popup geometry, saved values, keyboard navigation and
single-selector expansion. Combine it with `--compact` for the smallest layout.
The runtime does not contain the test fixtures or test hook.

The optional **Modal dimming** desktop effect coordinates the title bar with Weather's
embedded popup fades. Its reusable Wayland bridge is linked from
`desktop/effects/modal-dimming/client`; Weather works normally when the effect is
absent. Compact Settings/About/location pages do not dim the title bar. The effect
is installed and removed independently of the app makeover.

Modal composition uses one shared scrim whose exact opacity is also reported to
the decoration, including overlapping dialog transitions. The flat palette plane
is never blurred; only the scenery and foreground content are sampled for blur.
Scenery extends beyond the viewport so blur sampling cannot thin the window edge.
The native window alpha/blur contract stays fixed across Flat/Dynamic switches and
changes only with the selected theme material. `--rendered --compositing` checks
edge color and alpha, rapid style switching, resizing and dialog handoffs.
