# Sequoia Eleven login screen

A Plasma Login Manager presentation with Sequoia forest wallpaper and a frosted
central account card. It contains the actual account avatar/name, a password field
with a lock and visibility control, a blue Sign In button and Switch User control.
The live 24-hour clock/date sit at the lower left. Labeled Sleep, Restart and Shut
Down buttons sit at the lower right; Restart retains the original icon shape in
white. The desktop-session selector sits in the top-right tray as a monitor icon;
keyboard layout selection remains at the upper left when available.

The top-right glass status island has compact, matching outline icons, hover/focus outlines
and a blurred popover. Network shows active connections and a Wi-Fi on/off control;
Sound provides mute and a 0–100% volume slider for the preferred output. Battery
shows the actual percentage, charging state and remaining-time estimate; the clock
opens the full date. Escape or clicking outside closes the popover and returns
focus to login. Controls are disabled while signing in.

The desktop-session menu slides down beneath its button using the same rounded
glass, centered close control and blue selection highlight. It reads Plasma's
native session model and updates the native saved selection used for login.
Arrow keys navigate the choices; Enter or Space selects, and Escape dismisses.

The island uses native NetworkManager, audio and battery models. Wi-Fi changes
respect the hardware switch and login-session permissions, with a message if a
request does not take effect. Audio controls are disabled when no output is
available, and the battery indicator is hidden on machines without a battery.
Time and date share a system-clock value
refreshed every second, including midnight rollover. Enter or Sign In submits the
form; Switch User or the avatar replaces only the central card with a frosted
account picker. The wallpaper, status tray, clock/date and power controls stay in
place. The picker uses actual account pictures/names, with initials when no picture
is available, and includes Other User for manual username entry. Click an account
to return to its login form with the password field focused automatically.
Passwordless accounts focus Sign In; Other User focuses the username field when
it is empty. Arrow keys and the side buttons navigate accounts, Home/End jump to
the ends, and Enter or Space chooses. Long lists scroll horizontally. Escape or
the close button returns to the previous form without changing the selected user.

## Blur

The card, status strip, power-control background and each circular power button
sample the wallpaper at their own screen coordinates. A native Qt Quick painted
item caches a downsampled, three-pass separable blur shared by all surfaces. Rounded
masks clip the result before tint/borders and sharp foreground controls are drawn.
Screen resizing and layout changes update the sampling rectangle. The sharp
wallpaper remains unchanged outside those masks.

The native renderer resolves relative wallpaper URLs against the QML component,
so the installed theme can load its bundled image for blur. Re-run `./install.sh`
after native renderer changes to rebuild and install the bridge.

This uses the same QPainter image path on software and accelerated Qt Quick,
without depending on compositor blur or shader effects that disappear in software
rendering. Decoded wallpapers and blurred surfaces are bounded caches, reused
instead of being regenerated each clock tick. The native helper does no file writes.

## Organization

- `qml/`: responsive presentation, avatar, power controls and native backend adapter.
- `assets/`: original SVG symbols. The wallpaper is staged from `wallpapers/sequoia-forest.jpg`.
- `native/`: greeter presentation bridge and cached wallpaper-blur renderer; no authentication implementation.
- `tests/`: QML interaction tests with a fake backend; power actions only increment counters.

## Installation

Run `./install.sh` from the project root as your normal desktop user. The normal
Fedora build dependencies include the Qt/C++ tools needed here, plus `plasma-nm`
and `plasma-pa` for native status indicators. Plasma Login
Manager must already be present; the installer does not replace SDDM or enable a
different display manager. This theme is not an SDDM package.

The unified installer adds root-owned QML, SVGs, wallpaper copies and a bridge
under `/usr/local/share/sequoia-eleven/login-screen`, the bundled Adwaita font,
and `/etc/systemd/user/plasma-login.service.d/60-sequoia-eleven.conf`.
Reboot after installation to load the service override. `./uninstall-gui.py` removes
these tracked files and restores any overwritten original files. `--keep-system`
retains the shared login theme as well as native application/decorations plugins.

The Sequoia forest wallpaper is used for both light and dark palettes. Existing optional
`tools/install-login.py` appearance synchronization continues to work, but remains
a separate experimental helper with its own backup/restore workflow. The main
installer does not change PAM, autologin, sessions, or `/etc/plasmalogin.conf`.

The bridge changes only the native greeter's main QML URL. Native user/session
models, authentication, saved selections, keyboard layout and authorized power
actions remain in Plasma Login. Passwords are kept in its existing transient
shared state and cleared on user changes, Escape, failure and success. No
credentials are written by this theme. Busy/error state is shared across displays.

The bridge checks the installed greeter's SHA-256 and falls back to the stock
screen after a greeter update, when theme files are missing, or when QML cannot
load. Re-run `./install.sh` after Plasma Login updates to rebuild compatibility.
The preload is scoped to the greeter service and cleared before launching children.

## Preview and verification

```sh
python3 tools/preview-login.py --mode light --width 1280 --height 720
python3 tools/preview-login.py --mode dark --width 1280 --height 720
python3 tools/preview-login.py --demo-status --status-panel audio --width 1280 --height 720
python3 tools/preview-login.py --user-switcher --width 1280 --height 720
python3 tools/verify-login-screen.py
SEQUOIA_FULL_INSTALL_TEST=1 python3 tools/verify-theme-installer.py
```

Previews use the actual installed greeter with `--test`, offscreen rendering,
temporary configuration, and stubbed power actions. Normal previews retain the
theme's relative wallpaper URL, matching the installed screen. `--demo-status`
uses simulated hardware only in the temporary preview and writes separately named
`login-status-demo` images. `--status-panel` opens network, audio, battery or clock
details; `--status-panel session` opens the themed desktop-session menu.
`--user-switcher` captures the central picker with the native account model. The demo
fixture is never installed with the theme. Tests cover password entry,
failure/retry, visibility, manual/passwordless login, account selection, avatars,
keyboard navigation, live time/date and midnight rollover, sleep/power availability,
blur strength/clipping/position and relative wallpaper URLs, sizes from 800×480 through 4K, fallback,
and installation/removal, plus status controls, keyboard volume changes, missing
devices, disabled actions, centered highlights, session selection with the native
test greeter, popover bounds and returning focus to password entry.
Network/audio actions are tested with a fake backend; the offscreen environment
has no live audio/system bus, so real hardware controls in the greeter still need
a session check. Real PAM authentication and a rebooted live greeter
still require a manual session check. No live services are restarted by these tools.

For blur diagnostics, `tools/preview-login.py --wallpaper login-screen/tests/backdrop.svg
--output /tmp/login-blur.png` replaces only the preview wallpaper with high-frequency
stripes. The card and power circles must soften the stripes and retain the correct
local background color. `--renderer opengl` exercises the accelerated path when a
working OpenGL context is available; the current test environment has none, so
live GPU rendering remains a manual check. Normal previews use software rendering.
