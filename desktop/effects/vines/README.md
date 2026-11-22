# Jungle Vines

An optional KWin script with an original vector monkey swinging one arm at a time across leafy
vines, inspired by the supplied animation. Vines unfurl along the full width of
the primary display, directly below its top panel. The monkey enters from the
left, swings between vines anchored to the panel, exits completely past the right edge,
then the vines retract into the panel. They descend again before his next visit.
The monkey always skips two intervening vines, catching the third, with a fixed
0.8-second catch cadence and a 1.6-second whole-body cycle. Hips and feet lead
on one catch and trail on the next. The body drops deeply under the supporting
hand, the legs swing through, and the head keeps looking toward the next vine.
Comparison against the GIF led to a small torso/leg lag and an earlier low
reaching-arm sweep, while preserving the 0.8-second catch rhythm.
The supporting elbow bends with the load. Both arms circle from the trailing grip,
down and forward into the next catch. A 100-millisecond two-hand overlap transfers the
load before the trailing hand releases. Position, body pitch, hand velocity,
and elbow curvature stay continuous through each transfer.

The entry vine is already drawn backward and loaded when the monkey arrives,
as though released from an unseen launch point. Every subsequent vine hangs
upright until contact. The hand then pulls it about 25 pixels backward and
7–8 pixels downward while the trailing hand remains attached. The body is
positioned between the two grips from their reach geometry, then follows a
pendulum beneath the leading grip after release. Arm extension varies under
load; slight elastic stretch in the vine relaxes after release. Untouched vines
never anticipate a catch. Released vines settle with a damped swing. The
artwork remains naturally proportioned across screen widths, with a curling
tail and an occasional blink.

The canopy explicitly opts out of blur and rejects later internal-window blur
requests. Empty pixels show the unchanged desktop, including with Sequoia's
application style and either Liquid Glass or standard KWin Blur.

Enable **Jungle Vines** in **System Settings → Window Management → KWin Scripts**
and click **Apply**. It is installed disabled by default. Unchecking it and applying
unloads the script, destroying its window, animation driver, environment polling
timer, and diagnostics objects. No monkey animation or polling runs while disabled.
KWin may retain the native library in its QML cache until logout.

Use the **Configure** button beside Jungle Vines to adjust **Swing speed**
(25–300%; default 100%) and **Pause between visits** (0–3,600 seconds; default
2.6 seconds). The pause starts after the vines retract. Applying settings restarts
the current visit. Speed changes affect the crossing, while descent, retraction,
and the pause retain their durations in real seconds. A filesystem watcher applies
saved settings immediately and is destroyed when the script is disabled.

While the script is enabled, **Meta+Ctrl+M** toggles canopy visibility. This shortcut
is a temporary visibility toggle; use KWin Scripts to unload it entirely. The canopy automatically hides during
fullscreen applications, fullscreen compositor effects, and screen locking.
Mouse input passes through to applications; the effect never grabs the keyboard
or changes the active window. Its animation driver stops while hidden.

```sh
python3 tools/install-vines.py install
python3 tools/install-vines.py status
python3 tools/install-vines.py enable
python3 tools/install-vines.py disable
python3 tools/install-vines.py remove
```

Installation builds against the installed Qt 6 / KWin development libraries,
places the script under the user's XDG data directory, and leaves new installations
disabled. Reinstalling preserves an explicit enabled/disabled choice. Replaced
files are tracked for removal. Rebuild after upgrading KWin. Native QML modules remain cached inside
KWin: after changing an already-loaded C++ renderer, log out/in to load that new
binary; enabling and disabling the existing script work immediately.

The KWin declarative script owns a transparent internal Qt Quick window and a
small QML extension. Its root Window owns the animation directly without a fullscreen
effect view or input interception. The extension renders antialiased QPainter paths at
Qt Quick frame cadence, within a full-display-width canopy at most 480 logical
pixels tall. A FrameAnimation advances the pose before scene synchronization;
there is no competing 16 ms timer. Neither
the supplied GIF nor a solid background is used as a desktop overlay.

Verification uses a private KWin session and real pointer input:

```sh
python3 tools/verify-vines.py --output /tmp/vines-check
```

It checks forward travel, a complete offscreen exit and canopy retraction, vines at both screen edges,
unchanged background pixels, click-through, focus retention, the actual
global shortcut, fullscreen hiding, disabled-by-default behavior, settings-driven
activation, complete native-object teardown, reactivation, live settings through
the actual KWin configuration dialog, defaults, and shortcut use after reactivation. `render-vines`
also exports a cycle of transparent frames for visual inspection. Add
`--stock-blur` to exercise standard Blur instead of Liquid Glass.

Revision 9 uses the `sequoia-vines-v9` script ID and its own `native9` QML
module. Reusing the old effect's native module from a different package caused
KWin's protected-module cache to reject the script: the checkbox appeared enabled
but neither the animation nor shortcut was created. Both package and module paths
are now versioned to avoid cached QML on upgrades without restarting KWin.
The installer preserves the previous script's enabled choice and retires its old
entry, backing it up under `sequoia-eleven/vines-script-v8-upgrade-backup`.
The old Desktop Effects entry is also retired if present.

The `test-vines-motion` executable checks continuous body/hand velocities,
alternating whole-body poses, a deep suspension arc, overlapping grips, fixed
two-vine skips, upright
untouched vines, bounded vine stretch, post-contact loading, reachable catches, and canopy timing
across screen widths. `render-vines OUTPUT [START DURATION FRAMES]` exports
transparent frames for checking the movement at playback speed.

Frame timing can be measured without capturing desktop pixels. Sampling is off
by default and stores at most 10,000 timestamps per stage in memory:

```sh
python3 tools/verify-vines.py --profile-only --output /tmp/vines-pacing
qdbus-qt6 org.kde.KWin /SequoiaVines/Diagnostics org.kde.SequoiaVines.Diagnostics.Start
# Allow a few seconds of animation, then collect and stop sampling:
qdbus-qt6 org.kde.KWin /SequoiaVines/Diagnostics org.kde.SequoiaVines.Diagnostics.Stop
```

The report separates CPU paint cost, Qt buffer submissions, and distinct
animation buffers presented by KWin on the canopy's output. The private test
saves `frame-metrics.json` alongside its log. `--package` and `--script-id`
allow another script build to be sampled for comparison.

Source and original artwork: GPL-2.0-or-later.
