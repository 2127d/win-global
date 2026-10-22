# Sequoia widget archives

The release inventory is `dist/CONTENTS.txt`. `dist/sequoia-widgets.tar.gz`
contains an exact copy of `dist/widgets/`: eight widget packages, the quick
installer, instructions, and checksums. The five panel widgets below use `.tar.gz`;
Analog Clock, Digital Clock and Grid Test use `.plasmoid` in both locations.
The included installer handles both formats. Every release archive has
a SHA-256 sidecar. `python3 tools/build-dist.py` refreshes all distribution
packages; `python3 tools/verify-dist.py` checks freshness, integrity, compatibility,
and installation of the three plasmoids into a temporary package directory.

The widget bundle includes widget packages only. Guided preset selection and
desktop snapping also require **Plasma style** from the complete installer.
The native widget export command described below remains a separate workflow
for its original five panel archives.

## Analog Clock

**Sequoia Analog Clock** offers two 2×2 presets: **Sequoia** first, with a
24-logical-pixel rounded square matching the application window corners, and
**Original** second, using the active Plasma theme's round clock face. The dial,
hands and optional seconds hand retain KDE's artwork and native animation.
The new face follows the theme's View palette. Both presets keep the native live
clock, localized tooltip, time-zone label, and click/keyboard calendar popup.
Appearance settings also allow switching faces after placement. Resizing snaps
to the shared grid while retaining the selected face.

Sources are in `plasmoids/org.sequoia.analogclock`; build the standalone package
with `python3 tools/build-analog-clock.py`. It produces
`dist/widgets/sequoia-analog-clock.plasmoid` and its SHA-256 sidecar. On an
existing managed Sequoia installation, install the clock and matching picker with:

```sh
python3 tools/install-widget-picker.py --with-widget org.sequoia.analogclock
```

Reload Plasma, then search **Sequoia Analog Clock** in Add Widgets. The full
installer includes it under Desktop widgets. The package has a separate identity
from the stock clock; existing clock instances and layouts are preserved.

## Grid system test widget

**Sequoia Grid Test** exercises the new grid, guided placement, responsive presets,
and native resize handles. Source is in `plasmoids/org.sequoia.gridtest`; the
standalone package is `dist/widgets/sequoia-grid-test.plasmoid`. See the
[grid and picker instructions](../desktop/widget-picker/README.md) to install
the matching shell integration and run isolated Plasma tests.

For a fresh computer, the main installer can build and install the widgets with
their dependencies. Select **Desktop widgets** in `install-gui.py`, or run
`./install.sh --components widgets` from the project folder. Global Theme also
includes them. This path preserves your existing panel configuration and tracks
widget files for removal with `./uninstall-gui.py`.

The prebuilt archive workflow below requires matching KDE/Qt versions.

## Packaged animated task manager

[`icons-only-task-manager-animated.tar.gz`](../dist/icons-only-task-manager-animated.tar.gz)
and its SHA-256 sidecar are stored in `dist/`. The extracted package lives in
[`widgets/plasmoids/org.sequoia.icontasks.animated/`](plasmoids/org.sequoia.icontasks.animated/).
It contains version **2.1.1** of
`org.sequoia.icontasks.animated`, copied on October 19, 2026 from
`~/.local/share/plasma/plasmoids/org.sequoia.icontasks.animated/`. This user-installed
copy includes newer animation changes than the system-wide copy found in
`/usr/share/plasma/plasmoids/`.

The archive preserves all 48 package files, including editable QML, configuration,
the compiled native adapter and its source, license notices, and the compatibility
manifest. It targets x86_64 and Qt 6.11.2; the installer also verifies the KDE
task-manager backend fingerprint. A SHA-256 sidecar verifies the archive itself.

The archive installer scans every `.tar.gz` in its input folder, so use a
temporary folder to keep this widget separate from the theme archives in `dist/`.
From the project root, validate or install this packaged widget with:

```sh
(cd dist && sha256sum -c icons-only-task-manager-animated.tar.gz.sha256)
widget_stage=$(mktemp -d)
cp dist/icons-only-task-manager-animated.tar.gz "$widget_stage/"
python3 tools/install-widget-archives.py --folder "$widget_stage" --check
python3 tools/install-widget-archives.py --folder "$widget_stage"
rm -r "$widget_stage"
```

Installation requires sudo for system files and restarts Plasma. Existing task
manager instances remain in place; add **Icons-Only Task Manager Animated** through
**Add Widgets**. The native adapter requires the supplied archive installer;
Plasma's local-file widget installer is insufficient.

## Full collection export

`~/sandbox/widgets` contains exactly these six files:

