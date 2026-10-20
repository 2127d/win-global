# Sequoia Digital Clock

First review version of a separate KDE Digital Clock fork, with a live time/date
header, rounded calendar and agenda surfaces, segmented day/month/year navigation,
accented dates, and clearer world-clock cards. The inner calendar uses the theme's
card surface with 18px corners and a soft drop shadow. Its background and text
follow live light/dark theme changes, while date highlights and focus accents
track the active desktop palette, including wallpaper-derived accents.

The package ID is `org.sequoia.digitalclock`. It coexists with KDE's clock and has
independent settings. It does not migrate, replace, or edit existing panel clocks.

Settings use a palette-tinted, blurred window with separate rounded sidebar and content
panels, inset margins, and soft shadows. Surfaces follow the desktop palette,
including its transparent variant. The inner page stack is masked to 14px corners
on the normal GPU renderer, including native page backgrounds and toolbars.
The applet's configuration model decorates
only its own settings session; shared Plasma shell files and the default clock are
untouched. Native navigation, Apply/Cancel, shortcuts, About, and calendar plugin
pages remain in use. The decorator checks the Plasma settings shell structure and
keeps the native frame if a future shell is incompatible.
The outer margins and panel gap use the Window palette's color and alpha, matching
the Sequoia decoration rather than leaving an entirely transparent canvas. The
window paints this material once and follows palette changes while open.

The SequoiaEleven application style supplies blur for this settings window and
its transient dropdowns, including reopened menus and recreated surfaces. The
settings window opts in by name; KDE's original clock, panels and other Plasma
popups retain their own blur behavior. The Appearance and Calendar dropdowns use
Qt's popup windows so the compositor can blur the settings content behind them.
The style also handles Qt 6.11's ComboBox focus-out regression: transferring focus
into that combo's own visible popup must not immediately dismiss it. This exception
is restricted to the marked clock settings window; other focus loss and normal
menu selection/dismissal still follow Qt's handling.
This requires the updated SequoiaEleven style. The widget is pure QML again;
there is no separate native blur plugin to build or install.
The rounded mask requires Qt5Compat GraphicalEffects;
the software renderer keeps the native page paint so the controls remain usable.

## Install and review

From the repository root:

```sh
python3 tools/build-digital-clock.py
kpackagetool6 --type Plasma/Applet --install dist/widgets/sequoia-digital-clock.plasmoid
```

For subsequent revisions, use `--upgrade` instead of `--install`. Add **Sequoia
Digital Clock** using the panel's **Add Widgets** menu. To open a separate review
window, run `plasmawindowed org.sequoia.digitalclock` and click the clock.

This review package is intentionally separate from the main desktop installer.
Remove it with `kpackagetool6 --type Plasma/Applet --remove org.sequoia.digitalclock`.

## Functionality and upstream

The fork retains KDE's settings schema and QML behavior: automatic or custom fonts,
12/24-hour time, seconds, date formats and positioning, time-zone selection and
wheel switching, tooltips, clipboard menu, middle-click calendar launch, calendar
plugins, agenda, alternate calendars, week numbers, first weekday, day/month/year
navigation, keyboard navigation, and popup pinning.

The applet QML was extracted from the installed Plasma 6.7.5 plugin; the five
calendar view components were copied from its installed calendar QML module.
`upstream.json` records original file hashes. Original copyright headers and
licenses are retained. Existing strings explicitly use KDE's translation catalog.
Shared clock, calendar and event backends remain provided by KDE; no system QML
or shared calendar components are overridden.

Requires Plasma 6.7 or later, with the digital clock and calendar QML modules.
The package declares the Plasma 6 applet API marker `6.0`, matching KDE's
original clock. This API marker is separate from the runtime version requirement;
using `6.7` causes the widget picker to flag the package as unsupported.
Private KDE interfaces should be rechecked after Plasma updates. This version was
tested on Plasma 6.7.5; older Plasma versions are not claimed to be compatible.

## Validation

```sh
python3 tools/verify-digital-clock.py
python3 tools/verify-digital-clock.py --native
python3 tools/verify-digital-clock.py --native --desktop-style
python3 tools/verify-digital-clock.py --native --desktop-style --transparent
# Test a freshly built style before installing it:
bash tools/build-desktop.sh
python3 tools/verify-window-materials.py
python3 tools/verify-digital-clock.py --built-style --transparent
python3 tools/verify-digital-clock.py --native --built-style --transparent
```

The verifier first checks that Plasma's actual Add Widgets model marks the package
supported. It then builds a test hook and runs the real package in `plasmawindowed`
with private XDG directories and a private D-Bus session. It checks opening the
popup, time formats, custom dates, time zones, month navigation, month/year overviews,
week numbers, first weekday, pinning, and single-zone/agenda layouts. It generates
light and dark captures in `preview/digital-clock-*.png`. It requires Qt 6 Quick,
Widgets and Test development packages, a C++ compiler, and Plasma runtime tools.
Live personal calendar accounts and external application launches are not exercised.
Settings checks cover floating surfaces over transparency, theme blur requests
and re-exposure, real dropdown windows and reopen behavior with the Sequoia style,
rounded page pixels with the translucent palette, page switching,
Apply, and canceling/discarding unsaved edits. Settings captures use the
`digital-clock-*-settings-*.png` names. `--desktop-style` uses the installed
SequoiaEleven widget style and KDE Qt Quick controls for visual review.
`--transparent` selects the transparent light/dark palettes used by the desktop.
The offscreen style run clicks all six dropdowns with the combo initially focused,
checks that their windows stay open and receive blur, and selects values. It also
checks reopening, Space/arrow/Enter selection, Escape, and outside-click dismissal.
This catches the focus-transfer regression missed by calling `popup.open()` directly.
The Wayland run checks rendering and popup-window selection;
synthetic Qt test clicks cannot supply the compositor input serial required for a
Wayland menu grab. The window-material checks also cover nested menus, surface
recreation, and isolation from default Plasma windows.
The verifier tolerates two specific upstream Kirigami PageRow null-header warnings
when entering/leaving Time Zones, reproduced with the unmodified Plasma frame.
The native option uses a private Wayland compositor and Qt's graphics renderer to
capture the shadow; it additionally requires `kwin_wayland` and Mesa OpenGL.
The verifier also changes the background, text, and accent colors in its private
session to check that the calendar surface, dates, and highlights update while open.
