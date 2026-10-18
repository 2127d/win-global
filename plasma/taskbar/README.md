# Sequoia Taskbar

A separate Plasma panel with integrated pinned application launchers, running
tasks, and activity indicators. These are direct children of
the panel containment, not applets. Ordinary Plasma widgets remain addable in the
trailing widget area through Edit Mode. Add your preferred start menu as a regular
widget using **Edit Mode → Add Widgets**; the taskbar has no built-in start menu.

The native **Taskbar** KCM appears under **Appearance** in System Settings,
including the repository's Settings navigation fork. Its settings apply to all
Sequoia Taskbar panels in the current account; other panel types are unaffected.

| Setting | Behavior |
| --- | --- |
| Panel / Dock | Panel fills the available edge; Dock fits the built-in tasks and added widgets, growing and shrinking as contents change. |
| Keep icons the same size | Uses the selected logical pixel size; excess tasks scroll. |
| Fill available space | Expands button widths evenly; icons retain the selected size. |
| Resize with panel | Icon size follows panel thickness, minus padding. |
| Spacing and padding | Controls button separation and space around each icon. |
| Alignment | Start, center, or end within the available task area. |
| Running indicators | Optional dots/bars distinguish running and active applications. |
| Pinned applications | Pin, remove, and reorder application launchers. |
| Window filtering | Optionally restrict tasks to the current screen or desktop. |
| Sequoia animations | Hover zoom/lift, press feedback, a single launch bounce, a brief attention pulse, and task movement. Includes intensity/speed controls and a live preview. |

Icons are clamped when the panel is thinner than the requested icon and padding.
Widgets retain their own size preferences. Position, panel thickness, visibility,
floating mode, and panel length use Plasma's regular panel editor. Switching the
Panel/Dock preset changes only Plasma's length policy (Fill Available / Fit
Content). It preserves those other controls. A length override made afterward in
the panel editor survives unrelated settings changes and shell restarts. New
taskbars inherit the selected preset. In Dock mode, Fill available space has no
extra space to distribute unless the panel editor supplies a larger length.

Click a running task to activate it, or minimize it when active. Grouped tasks
open a window list. Middle-click requests another instance; right-click exposes
pin/unpin, new window, close, and Taskbar Settings. Scroll the task area when its
buttons exceed the available space.

## Build and install

This implementation targets the installed **Plasma 6.7.5** panel backend. The
builder compares the upstream panel's embedded QML with `upstream.json` before
building. Other Plasma releases need a compatibility review. Build on the target
computer with Qt 6 development tools, ECM, KDE Frameworks 6 Config, CoreAddons,
KCMUtils and I18n development packages, CMake, Ninja, a C++ compiler, and PySide6.
Runtime dependencies include Plasma Desktop, its task-manager QML module,
Kirigami, and Plasma Components.

From the repository root:

```sh
python3 tools/build-taskbar.py --stage .staging/taskbar-ready
python3 tools/verify-taskbar-install.py --stage .staging/taskbar-ready
python3 tools/verify-taskbar.py --payload .staging/taskbar-ready
python3 tools/verify-taskbar-motion.py
python3 tools/verify-taskbar-motion.py --reduced-motion
sudo python3 tools/install-taskbar.py --stage .staging/taskbar-ready
```

Use a new stage directory for a subsequent build. The native verification needs
KWin Wayland, a local D-Bus session, and Qt's `qml-qt6` runtime. It creates a private
desktop with separate XDG directories, leaving the current desktop untouched.
Its logs and screenshots are in `.staging/taskbar-verification/`.

After installation, log out and back in. Choose **Add Panel → Sequoia Taskbar**,
then **System Settings → Appearance → Taskbar**. You can also open the page with
`kcmshell6 kcm_sequoia_taskbar`. Installation adds the panel choice without
creating or replacing panels automatically. This component has its own installer
and is not yet part of the theme installer's component selector.

The installer verifies the staged file hashes and the installed panel backend.
It uses an ownership receipt and backs up existing files. Remove any Sequoia
Taskbar instances in Plasma first, then uninstall with:

```sh
sudo python3 tools/install-taskbar.py --remove
```

Uninstallation restores backed-up files and refuses to overwrite local edits.
The account's `sequoiataskbarrc` preferences are preserved. After Plasma updates,
rebuild against the matching backend; compiled adapters are not portable binaries.

## Implementation and verification

The containment reuses KDE's installed panel backend through the independently
named native factory. The forked containment layout retains its widget model and
editing support; `TaskStrip.qml` supplies the built-in tasks using KDE's TasksModel.
The native KCM and
panel share `TaskbarSettings`, backed by KConfig and change notifications. Apply
writes only edited settings so a concurrently pinned app is not overwritten.

The QML module installs the same `DockMotion.qml` source used by the Sequoia
icons-only task manager under `plasma/control-center/taskmanager/`. Hover and
press use OutCubic easing and a 370 ms baseline scaled by the speed control.
Hover styles are zoom, lift, or both; press scales the icon to 94%; launch bounces
once using OutCubic/InOutCubic. Attention pulses an indicator once over 450 ms.
Task displacement uses the same 370 ms family. These effects work in both panel
and dock modes, with lift directed inward from all four screen edges. Only
visuals transform, keeping button hit areas and layout stable. Motion is clamped
to available button space; increase padding/thickness for more headroom.

The shared component treats Kirigami durations of 0 **or 1 ms** as disabled
motion. KDE's desktop integration clamps a disabled animation duration to 1 ms;
the original widget's `> 0` check did not cover this (see KDE's
[desktop units implementation](https://github.com/KDE/qqc2-desktop-style/blob/master/kirigami-plasmadesktop-integration/plasmadesktopunits.cpp)). The widget generator uses
the corrected guard too. The original widget's hover, launch, attention, and
movement configuration keys and defaults are retained in the taskbar's separate
settings file; widget settings are not imported or modified.

CTest covers persistence, cancel/reset behavior, input normalization, concurrent
edits, sizing policies, and overflow calculations. Native tests load the KCM,
check Apply state, add the panel with zero applets and no built-in start menu,
inspect rendered icons, launch a test application, exercise its minimize/restore
behavior, add regular launcher and clock widgets, change sizing modes live, and
rotate the panel. Additional checks
exercise real pointer hover without resizing button hit areas, disabled motion,
dock growth/shrink and vertical widget layout, preserving editor choices across
mode changes and restarts, and creating a new dock. Offscreen tests exercise the
shared icon wrapper's motion bounds, both axes/directions, finite attention
pulses, cancellation, and the actual desktop reduced-motion setting. The installer
test covers updates, backup restoration, edited-file protection, and stage tampering.

Physical multi-monitor, mixed-DPI, touchscreen, and long-running session behavior
still need interactive desktop testing. This first version does not implement
task thumbnail previews or drag reordering of pinned apps; launcher order is
edited centrally in Taskbar settings. Menu behavior and shortcuts belong to the
launcher widget you choose.

KDE's panel files retain their original copyright and GPL/LGPL notices. New
components are MIT licensed; license texts are in `LICENSES/`.
