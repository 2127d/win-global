# Sequoia Digital Clock

First review version of a separate KDE Digital Clock fork, with a live time/date
header, rounded calendar and agenda surfaces, segmented day/month/year navigation,
accented dates, and clearer world-clock cards. The inner calendar is an opaque
white surface with 18px corners and a soft drop shadow. It uses dark text in both
themes while date highlights and focus accents track the active desktop palette,
including wallpaper-derived accents. The surrounding popup follows the theme.

The package ID is `org.sequoia.digitalclock`. It coexists with KDE's clock and has
independent settings. It does not migrate, replace, or edit existing panel clocks.

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
```

The verifier first checks that Plasma's actual Add Widgets model marks the package
supported. It then builds a test hook and runs the real package in `plasmawindowed`
with private XDG directories and a private D-Bus session. It checks opening the
popup, time formats, custom dates, time zones, month navigation, month/year overviews,
week numbers, first weekday, pinning, and single-zone/agenda layouts. It generates
light and dark captures in `preview/digital-clock-*.png`. It requires Qt 6 Quick,
Widgets and Test development packages, a C++ compiler, and Plasma runtime tools.
Live personal calendar accounts and external application launches are not exercised.
The native option uses a private Wayland compositor and Qt's graphics renderer to
capture the shadow; it additionally requires `kwin_wayland` and Mesa OpenGL.
The verifier also changes the accent in its private session to check that date
highlights update across the calendar's light surface.
