# Sequoia Activity Bar

Desktop activities form one centered group inside a pill. The complete footprint
is at least 2×1 cells on Sequoia's 96 logical-pixel grid, with a six-pixel inset.
Width rounds up to whole columns as activities are added, removed or renamed;
extra width is split equally around the group. Height accommodates the font.
Matching minimum and maximum sizes describe its fixed footprint. A gesture guard
on Plasma's existing handles blocks manual resizing before any geometry changes
and uses their existing red surface feedback. This also avoids the native handle
path's lack of maximum-size enforcement. The grid does not repair drag-induced
size changes after the fact.

The panel presentation keeps KDE's Activity Bar tabs, orientation, sizing and
activity model. Mouse and keyboard switching, Activity Manager and Configure
Activities remain connected to KDE's installed backends.

The desktop requires Sequoia's grid integration. `sequoiaAutomaticSize` on the
root asks the grid guard to apply changed size constraints outside Edit Mode,
including correcting old oversized geometry. Plasma manages collision placement,
occupancy and persistence. The standalone applet does not replace the KDE plugin.

Install through `python3 tools/install-widget-picker.py --with-widget
org.sequoia.activitybar`, then reload Plasma. The full theme's Desktop widgets
component also includes it. Add **Sequoia Activity Bar** from the widget picker.

Run `python3 tools/verify-activitybar.py` for isolated native Plasma checks:
desktop and panel loading, real mouse/keyboard activity switching, automatic
growth/shrink, long-name centering, occupancy and all eight blocked resize
handles. Signal spies require zero transient position or size changes during
inward and outward drags, including release.
`python3 tools/verify-widget-picker.py` covers shared grid regression behavior.
