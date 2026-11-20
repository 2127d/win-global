# Global-menu category switching

This builds Plasma's global-menu applet from the matching 6.8.2 sources, with
the existing plugin ID so panel configuration and application menu export stay
compatible. Source provenance is recorded in `upstream.json`; upstream licenses
and copyright notices are retained.

Category replacement is a single paint transaction. The popup holds its size
while actions are exchanged, instead of resizing after each action removal or
insertion. After applying the final geometry, the applet renders and flushes the complete
new menu into its backing store. Qt may defer ordinary QWidget repaints until
a Wayland reposition acknowledgement; explicitly submitting the new buffer
prevents the previous category's pixels being stretched into the new geometry.
The existing popup and mouse grab remain active. Painting, material and submenu
spacing continue to come from the shared application style.

Build with `python3 tools/global_menu.py`. The theme's widgets installation
stages this native plugin through the normal installation receipt, including
backup and removal of the replaced system plugin.

The private desktop check uses actual pointer input to switch global-menu
categories, checks keyboard navigation and dismissal, and verifies that a new
category is painted before the switch returns:

```
SEQUOIA_GLOBAL_MENU_SWITCH_CHECK=1 python3 tools/verify-gwenview.py \
  --global-menu --transparent --style-plugins /path/to/staged/plugins
```

The staged plugin directory must contain `plasma/applets/org.kde.plasma.appmenu.so`
and the Sequoia `styles` directory. This test also checks the normal DBus importer
through repeated image and fullscreen changes, without touching the live desktop.
