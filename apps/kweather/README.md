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
Use `--mode light|dark --material solid|transparent` to test each palette.
The runtime does not contain the test fixtures or test hook.
