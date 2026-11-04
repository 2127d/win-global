# Weather detail page

The popup uses KDE's native ForecastControl, forecast model, providers, settings,
unit conversion and weather notices. The presentation shares Sequoia's rounded
surfaces, segmented tabs and compact action footer. Current conditions remain
visible while forecast, conditions and notices scroll below them. Main cards and
tabs share the tray header's width; the footer fits Settings and Source.

The date selector uses the native `timestamp` header role. Choosing a later day
slides the forecast left; choosing an earlier day slides right. Circular arrow
buttons, keyboard arrows and horizontal scrolling provide the same navigation.
The selected day survives tab changes, and animation follows KDE's timing setting.
Forecast, Details and Notices use the same directional slide between tabs, with
the destination page supplying its natural height to the tray's sizing animation.
Day/night periods stay together. Day-only providers retain both high and low readings.
Missing observations are hidden while valid zero readings remain visible.
Provider credit and the original source link remain available. Available reports
stay visible while the provider refreshes, with an Updating label; an empty
loading state appears only before any usable report is available.

`tools/control_center_weather.py` extracts the installed KDE resources and applies
these components during staging/install. It preloads the full representation so
the tray can attach it on the first opening, including before location setup.

Run `tools/verify-weather-popup.py` against a staged weather directory with
`--source` and save images with `--output`. The isolated fixtures cover forecast
columns/dates, zero observations, details/notices, footer signals and loading,
configuration and unavailable states. `--dark --large --rtl --day-only` exercises
the alternate palette, larger fonts and providers without separate night data.

The tray reads `sequoiaPreferredWidth` and `sequoiaPreferredHeight` from this
page. Its height follows the current tab or loading/empty state, including the
current-conditions card and compact footer. Forecast rows retain enough space
for both high and low temperatures at larger text sizes.
