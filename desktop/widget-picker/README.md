# Grid placement and widget picker

Sequoia's picker keeps Plasma's native catalog, filters, package installation,
drag source, panel handling and desktop resize handles. The shell adds a shared
96 logical-pixel desktop grid and a guided placement flow. Display scaling is
handled by Qt; cell dimensions do not change between widgets.

## Try it

On an existing managed Sequoia Plasma-style installation:

```sh
python3 tools/install-widget-picker.py --with-test-widget --check
python3 tools/install-widget-picker.py --with-test-widget
```

Log out and back in to reload the shell. Open **Add Widgets** and search for
**Sequoia Grid Test**. Installation updates the managed shell receipt, backs up
the prior picker files, and tracks the test widget under Desktop widgets.
The full theme installer also includes the test widget when selecting widgets.
The grid/picker belongs to Plasma style; removing that component restores the
stock shell while retaining independently selected shell layers.

Click a card (or press Enter) to see your current screens and panels. Selecting
a panel adds the panel representation. Selecting a desktop opens one preset at
a time, with Previous/Next and Use this preset. Click a free grid location to
place the selected footprint. Arrow keys move the preview; Enter places it;
Escape cancels. Existing widgets and reserved panel space prevent placement.
Every cell can be a starting position, so a 2×2 footprint is not restricted to
an arbitrary set of 2×2 blocks.

Dragging a card hides the picker window without destroying the native drag
source. Desktop drops use the first preset. Native panel drops use the panel
representation. Completion or cancellation brings the picker back.

Use Plasma's Edit Mode resize handles on the test widget. It snaps while resizing
and changes between compact, square, wide, tall and detailed layouts. Position
and size are saved by Plasma's existing layout manager. The grid uses its native
collision map rather than a separate database of desktop positions.

## Widget contract

The first conforming widget is `widgets/plasmoids/org.sequoia.gridtest`.
Its metadata declares `"X-Sequoia-Grid": "grid.json"`. That manifest specifies
version 1, minimum columns/rows, whether a panel representation is supported,
recommended presets, and a presentation-only QML preview. The first preset is
the drag default. The builder validates manifests before touching installed files.

A footprint includes the complete outer container: columns × 96 by rows × 96.
The test widget uses no Plasma background padding and draws its surface six
pixels inside each edge, giving neighboring widgets a consistent 12-pixel gap.
It keeps all content inside the footprint and has a separate panel presentation.
The preview and installed widget use the same QML face. Future widgets should
follow this contract and retain all their native functionality.

The current catalog gives legacy widgets a default footprint; they still retain
their native minimum sizes and presentation. Their full set of responsive
presets will be added as those widgets are redesigned. The first implementation
does not migrate widget identities or implement the separate restore/remove
workflow discussed for future replacements.

The integration targets the installed Plasma desktop shell and its
`AppletsLayout` API. Checked source transformations reject changed upstream
anchors. Refresh and rerun functional tests after Plasma updates.

## Build and verify

```sh
python3 tools/build-grid-test.py
python3 tools/verify-shell-overrides.py
python3 tools/verify-widget-picker.py
python3 tools/verify-widget-picker.py --width 800 --height 600 --mode light
python3 tools/verify-widget-picker.py --offset 1280
```

The functional test starts a private D-Bus session and virtual Wayland desktop
with temporary configuration. It never changes the live desktop. It checks the
destination map, preset placement, panel addition, occupied slots, native live
resizing, adaptive content, drag/drop, cancellation, and existing picker behavior.
It also restarts Plasma to check saved geometry. Compositor-driven drag tests
require libei development headers alongside the existing Qt development tools.
Test input is restricted to the temporary compositor's D-Bus and runtime directory.

`dist/widgets/sequoia-grid-test.plasmoid` is the standalone test-widget package.
Installing it alone does not install the shell's grid or placement wizard.
