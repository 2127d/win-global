# Shared KDE Get New browser

`tools/newstuff_style.py` adapts the installed `org.kde.newstuff` QML module.
It reuses the Sequoia component library's ActionPill, CircleButton, SearchField,
ThemeSelector, PopupMenu, MenuItem and Panel. KDE retains its engine, native
filter/sort actions, question handling, install/update/remove operations, and
entry-event notifications. No replacement store or application-specific backend
is introduced.

The default compact tiles and larger preview cards both remain available.
Plasma Style uses preview cards. The shared header wraps, controls fit their
content, cards have bounded widths, and the back button follows the details
stack. Popup menus close on a second trigger click.

The full Quick overlay includes this presentation. Native Sequoia styles also
load a smaller overlay for QWidget apps and Plasma: NewStuff, the component
library, and stock desktop controls with the opaque toolbar surface. Other
Kirigami/Plasma controls are not replaced by that smaller overlay. Both overlays
fingerprint upstream QML/plugins and fall back after incompatible KDE updates.
Applications must be reopened after replacing a native style plugin.

Installed catalogs found on this machine (2026-11-13):

- Plasma styles, global themes, colors, icons, cursors, GTK themes, fonts
- Window decorations, Aurorae decorations, splash screens, task switchers
- Wallpapers, mobile wallpapers, wallpaper plugins, Plasma widgets
- KWin effects and scripts, KRunner plugins, Dolphin service menus
- System Monitor pages, presets and faces
- Comic strips, KPat themes, card themes, message-header themes, Sieve scripts

These 27 `.knsrc` files use the shared KDE engine. Callers that use NewStuff
Dialog, DialogContent or Page receive this presentation when running with the
Sequoia native style or the Quick overlay. Custom store interfaces and separately
sandboxed application runtimes are outside this host module's reach.

Build the native regular and Glass style plugins and the Quick overlay, then run
`python3 tools/install-newstuff-style.py`. The installer updates the existing
per-user style path and Quick installation, preserves the active Glass restore
state, and saves a rollback copy under `~/.local/share/sequoia-eleven/backups`.
It does not change the user's selected theme or install store content.

Validation:

- `tools/verify-glass-toolbar.py`: light/dark native surface alpha, canvas preserved
- `tools/verify-glass-rim.py`: opaque toolbar, live menu/Quick glass rims retained
- `tools/verify-application-islands.py`: toolbar input, overflow, orientation, docks
- `tools/verify-newstuff.py --composited [--dark --glass] [--minimal]`: local
  provider search/filter, menu toggle, details/back, actual payload install/remove
  in private XDG roots and a private compositor
- `tools/verify-newstuff.py --composited --native plasma` and `--native dialog`:
  the real Plasma Style Get New action and KDE's standalone download window,
  with a local provider and automatic native-style import activation
