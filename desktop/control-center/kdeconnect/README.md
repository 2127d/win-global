# KDE Connect

The Sequoia presentation keeps KDE Connect's installed device, plugin,
notification and command models. `DeviceDelegate.qml` adapts those native
interfaces; `ConnectDeviceView.qml` presents their supported actions.

Each connected device has a summary card with battery and cellular status,
labelled file/clipboard/message/ring/virtual-display actions, and file dropping.
Notifications and remote commands have separate expandable cards. Notification
replies preserve Enter to send, Shift+Enter for a newline, and Escape to cancel.
The installed `RemoteKeyboard` component preserves native keyboard event handling.
Pairing, plugin configuration, and mobile-app download links remain available.

The header and top-level cards share a width. `ConnectView.naturalHeight` measures
its content independently of the assigned viewport. The shared tray caps the
popup and leaves longer device lists scrollable, with a compact settings footer.

The QML-only applet ships as `org.sequoia.kdeconnect`; its migration alias remains
`org.kde.kdeconnect`. It requires the installed KDE Connect QML backend but no
native applet factory wrapper.

Validation:

- `python3 tools/verify-kdeconnect.py --output /tmp/kde-connect.png`
- Add `--render-only --expanded --dark --large --rtl` for a larger-font preview.
- `python3 tools/verify-tray-sizing.py --output /tmp/tray-sizing`

Fixture interaction tests do not communicate with real devices. The tray check
uses a private D-Bus session and virtual Wayland compositor.
