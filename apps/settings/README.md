# System Settings navigation

This application's sources and launcher live in `apps/settings/`. Run the
commands below from the repository root; shared build and installation tools
remain in `tools/`.

This is a presentation fork of KDE System Settings 6.7.5. It replaces the long
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
in `systemsettings-v6.7.5/CMakeLists.txt`, plus CMake, Ninja, a C++20 compiler, ECM,
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

`systemsettings-v6.7.5/` was obtained from:
https://invent.kde.org/plasma/systemsettings/-/archive/v6.7.5/systemsettings-v6.7.5.tar.gz

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