- `downloads.tar.gz`
- `separator.tar.gz`
- `icons-only-task-manager-animated.tar.gz`
- `system-tray.tar.gz`
- `start-menu.tar.gz`
- `install-widgets.py`

## Install

Copy the folder to the destination account, open a terminal there, and run:

```sh
cd ~/sandbox/widgets
python3 install-widgets.py
```

The script scans the **current directory** for widget archives, validates their
IDs, entry points and compatibility, and asks for sudo to install the package
files system-wide. It installs packages under `/usr/share/widgets/plasmoids/`
and the included native adapters under Qt's system `plugins/plasma/applets/`
directory. It then selects the unified notification fork for your account and
restarts Plasma. No compilation or source checkout is needed for installation.
Do not start the script with sudo: it handles elevation and keeps desktop
settings and the restart in your own session.

Your existing System Tray and Icons-only Task Manager keep their IDs, settings,
positions and pinned applications. Add **Sequoia System Tray** and
**Icons-Only Task Manager Animated** yourself through **Add Widgets** when ready.
**Downloads Stack**, **Dock Separator**, and **Sequoia Start Menu** are also available there.

The Start menu has pinned apps, activity-based recommendations, Category/List/Grid
views, search, and button-driven page animations. Its gear opens layout, pinned-app,
and animation settings. Editable source lives in `widgets/plasmoids/org.sequoia.startmenu`;
see its [README](plasmoids/org.sequoia.startmenu/README.md) for details.

The Global Theme installer also supplies the pure QML widgets used by its
[optional desktop layout](../desktop/layouts/README.md), preserving separately installed
copies. It also builds and installs the native tray, trash and animated task-manager forks on the destination machine; the optional layout selects them.

The tray archive includes independently named forks of its styled detail widgets,
including Notifications, plus the styled Trash widget. The installer selects
**Sequoia Notifications** inside existing trays, preserves notification visibility
and disabled states, and keeps existing notification instance settings. The tray
fork selects its own styled children by default. KDE's original widget packages
and libraries are retained.

Previous system files are backed up under `/var/backups/sequoia-widgets/`.
Changed panel settings are backed up beside the user's configuration. With `--activate-forks`, existing tray, trash and task-manager instances are
migrated in place, including child settings, visibility and pinned applications.
The legacy shell override is retired only with this migration. Older
user-local copies of the same custom IDs are backed up outside Plasma's applet
folder so they cannot hide the system installation. The recognized legacy
Sequoia control-center service hook is also backed up and retired; original
widgets then use KDE's original presentation, and the custom presentation lives
in the new forks. Unrelated service overrides are retained.

## Compatibility

The native adapters reuse the installed KDE backends, while each fork has its
own ID, QML and configuration schema. There is no shell-wide preload. The tray
and task-manager archives contain compiled adapters for the build machine's
architecture and Qt/KDE versions. The installer checks Qt and upstream file
fingerprints **before making changes** and rejects incompatible archives.
These packages were built on Fedora 44 with Plasma 6.7.5 and Qt 6.11.2.
Rebuild the archives after relevant KDE updates or for another supported machine.
The pure QML Downloads, Separator, and Start Menu packages do not need native adapters.

Use the supplied script for the full collection: Plasma's local-file widget
installer alone cannot install the native adapter libraries in the other two
archives. Installing the theme/icons is separate.

Validate without installing:

```sh
python3 install-widgets.py --check
```

Stage system files without sudo or changing a desktop:

```sh
python3 install-widgets.py --prefix /tmp/sequoia-widget-stage
```

## Development

The editable source and build tools remain in `win-global`. Rebuild and replace
the export with:

```sh
python3 tools/export-widgets.py --destination ~/sandbox/widgets
```

The old export is preserved as a sibling timestamped backup. The exported folder
contains the five archives and the installer. Each archive includes its
editable QML, native adapter source where needed, and license notices.
`tools/build-widget-forks.py` builds the archives; `tools/install-widget-archives.py`
is the source of the exported installer. `widgets/widget-forks/native-factory.cpp`
provides the normal applet factory used by the native forks. The customization
sources and checked upstream transformations remain under `desktop/control-center/`
and `tools/control_center_*.py`.

Build dependencies are the existing theme/widget development dependencies,
including PySide6, Qt 6 development tools, a C++ compiler and KF6 CoreAddons
headers. Installation needs Python 3.12 or newer, Plasma 6, `qtpaths6`, sudo and
a systemd user session.

Verify staging, notification configuration, fork loading, task settings and
notification delivery:

```sh
python3 tools/verify-widget-archives.py --folder ~/sandbox/widgets --functional
```

KDE source files retain their copyright and SPDX notices; license texts are
included in each derived package. Sequoia's original additions are MIT licensed.
