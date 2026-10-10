# Desktop repair — 2026-09-23

## Installed changes

- Added Tahoe application aliases for Mission Center Qt and Merkuro Calendar,
  Contacts and Mail, in the source generator, bundled themes and installed themes.
  Refreshed icon caches and the standalone icon archive.
- Restored the existing Default activity
  (`f31d355d-c20f-456d-a1f9-a69e69a5ee55`) through Activity Manager.
- Disabled Contacts full-representation preloading and deferred ContactsPage
  construction until the applet expands. Installed the tested QML and restarted
  `plasma-plasmashell.service` successfully.
- Isolated the media, contacts, weather and non-live Control Center preview buses
  before daemon startup. Fixed missing data/state isolation in the Settings
  sidebar verifier and the old staged Settings diagnostic.

## Findings

The login shell (PID 2449) hung in a synchronous Akonadi request:
`QDBusPendingCallPrivate::waitForFinished → AgentManagerPrivate::readAgentTypes
→ ContactsModel → AppletQuickItemPrivate::preloadForExpansion`.
Systemd aborted it after its stop timed out. The Contacts applet was constructing
its database model while Plasma preloaded the popup during login. The fix avoids
that login path; Akonadi remains in use when Contacts is explicitly opened.

Activity Manager returned an empty current activity. The saved state referenced
an unknown activity (`4ecf012b-66c2-45b5-b4d2-37984af6daa1`), while the only
configured activity was Default. Both Folder View desktops reported screen -1.
Restoring Default immediately changed those mappings to screens 0 and 1.

This explains the empty wallpaper page and missing Most Used results. The
installed Folder View QML also delays loading its folder layer until its screen
is ready; that layer registers `newMenu` (Create New). A disconnected desktop
therefore lacks that menu. Interactive confirmation of the restored menu was
requested; no confirmation had arrived when this report was written.

Some previous preview tools set private paths only after a private D-Bus daemon
was started, or omitted XDG_STATE_HOME. Activated services inherit the daemon's
environment, so those tools could overwrite live activity state. The exact
previous process that wrote the invalid UUID cannot be established from the
available logs. The saved invalid state was last modified September 22, 16:57.

## Verification

- Qt resolves both requested application aliases to pixel-identical Tahoe icons
  in SequoiaTahoe, SequoiaTahoe-light and SequoiaTahoe-dark.
- Actual Settings landing page: MostUsedModel has six rows, source has twelve.
  Capture: `preview/settings-repaired-20260923.png`.
- Actual wallpaper page shows HDMI-A-1 and DP-3 plus the image gallery.
  Capture: `preview/wallpaper-repaired-20260923.png`. The temporary capture
  process segfaulted during Qt D-Bus thread teardown after saving its image;
  its page loaded successfully. This separate teardown issue is not diagnosed.
- Plasma wallpaper API reads/writes pass for both monitors, reapplying each
  monitor's current image. The Dolphin click itself was not automated.
- Isolated native tray startup passes with Contacts enabled: after 12 seconds,
  the UI remains responsive and no Akonadi service has started.
- Native Contacts popup/profile/QR/Back test passes. Sample-data contacts
  filtering, keyboard navigation, details, QR and search preservation pass.
- Preview-isolation regression test inspects the activated Activity Manager's
  actual environment: all four XDG homes are private, and live saved activity
  bytes remain unchanged.
- Live shell remains active/running with zero automatic restarts; both desktop
  mappings survive the restart. A full logout/login has not been performed.

Backups: `.staging/desktop-repair-20260923/` contains the prior activity state,
activity configuration, desktop configuration, Contacts QML and icon aliases
that already existed. To undo the Contacts change, restore `contacts-main.qml`
to `~/.local/share/sequoia-eleven/control-center/contacts/main.qml` and restart
Plasma. The previous activity state was broken and should not normally be restored.
