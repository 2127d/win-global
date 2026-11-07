# Sequoia Liquid Glass — experimental

A native KWin **6.8.2** effect. This is a working compositor shader, not a tint or
wallpaper simulation. It captures the actual scene behind blur-requesting windows,
uses KWin's Dual Kawase blur, and refracts that texture before the application is
composed. Opaque controls, screenshots and text therefore remain sharp.

The final shader uses a rounded-box surface normal, Snell refraction with IOR 1.45,
a configurable directional light, a specular lobe and a Fresnel-weighted rim.
The default light is above-left (azimuth 225°, elevation 50°). Refraction is 34
logical pixels, the bevel is 96 pixels, and lighting strength is 0.22. A shallow
aspheric lens (strength 0.09) bends the interior too; the rim preserves 75%
full-resolution backdrop detail. Displacement is capped at 64 logical pixels
and repaint expansion includes that sampling range. Sampling
stays inside the captured texture. All geometric shader inputs scale with the output.
This is a screen-space approximation: it cannot see off-screen scenery or derive
lights and reflections from 3D geometry.

## Try it

From the repository root:

```sh
python3 tools/preview-liquid-glass.py
```

The preview builds the plugin and opens a **separate nested Wayland compositor**.
Use the sliders to move the light or change refraction, and the button to compare
with stock KWin Blur. Close the preview to end the test. No administrator access,
live-compositor restart, live KWin settings change, or global effect installation
is performed. Its temporary session, config and D-Bus service are discarded.

To try the installed Discover makeover over the same real backdrop:

```sh
python3 tools/preview-liquid-glass.py --discover
python3 tools/preview-liquid-glass.py --discover --dark
```

This variant requires the installed Sequoia Quick wrapper. A non-installed
component never replaces stock Blur on the user's desktop. The lab explicitly swaps stock and experimental blur. The plugin refuses to
start while stock Blur is loaded; if stock Blur is subsequently enabled, revision
6 suspends its own rendering until stock Blur is disabled.

## Live desktop trial

After installing the matching `sequoia_liquid_glass_v6.so` into KWin's native effect
plugin directory (administrator authentication required), run:

```sh
python3 tools/liquid-glass-trial.py start
python3 tools/liquid-glass-trial.py status
python3 tools/liquid-glass-trial.py restore
```

This swaps the live session's standard Blur for liquid glass without restarting
KWin. It writes no `kwinrc` activation keys: the saved theme returns at the next
login. A receipt under `$XDG_STATE_HOME/sequoia-eleven/` records the original blur
state and compositor identity, and activation failures restore that state.
Unrelated effects, including the separate Firefox blur effect, are left intact.
The `restore` command removes the trial effect and restores the prior blur state.
The installed plugin remains available but disabled by default.

## Clearer app backgrounds (reversible trial)

`python3 tools/app-glass-trial.py start` applies the gallery-inspired app preset:
34px refraction, a 96px curved rim, broad lens strength 0.09, rim clarity 0.75,
a 7px polished edge with an outer highlight, broad shoulder and inset shade,
0.34 directional light strength, blur strength
4 and 112% saturation. Existing translucent Window/Header palette roles use
148/255 tint in light mode and 168/255 in dark mode. Opaque palettes and the
View/Button roles are not converted to transparent surfaces.

The settings receipt records only the changed keys. `python3
tools/app-glass-trial.py restore` restores them, preserving subsequent manual
changes. The older `liquid-glass-trial.py restore` control delegates to this full
restore when an app trial is active. A temporary Plasma logout script restores
these settings at normal logout and removes itself. There is no persistent effect
autostart. After an abrupt power loss, use the restore control if the trial palette
is still present.

### Keep glass after logout or reboot

```sh
python3 tools/app-glass-trial.py enable-persistent
```

This applies the same app and widget treatment, saves V6 activation in KWin's
`[Plugins]` settings, disables stock Blur and older glass revisions, and removes
the session-only logout cleanup. KWin loads the installed effect directly at
startup; no delayed autostart helper is needed. The widget QML service override
and matching app chrome remain selected across logins. Reapplying the app preset
retains this choice.

