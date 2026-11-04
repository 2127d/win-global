# Clipboard

Clipboard uses the shared Sequoia detail surfaces and controls. Search and History/Starred tabs sit above individual content cards; each card keeps Star, Copy, Delete, and More actions within reach. Settings and Clear history live in the bottom action island. Empty history, empty starred history, and search with no matches have separate explanations. The text editor and QR view use matching footers.

`tools/control_center_clipboard.py` applies this presentation to the extracted KDE applet during the regular overlay/fork build, after the shared detail and Power Management styling. Changes are checked against exact upstream anchors, so an incompatible KDE update stops the build for review.

The native `HistoryModel`, Klipper service, proxy filtering, text/image/file delegates, text escaping, drag MIME data, sensitive-data warning and clearing action, keyboard search/navigation, configured history retention, action invocation, and Prison barcode generation remain in use. Copying, starring, editing, and removal act on the native model. Clear history retains KDE's starred-item behavior and confirmation dialogs; removing a starred item still requires the native confirmation. Configuration opens KDE's existing settings.

The shared tray gives Clipboard a 400-pixel empty view and a 580-pixel populated/editor/QR view, capped to the screen. History scrolls independently of the search and footer.

## Verification

Run from the repository root:

```sh
python3 tools/verify-clipboard.py --output /tmp/sequoia-clipboard-light
python3 tools/verify-clipboard.py --output /tmp/sequoia-clipboard-dark --dark --large --rtl
```

Requires the normal theme build dependencies, Qt development files, KWin Wayland, `wl-copy`, `wl-paste`, and libei development headers. If libei has no pkg-config metadata, pass `--ei-include /path/to/header-directory`.

The verifier creates private XDG directories, a private D-Bus session, and a virtual Wayland compositor. Both clipboard producers and consumers run there. It exercises native search, starring, text editing, QR image copying through a real pointer click, image/file history, selection-to-copy, removal, native confirmations, and Back navigation. Logs and captures are written only to the requested output directory. It does not read or modify the user's real clipboard. The test helpers refuse to act without the private-session marker.
