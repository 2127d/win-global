# Jungle Vines

A KWin desktop effect with an original vector monkey swinging from two leafy
vines, inspired by the supplied animation. The canopy hangs at the top of the
primary display, with swaying background vines, a curling tail and an occasional
blink. The artwork has a transparent background and scales with the output.

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
30 frames per second, within a maximum 920 × 396 logical-pixel canopy. Neither
the supplied GIF nor a solid background is used as a desktop overlay.

Verification uses a private KWin session and real pointer input:

```sh
python3 tools/verify-vines.py --output /tmp/vines-check
```

It checks changing composited frames, click-through, focus retention, the actual
global shortcut, fullscreen hiding, unload cleanup, and reload. `render-vines`
also exports a cycle of transparent frames for visual inspection.

Source and original artwork: GPL-2.0-or-later.
