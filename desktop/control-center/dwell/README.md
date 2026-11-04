# Dwell Clicker

The native KWin D-Bus service watcher, property reader and button property setter
remain in use. The five original IDs (10 through 14) and selecting the same mode
again to clear it (0) are preserved. External button updates follow KWin. The
applet retains KDE's close-on-selection behaviour when hideOnWindowDeactivate
is set, and does not enable dwell clicking merely by being installed or opened.

The presentation has aligned status and mode cards, large labelled mode buttons,
a clearly selected mode, an explanation when the service is off, and an
authorized Accessibility settings shortcut. Size comes from content and supports
narrow widths, larger fonts and right-to-left layout.

- `python3 tools/verify-dwell.py --output /tmp/dwell.png` tests the presentation.
- `python3 tools/verify-dwell-backend.py --main STAGED_DWELL_MAIN.qml` exercises
  the exact staged watcher/controller against a fake service on a private bus,
  including all IDs, clearing, external changes, service loss and popup policy.
- `tools/verify-tray-sizing.py` checks native loading and independent sizing.

Tests never select a click mode on the user's real KWin service.
