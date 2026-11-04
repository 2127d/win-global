# Display Configuration

This is the `org.kde.kscreen` layout/presentation applet, separate from the
brightness/color `org.kde.plasma.brightness` applet. The Sequoia package is
`org.sequoia.kscreen` and reuses KDE's installed native applet factory.

The layout card exposes every native `availableActions` entry with its original
label, icon and action value. Mirror comes first, extension choices are paired,
and single-output choices are paired. Unknown future actions remain visible.
Presets are disabled with fewer than two connected outputs, just as upstream.
No selection is highlighted because the backend does not expose an active preset.

Presentation Mode retains the PowerDevil data source and the native
`beginSuppressingScreenPowerManagement` / `stopSuppressingScreenPowerManagement`
operations. The switch stays disabled while a job is pending, and reports job
errors while restoring its prior state. All external inhibitions show their
application name and reason. The compact footer uses KDE's authorized configure
action for `kcm_kscreen`.

The page supplies intrinsic size hints to the tray; long app lists scroll.
`tools/verify-display-configuration.py` tests action forwarding, keyboard toggling,
job-busy UI, unavailable power service, single-screen restrictions, width alignment
and changing content heights using fixtures without changing physical displays.
Use `--dark --large --rtl --render-only` to render the alternate layout.
