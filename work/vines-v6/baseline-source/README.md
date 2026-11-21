# Jungle Vines

A KWin desktop effect with an original vector monkey swinging one arm at a time across leafy
vines, inspired by the supplied animation. Vines unfurl along the full width of
the primary display, directly below its top panel. The monkey enters from the
left, swings between vines anchored to the panel, exits completely past the right edge,
then the vines retract into the panel. They descend again before his next visit.
The monkey always skips two intervening vines, catching the third, with a fixed
0.82-second cadence. Both arms circle counterclockwise from the trailing grip,
down and forward into the next catch. A brief two-hand overlap transfers the
load before the trailing hand releases. Position, body pitch, hand velocity,
and elbow curvature stay continuous through each transfer.

Untouched vines hang straight. Only contact starts their swing; they never lean
toward an approaching hand. Released vines settle with a damped swing. The
artwork remains naturally proportioned across screen widths, with a curling
tail and an occasional blink.

The canopy explicitly opts out of blur and rejects later internal-window blur
requests. Empty pixels show the unchanged desktop, including with Sequoia's
application style and either Liquid Glass or standard KWin Blur.

**Meta+Ctrl+M** toggles the canopy. It is also listed as **Jungle Vines** in
System Settings → Desktop Effects. The effect automatically hides during
fullscreen applications, fullscreen compositor effects, and screen locking.
Mouse input passes through to applications; the effect never grabs the keyboard
or changes the active window. Its animation timer stops while hidden.

```sh
python3 tools/install-vines.py install
python3 tools/install-vines.py status
python3 tools/install-vines.py remove
```

Installation builds against the installed Qt 6 / KWin development libraries,
places the effect under the user's XDG data directory, and activates it without
restarting KWin. It records replaced files and the prior activation setting for
removal. Rebuild after upgrading KWin. Native QML modules remain cached inside
KWin: after changing an already-loaded C++ renderer, log out/in to load that new
binary; toggling and unloading the existing version work immediately.

The KWin declarative package owns a transparent internal Qt Quick window and a
small QML extension. It deliberately does not activate SceneEffect's fullscreen
view or input interception. The extension renders antialiased QPainter paths at
approximately 60 frames per second, within a full-display-width canopy at most 396 logical pixels tall. Neither
the supplied GIF nor a solid background is used as a desktop overlay.

Verification uses a private KWin session and real pointer input:

```sh
python3 tools/verify-vines.py --output /tmp/vines-check
```

It checks forward travel, a complete offscreen exit and canopy retraction, vines at both screen edges,
unchanged background pixels, click-through, focus retention, the actual
global shortcut, fullscreen hiding, unload cleanup, and reload. `render-vines`
also exports a cycle of transparent frames for visual inspection. Add
`--stock-blur` to exercise standard Blur instead of Liquid Glass.

Revision 5 uses a separate QML module and effect ID so upgrading from revision 4
works without restarting KWin. The installer retires its owned previous revision and preserves a copy under `sequoia-eleven/vines-v4-upgrade-backup`.

The `test-vines-motion` executable checks continuous body/hand velocities,
counterclockwise free arms, overlapping grips, fixed two-vine skips, upright
untouched vines, constant vine length, reachable catches, and canopy timing
across screen widths. `render-vines OUTPUT [START DURATION FRAMES]` exports
transparent frames for checking the movement at playback speed.

Source and original artwork: GPL-2.0-or-later.
