# Notifications

The notification center uses the tray header's horizontal bounds for the Do Not Disturb card, empty state, and all history cards. The native scrollbar overlays history so populated and empty views stay aligned. The Settings / Clear history footer fits its contents, like the other Sequoia action islands.

`tools/control_center_notification_history.py` arranges the center after `style_notifications` has applied the existing banner and shared delegate styling. `NotificationQuietCard.qml` uses KDE's notification settings and shared inhibition owner, including its original duration choices and descriptions of application, mirrored-screen, and fullscreen inhibition. The native history model, grouping, read state, filtering, dismiss gestures, notification actions, inline replies, attachment actions, and job controls remain in use. Clear history still uses KDE's `ClearExpired` policy. Settings retains KDE's authorization check and opens the existing KCM.

History has no enclosing second card. App group headings and their children keep the native grouping behavior; each visible card uses the same outer width. The empty center uses a 480-pixel content height and populated history uses 640 pixels, capped to the current screen. Long history scrolls above the footer.

## Verification

```sh
python3 tools/verify-notification-center.py --output /tmp/sequoia-notifications-light
python3 tools/verify-notification-center.py --dark --large --rtl --output /tmp/sequoia-notifications-dark
python3 tools/verify-notification-center.py --banners --output /tmp/sequoia-notification-banners
```

The runner builds a temporary overlay, creates private XDG homes, and starts a private D-Bus session and virtual Wayland compositor. Synthetic notifications never go to the live desktop, and DND tests do not modify live settings. Center checks cover duration selection, matching component widths, grouped history expansion/collapse, native action and reply dispatch, DND deactivation, clearing expired history, and Back navigation. Banner checks reuse the existing native delivery suite, including actions, replies, critical and long notifications, job progress controls, timeouts, and DND suppression.

Requires the regular overlay build dependencies, Qt Quick/Widgets/DBus/Test development files, libnotificationmanager, KWin Wayland, and plasmawindowed. Screenshots and logs are saved to the requested output directory.
