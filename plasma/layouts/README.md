# Bundled desktop layout

All six Sequoia Eleven Global Themes include the desktop layout captured on
2026-10-06. Select a theme in KDE System Settings → Colors & Themes → Global
Theme, enable **Desktop layout**, and apply. KDE replaces the current panels and
desktop widgets when this option is selected. Applying appearance alone preserves
the current layout. Installing the files does not activate or replace anything.

The preset creates these panels on the primary screen:

- A 34px, full-width, non-floating top bar: Kpple Menu, margin separator,
  Window Title Fork, Sequoia Desktop Menu, global menu, expanding spacer, Color Picker, System Tray,
  two margin separators and the bold 11pt clock with its custom date format.
- A 64px, centered, content-fitting floating dock that dodges windows: Sequoia
  Start Menu, margin separator, Icons-only Task Manager with the saved launcher
  order, Dock Separator, Downloads Stack and Trash.

The Global Theme component supplies the six required nonstandard pure QML
widgets. Copies already installed by the user are kept and remain outside the
theme's uninstall receipt. Bundled copies installed by the theme are tracked and
removed with it. Stock KDE applets come from the destination's Plasma installation.
Optional tray integrations and pinned applications must be installed separately;
the layout does not install applications. Native Sequoia tray/task-manager styling
still requires the separate widget/overlay installation.

Downloads uses the destination account's XDG Downloads directory. The desktop
uses the Global Theme wallpaper; personal images, weather location, activity IDs,
screen connectors, stale applets and configuration-dialog geometry are excluded.
Additional screens receive desktops without extra panels. This is a saved preset,
not a continuously updated copy of the source user's desktop.

`desktop-layout.json` holds the reviewed panel/widget settings;
`desktop-layout.js.in` implements the Plasma scripting API. After editing either:

```sh
python3 tools/build-desktop-layout.py
python3 tools/build-desktop-layout.py --check
python3 tools/verify-desktop-layout.py
python3 tools/verify-desktop-layout.py --native
```

Each generated `contents/layouts/org.kde.plasma.desktop-layout.js` is standalone,
so installed Global Theme packages do not need access to this source checkout.
The packaging follows [KDE's Global Theme layout convention](https://develop.kde.org/docs/plasma/theme/theme-porting-to-plasma6/).

Third-party widget sources were copied from the installed versions: Kpple Menu
1.6.5 ([upstream](https://store.kde.org/p/2138251/), GPL-2.0) and Window Title Fork
0.7.1-1 (authors and upstream recorded in its metadata, GPLv2). Their original
source, authorship and license notices are retained; only the explicit
`KPackageStructure` metadata field was added for package validation. Start Menu
1.0.2 was restored from this project's existing `dist/widgets/start-menu.tar.gz`.

The Global Theme installer builds the Sequoia System Tray, Trash and animated task manager with their native adapters. The preset selects those custom IDs and their styled tray children. Apply the desktop layout only when you want to replace the current panel layout.
