# Power Management

Power mode is the primary card: current mode, a short explanation and explicit
Power Save / Balanced / Performance choices. Unsupported or inhibited choices
remain disabled. Selection follows the native backend, including failed requests
and external changes. A separate Keep awake card explains that the existing
manual inhibition blocks both automatic sleep and screen locking.

`PopupDialog.qml` connects the view to the existing battery applet signals.
Native BatteryItem delegates retain charge state, unknown levels, health and
remaining time. Native application inhibition rows retain Allow/Unblock and lid
warnings. Settings and Energy info use the existing authorized KDE launchers;
percentage-on-icon remains available through the applet's contextual actions.
The shared tray sizes this page to its content, capped at 660 px / screen space,
with scrolling for longer content and a fixed action footer.

`tools/control_center_power.py` integrates these files into both the overlay and
native widget builds. `tools/install-control-center.py --stage --prefix PATH`
prepares a disposable overlay; `tools/build-widget-forks.py --destination PATH`
builds distributable widget packages. The native plugin and power services are
unchanged.

Run `python3 tools/verify-power.py` for interaction tests and a light preview.
Use `--render-only --dark --large --rtl` to inspect the large-font mirrored layout.
The tests cover external state, unsupported/inhibited profiles, failed profile and
sleep requests, native action dispatch, narrow layouts and the fixed footer.
Native tray integration was also checked in a private D-Bus session with populated
battery and application-inhibition fixtures, then inspected on the live desktop.