`python3 tools/app-glass-trial.py restore` (also the **Restore App Appearance**
launcher) disables saved activation and restores the previous appearance. The
separate persistence receipt retains the original plugin settings across repeated
enables and preserves later manual edits when restoring. A stale logout cleanup
invocation is ignored while persistent mode is enabled.

Validation: a fresh private KWin session using a copy of the saved `kwinrc` loaded
the installed V5 plugin automatically, with stock Blur and earlier revisions off.
Temporary-config checks covered repeated enable, logout protection, restoration,
and preservation of later edits. This validates startup without rebooting the
user's desktop. The native plugin still requires its matching KWin version.

Preview this exact preset without touching the live session:

```sh
python3 tools/preview-liquid-glass.py --app discover --app-glass --headless
python3 tools/preview-liquid-glass.py --app gwenview --app-glass --headless
python3 tools/preview-liquid-glass.py --app kweather --app-glass --dark --headless
```

The effect acts on existing transparent regions. Weather's painted scenery and
opaque app panels remain their own artwork; this does not turn every button into
a separate refractive lens.

## Verify

```sh
python3 tools/preview-liquid-glass.py --headless
python3 tools/preview-liquid-glass.py --headless --discover
```

The fixture captures the composited workspace, not a window grab (which bypasses
the backdrop effect). It checks that glass differs from the matching stock blur,
changing the light changes pixels, moving the backdrop updates the result,
restoring the backdrop restores its pixels, and the opaque card is pixel-identical
across blur, lighting and edge thickness changes. A zero-thickness comparison
checks that the polished rim changes visible pixels without affecting opaque content. A zero-refraction capture keeps lighting and
clarity fixed to isolate actual warping. Moving a small backdrop patch and then
removing it must restore the frame exactly, checking partial repaint damage. Captures and logs go to `preview/liquid-glass/`.
Use `--output` to retain separate light/dark captures. `--build-dir` selects a
private build directory.

The lab supplies a 28px fallback corner radius only for its frameless test pane;
normal windows use KWin's own radius. The original requested blur regions and
clipping remain authoritative. Small irregular popup shapes, HDR, mixed-scale
monitors, transformed-window effects and sustained frame-time profiling need
further validation before general deployment. This is deliberately outside the
main theme installer.

## Source and license

