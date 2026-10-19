# Sequoia Start Menu

A Plasma 6 launcher modeled on the supplied Windows 11 Start screenshot, using the installed Linux applications and current icon theme.

The main page has search, a six-column pinned grid, recommendations, an All apps view selector, category cards, and an account/power footer. Category, List, and Grid are interchangeable views of the installed apps. Recommendations come from Plasma activity history; the empty state explains when no history is available.

The menu defaults to 740×800 pixels, with size controls in its settings. The launcher icon scales with the panel's available icon area. On horizontal panels the menu is centered on that screen, with a 3-pixel gap from the visible panel surface, including floating panels. Its dimensions shrink to fit smaller screens. Vertical panels place it beside the panel with the same gap. Saved custom sizes remain in effect when upgrading.

## Install

Editable source is stored in `widgets/plasmoids/org.sequoia.startmenu`. Run
`python3 tools/build-start-menu.py` from the repository root to refresh
`dist/widgets/start-menu.tar.gz`. The Global Theme installer includes this widget
for its optional desktop layout.

From `dist/widgets`, run `python3 install-widgets.py --check` to validate all archives, then `python3 install-widgets.py` to install the collection. The installer requests elevation for system installation and restarts Plasma. Add **Sequoia Start Menu** from Plasma's **Add Widgets** menu.

To install only this widget for the current user:

```sh
kpackagetool6 --type Plasma/Applet --install ./org.sequoia.startmenu
```

For an existing installation, use `--upgrade` instead of `--install`. Right-click the panel launcher to configure its keyboard shortcut, or assign it as the panel's application launcher using Plasma's normal controls.

## Customize and use

- Click the gear to open settings for menu size, launcher icon, pinned rows/columns, recommendations, and the default app view.
- Right-click an app to pin or unpin it and move it earlier or later. The **Pinned apps** settings page also provides ordering controls. Pins are stored separately for each widget instance.
- **Show all** and category buttons navigate with a leftward slide. **Back** also slides left, as requested. These are button-driven transitions, not touch gestures.
- Opening and closing use Plasma's desktop effects for the entire popup, keeping the contents, background, and shadow together. Launching an app shrinks its tile. The **Animations** settings page controls page-transition duration, launch feedback, and reduced motion, with a preview.
- Type to search installed apps, settings, indexed files, and recent documents. File results depend on the corresponding Plasma search providers.
- Escape clears search, returns to the main page, or closes the menu. Alt+Left returns to the main page. Tab/Shift+Tab and Enter activate controls without a mouse.
- Power actions use Plasma's session backend and its normal confirmation behavior.
- The View menu opens 4px below its control at the same width. Power opens 4px
  above its button, aligned to the button's right edge. Both use native Plasma
  surfaces for themed rounded corners, shadows, transparency and compositor blur.
  Arrow keys select an entry; Enter activates it and Escape returns to the button.

## Build and verify

From the repository root:

```sh
python3 tools/build-start-menu.py
python3 tools/verify-start-menu.py
python3 tools/verify-start-menu-native.py
python3 dist/widgets/install-widgets.py --folder dist/widgets --check
```

The functional verifier needs PySide6, Plasma 6's Kicker QML module, `kbuildsycoca6`, and permission to create a private D-Bus socket. It uses disposable configuration/data directories and launches only a fixture that creates a temporary marker file. It also saves `preview/start-menu.png` and separate View/Power menu captures. The native verifier additionally needs `kwin_wayland`; it runs a private virtual display and verifies compositor blur-region requests without changing the current desktop.

This package uses Plasma's private Kicker models, as Plasma's own application launchers do; it should be rechecked after major Plasma upgrades. It contains no compiled native adapter. The popup background, blur mask, shadow, colors, and controls follow the active Plasma theme. The account photo is circular, with a user-icon fallback when no photo is available.

The home page stays instantiated across ordinary open/close cycles. Other pages
reset after the popup hides. Hidden app views do not create tiles, limited
collections load only their displayed tiles, and category cards load at most
four preview icons. The verifier checks these limits with a 500-row fixture and
exercises immediate native-window dismissal and repeated toggles.
