# KCalc makeover

Presentation fork of KCalc 26.08.1, matching the installed Fedora package.
The native calculator engine, scientific functions, statistics, number bases,
shortcuts, history, constants and preferences remain available.

The interface adds a visible mode selector, a rounded calculation history panel
above the input box, larger rounded keys with space between them, and an
accent-colored equals key. Surfaces
and text follow the system light/dark palette. File, Edit, Settings and Help
remain available in the global menu. The top-level Constants menu is omitted:
recreating it on mode changes left stale, non-opening entries in the native menu
exporter. Constants remain available through the configurable constant buttons
and preferences.

The input expression reads left to right from the left edge; a live answer
appears at the right end of the same row while typing. The
history above stays unchanged until Enter or equals commits the calculation.
For example, typing `1 + 2` previews `3`; Enter adds `1 + 2 = 3` to history,
replaces the input with `3`, and hides the preview. Another operation continues
from that total and shows a fresh preview. Empty or incomplete input hides the
preview; explicit evaluation errors appear in the input row without entering
history. Both C and AC clear the current entry, preview and history.

History is always visible, with newest calculations first, left-aligned text,
12-pixel spacing between equations, and scrolling for longer sessions. Its text
is selectable. It uses the configured history font without
scaling the text when the window gets wider. The old separate sidebar and Show
History toggle are replaced by this upper panel. History remains session-local,
as in upstream KCalc. Existing explicit memory/statistics actions retain their
history entries.

Switching Simple, Science, Statistic or Numeral System mode schedules one resize
to the new layout's natural minimum. Simple mode therefore shrinks after a larger
mode, even if the previous window was manually enlarged. Rapid switches coalesce
into one fit for the final mode. The layout uses the configured base button font
when measuring, so enlarged fonts from the old mode cannot inflate the new size.

The window remains freely resizable above its content minimum. Manual sizes last
until the next mode change; this version does not save a separate size per mode.
Maximized and fullscreen windows skip the automatic fit, including a state change
before the queued fit executes. Saved startup geometry uses native KCalc behavior.

## Build and install

From the repository root, with KCalc 26.08.1 and the Qt/KDE development libraries,
GMP, MPFR and MPC development packages installed:

```sh
python3 tools/build-kcalc.py
python3 tools/verify-kcalc.py
python3 tools/verify-kcalc.py --dark
python3 tools/install-kcalc.py --skip-build
```

Alternatively select **KCalc makeover** in the graphical installer. It installs
build dependencies and checks the installed KCalc version before making changes.
Builds use all available logical CPU cores unless `BUILD_JOBS` is set.
An optional `--sdk /path/to/usr` supplies a local development prefix.

The private binary lives in `~/.local/share/sequoia-eleven/kcalc/`; the per-user
launcher and `org.kde.kcalc.desktop` entry select it. Close and reopen KCalc after
installation. The system package remains installed and supplies translations and
other unchanged resources. If the packaged executable changes after an upgrade,
the launcher falls back to it until this fork is updated and rebuilt.

Remove it through the graphical uninstaller or:

```sh
python3 tools/install-kcalc.py --remove
```

Receipt-based removal restores the previous launcher and desktop entry, including
pre-existing custom files. KCalc preferences are retained.

## Source and verification

`override.json` and `upstream.sha256` identify the official KDE source archive;
the digest records the downloaded baseline, not signature verification. Original
source licenses and translations are bundled in `kcalc-26.08.1/`.
Presentation changes are limited to `kcalc.{cpp,h}`, `kcalc_button.cpp`,
`kcalc_display.cpp`, `kcalc_display_frame.{cpp,h}`, `kcalc_history.{cpp,h}`
and `kcalcui.rc`.

The verification helper loads into the actual executable on a private session bus
with temporary XDG directories. It checks every mode against Simple mode,
repeated and rapid transitions, manual resizing, mode selector wiring, fullscreen
and maximized state/geometry, stable menu identity/counts without a top-level
Constants menu, live arithmetic/copy, inline-preview visibility, history updates
only on commit, running totals, long-history scrolling, keyboard and keypad
edits, C/AC and error recovery, and normal close/process teardown
without forcing application exit. It also tests
launcher arguments, package-upgrade fallback and reversible installation into
paths with spaces. Captures are in `preview/kcalc-light/` and `preview/kcalc-dark/`.
The automated suite uses Qt's offscreen platform. A separate live Wayland check
with the Sequoia Eleven style exercised 32 mode changes and queried the actual
`com.canonical.dbusmenu.GetLayout` export. The old build left nine dead Constants
entries; the corrected build exported only File, Edit, Settings and Help and
exited approximately 11 ms after close. The reported desktop exit lag was not
reproduced independently in either build.
