# Sequoia Modal Dimming

An optional native KWin effect for Plasma 6.7 and newer. This is a desktop effect,
independent of the decoration, Animation kit, and Weather makeover.

For embedded popups, the application shades/blurs its own content and reports the
current overlay opacity. The effect shades only the server-side decoration, leaving
the popup, client pixels, window shadow, layout and input handling intact. Weather
is the first integrated application. Compact auxiliary pages do not report a modal.

For separate native modal windows, KWin's modal-parent relationship is used. If KDE's
`dialogparent` (or legacy `dimparent`) effect is loaded, it retains responsibility;
Sequoia does not apply a second parent dim. Otherwise Sequoia animates the parent to
30% dim using InOutQuad, 160 ms scaled by KWin's AnimationDurationFactor, including
zero-duration/reduced-motion settings. Embedded popup timing follows the app's
actual opacity, including the exit animation, rather than starting another timer.

## Installation and removal

Select **Modal dimming** in the installer. It is unchecked by default, including
with the complete desktop. Sources are included in the installer archive; the plugin
is compiled against the destination's KWin development package. Installing or
removing the native plugin needs administrator authentication. KWin is not restarted.

```sh
python3 tools/install-theme.py install --components modal-dimming --skip-restart
python3 tools/install-theme.py uninstall --components modal-dimming
```

The shared plugin uses the existing theme installation receipt. Only its own entry
in `[Plugins]` in `kwinrc` is changed. The original value is saved under
`$XDG_DATA_HOME/sequoia-eleven/modal-dimming/activation.json`; reinstalling preserves
that baseline. Removal restores it while preserving other settings and a later
explicit user disable. `--keep-system` removes activation but retains the shared
plugin for other accounts. Rebuild the plugin after KWin upgrades if its ABI changes.

## App integration (Wayland)

Link `client/` as a CMake subdirectory and create a `SequoiaModalBridge` owned by the
app's `QWindow`. Expose a notifying `sequoiaModalAmount` real property on that window:
zero when no embedded modal is showing, otherwise the black overlay's opacity
(including fades; multiple overlapping modal layers should report their effective
combined opacity). Weather uses a maximum because its dialogs replace one another.
The allowed range is 0–0.6. The app owns blur and input blocking.

The session-bus API is `org.sequoia.ModalDimming`, object
`/org/sequoia/ModalDimming`, interface `org.sequoia.ModalDimming1`, method
`SetDecorationDim(uint wlSurfaceId, double amount) -> bool`. The effect obtains the
sender's PID from D-Bus and matches it against the Wayland connection credentials
and surface ID. It rejects other processes and ambiguous multiple connections.
No title matching, active-window guessing, persistent window IDs, polling daemon,
or global title-bar property is used.

Updates are asynchronous and coalesced; the bridge retries briefly if a new surface
is not mapped yet and republishes after effect reload. Closed windows, disappeared
D-Bus owners, and effect unload clear compositor state. Without the effect, Weather
continues to use its existing overlay. Embedded-popup reporting is currently Wayland
only. Other applications need an equivalent integration; KWin cannot infer a modal
item inside an arbitrary app. Client-side decorations remain the app's responsibility.
During overview or rotated-window effects, embedded decoration dimming is omitted
so those effects can own their composition.

## Verification

Build with `-DSEQUOIA_BUILD_TESTS=ON` to include the probe, then run
`python3 tools/verify-modal-dimming.py`. The test uses a private D-Bus session,
private XDG directories, and a virtual Wayland compositor. Screenshots are actual
compositor output: individual-window screenshots bypass effects and are unsuitable
for checking dimming. Test alternate decorations with `--decoration`.

`python3 tools/verify-modal-installer.py` checks independent selection, file ownership,
reinstall/remove behavior, failed activation, and preservation of unrelated settings.