The backdrop capture, damage tracking, blur protocol and framebuffer pipeline are
adapted from [KDE KWin v6.8.2's Blur effect](https://invent.kde.org/plasma/kwin/-/tree/v6.8.2/src/plugins/blur).
Original author notices are retained in `blur.cpp` and `blur.h`. This directory is
**GPL-2.0-or-later**, as specified in `LICENSE`, independently of the repository's
MIT license. The names, resource paths and configuration group are distinct from
stock Blur so loading/unloading the experiment does not replace upstream files.
`shaders/glass.glsl` and the final-pass uniform setup implement the new material.
The build rejects other KWin versions pending an API and rendering review.

Revision 5 uses distinct plugin, C++ class and resource identities so it can load
in the same KWin process as a previously tested revision without restarting the
desktop. Only one revision runs at a time; restore unloads all five revisions. The
configuration group remains `Effect-sequoia_liquid_glass` for receipt compatibility.


## Matching chrome and Plasma surfaces

The reversible app trial also selects the separately built
`org.kde.sequoiaeleven.glass-polished` decoration and `SequoiaElevenGlass` Qt style. Build
those from `themes/application-style/SequoiaEleven` with
`-DSEQUOIA_GLASS_TRIAL=ON`. Install the two resulting modules into the normal
KDecoration3 and Qt styles plugin directories. The normal plugins are unchanged.
The title bar, standard toolbar and menubar use the canvas tint without a second
opaque fill. Interior content islands remain opaque. Gwenview's custom floating
surfaces detect the trial style and use the same tint plus a layered rim; their
normal style behavior is unchanged.

`tools/glass_surfaces.py` builds a separate Plasma theme with 24px widget/popup
corners, a 1px outer highlight, a 2px inner shoulder and a faint inset shade.
Popup tint is 42% light / 44% dark; desktop widget tint is 28% / 30%. Opaque
fallbacks and panel assets are copied unchanged. Popup windows get actual
backdrop refraction from the KWin effect. Desktop widgets retain Plasma's
wallpaper blur: the SVG rim does not independently refract the desktop texture.
The Quick toolbar patch paints only a rim and sheen over the already tinted
application canvas, avoiding accumulated alpha.

The receipt includes the original decoration, Qt style, WM colors, menu opacity,
Plasma theme and Quick toolbar file. Restore preserves later edits and restores
inherited KConfig defaults rather than leaving deletion markers. Logout cleanup
covers these settings too. Already-open Qt Quick apps need reopening for their
cached toolbar QML; existing Gwenview processes need reopening after the private
binary update. No apps are forcibly closed and KWin is not restarted. If a global theme switch
resets the selected style/decoration, run the trial start control again: it
preserves that newly selected palette and reapplies the matching glass chrome.

Validation:

```sh
python3 tools/preview-decoration.py --material glass
python3 tools/preview-decoration.py --material glass --dark
python3 tools/verify-glass-toolbar.py
python3 tools/preview-liquid-glass.py --app discover --glass-surfaces --headless
python3 tools/preview-liquid-glass.py --app popup --glass-surfaces --dark --headless
```

The decoration and toolbar probes compare their rendered RGBA to the canvas in
both palettes; the toolbar probe also checks the normal style stays opaque.


## Weather test seam correction

Revision 4 identifies the standalone `org.sequoia.GlassWeatherTest` main window
(240 × 328 logical pixels) and supplies its actual 40px radius to the optical
surface. Frameless windows otherwise report no decoration radius. It limits that
window's optical bevel to 32px, fading its normal before the rounded-box distance
field reaches the discontinuous interior nearest-edge boundaries. These boundaries
previously produced the diagonal X; the missing radius produced the rectangular
inner rim. Clarity retains its original falloff independently of the optical bevel.
Other windows use the preceding shader parameters. The weather layout is unchanged.

Verify against the previous build in separate nested compositors:

```sh
python3 tools/preview-liquid-glass.py --build-dir BUILD --app weather \
  --weather-binary WEATHER_EXECUTABLE --app-glass --headless --output OUTPUT
python3 tools/preview-liquid-glass.py --build-dir BUILD --headless --output CHECKS
```

The grid-backed weather comparison exposes corner and diagonal seams. The ordinary
fixture verifies refraction, lighting, opaque-content preservation and exact repaint
restoration. Cross-build image comparisons can have isolated 1–2/255 rounding
variations; they should not be confused with a changed material on other windows.


## Shared smooth glass and clear weather test (revision 5)

The bounded bevel now applies to all compositor glass surfaces. Decorated windows
use KWin's radius; frameless popups and panels derive their optical bounds and
rounded corner from the requested blur mask. Bevel lighting ends before the
interior nearest-edge transitions, preventing diagonal seams. Small-radius
surfaces also limit the inset rim thickness so it cannot cross those transitions.
The weather test keeps its explicit 40px radius and 32px bevel, but samples 96%
raw scene detail at the center and 98% near the edge. Its color transform is
neutral and the front tint is lighter; other surfaces keep their blur settings.

Desktop applets have a separate wallpaper-rendering path. `tools/widget_glass.py`
stages a private containment-layout QML module that refracts the existing blurred
wallpaper layer before the theme mask and foreground applet content. Its 24px
rounded surface uses a 19.2px bevel. It activates only for marked trial-theme SVGs;
rotated/scaled applets keep the ordinary Plasma rendering path. The system module
and existing motion overlay remain untouched. The temporary service override
prepends the widget module to the current QML import path. Applying/removing it
requires a Plasma shell refresh, not a KWin restart or an app restart.

The app trial's restore/logout control also restores widget SVG markers and removes
the temporary service override, preserving subsequent manual edits. The private
module remains inert on disk. Rendering validation:

```sh
python3 tools/verify-widget-glass.py
python3 tools/preview-liquid-glass.py --build-dir BUILD --headless
python3 tools/preview-liquid-glass.py --build-dir BUILD --app weather \
  --weather-binary WEATHER_EXECUTABLE --app-glass --headless
```


## Blur toggle recovery (revision 6)

KWin 6.8.2 advertises the Wayland blur capability with a shared boolean, rather
than counting effect providers. Loading stock Blur alongside glass and then
unloading it used to withdraw that capability from applications while glass
remained loaded. This was reproduced by the preview's capability check.

Revision 6 restores the capability in its pre-paint hook (a no-op when already
advertised). Glass suspends rendering while stock Blur is loaded, then resumes
when it is removed. Unloading glass while stock Blur remains loaded no longer
withdraws the capability. No polling process or app restart is required.
Distinct plugin and resource names allow this repair to load without restarting
KWin. Persistent activation migrates to V6 while retaining the original restore
baseline and disabling V5. The material and shader appearance are unchanged.

```sh
GLASS_TOGGLE_CHECK=1 python3 tools/preview-liquid-glass.py --build-dir BUILD --headless
```

This regression checks three stock Blur on/off cycles, exact recovery of the
composited pixels, stock-only rendering during suspension, removal of either
provider, and re-enabling after both are removed. The same capability test fails
against V5. The ordinary rendering fixture still checks refraction, lighting,
opaque content, and partial repaint restoration.


## Antialiased decoration corners

The glass decoration uses `org.kde.sequoiaeleven.glass-smooth` so the repaired
plugin can load without a compositor restart. Titlebar painting explicitly enables
QPainter antialiasing. Normal glass frames publish their full corner radius and
request a rectangular backdrop region; KWin's subpixel rounded-box shader performs
the outer clipping instead of a whole-pixel polygon approximation. This preserves
the 32px curve and the canvas tint while removing stair-stepped corner coverage.
The ordinary and floating-titlebar blur-region paths retain their existing masks.

Native previews were checked at 100% dark and 125% light scaling, including the
rendered canvas alpha, followed by a private compositor preview with Discover and
the live glass effect. `tools/preview-decoration.py` supports `--plugin`,
`--plugin-path`, and `--output` for isolated before/after comparisons; an explicit
output also saves the transparent alpha probe beside it.


## Polished rim, controls, and decoration size

The selected decoration is now `org.kde.sequoiaeleven.glass-polished`, retaining
subpixel corner clipping and the matching canvas tint. A 1px directional outer
reflection (maximum alpha 48/255) and faint inset shade give the edge definition
without a bright uniform outline. Traffic-light controls have a restrained bevel,
soft shadow, and animated hover symbols; disabled animations reveal symbols
immediately. Minimize uses a minus and maximize uses plus/restore symbols.

The glass decoration's gear opens `kcm_sequoiaelevenglassdecoration`. Its
**Decoration size** slider saves `Windeco/DecorationScale` in `sequoiaelevenrc`,
from 80 to 140%, default 100%. It scales titlebar controls and padding, leaving
caption font preferences intact. At 100%, titlebar padding gains 4 logical pixels
above and below, and ordinary button groups have at least 28px side insets plus
corner clearance when needed. Maximized windows retain edge hit targets.

The KCM was verified with an isolated config and session bus: load/save/reload,
defaults, keyboard steps, dirty state, endpoints, and percentage label. Native
previews covered 80/100/140%, active/inactive, light/dark and fractional scaling,
plus a private compositor check. The separate configuration module leaves the
ordinary decoration module available. Saved glass selection and the existing
restore receipt were updated; KWin was not restarted.


## Adjustable polished rim

`org.kde.sequoiaeleven.glass-adjustable` adds **Glass rim intensity** to the
General tab, through `kcm_sequoiaelevenglassrim`. It saves
`Windeco/GlassRimIntensity` from 0 to 200%, default 100%. This scales only the
outer highlight and inset shade; it does not change the canvas palette, blur,
refraction, or window opacity. Zero hides the added polished outline while the
underlying optical glass remains. Titlebar icon groups move approximately 6px
left and 2px down at 100% decoration size, subject to corner clearance and the
existing maximized-edge hit targets.

Verification covered slider labels, keyboard, save/reload, defaults and endpoints.
Rendered 0/100/200 comparisons changed edge pixels while preserving identical
interior RGBA. Light fractional-scale and compact-size previews checked placement.
The updated plugin/configuration module use distinct names to allow a live update
without replacing mapped libraries; saved selection and restore receipts follow
this revision. Existing decoration-size and transparency preferences are retained.
