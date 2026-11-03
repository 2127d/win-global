# Sequoia Union feasibility study

This is an experimental backend for the shared popup tokens, not an installed
Global Theme or a replacement for Sequoia's compiled application style.

## Run

```sh
python3 tools/popup_components.py --check
python3 tools/verify-union-style.py --output .staging/union-preview
python3 tools/verify-union-style.py --native --output .staging/union-native
```

Requires the distribution's Union package, PySide6 and, for `--native`, KWin
Wayland. The test creates private XDG directories and a private D-Bus session,
stages the installed Breeze CSS beside this prototype, and generates light/dark
palette values from Sequoia's actual color schemes. It starts a private virtual
compositor for native testing and tears it down afterwards. The user's selected
style is never changed. The prototype's relative Breeze import assumes that
staged sibling layout; this is not a standalone distributable style yet.

The QML probe uses ordinary Qt Quick Controls without custom backgrounds. The
Widgets probe uses QGroupBox, QPushButton, QLineEdit and QSlider through the real
Union QStyle. Both exercise keyboard activation and disabled controls. Screenshots,
style measurements and Wayland logs are saved to the selected output directory.

## Findings on Plasma/Union 6.8.2, Qt 6.11.2

| Requirement | Observed result |
| --- | --- |
| Shared card and control geometry | Qt Quick applies 18px card and 10px control corners from the generated CSS tokens. Qt Widgets paints visibly rounded controls. |
| Light/dark materials | Both outputs load the Sequoia palettes and opaque content islands. |
| Qt Quick translucency | The window buffer contains approximately 0.718 alpha for the requested 0.72 material; interior cards remain opaque. |
| Shadows | The Quick style resolves the shadow parameters and renders with the native scene graph. Pixel-perfect parity across Qt Widgets and Quick is not established. |
| Keyboard and disabled actions | Passed in both outputs and both palettes. |
| Automatic Qt Widgets glass window | Not supplied by this CSS. The unmodified QWidget remains opaque and does not enable `WA_TranslucentBackground`. |
| Automatic compositor blur | Neither native probe requests a Wayland blur region, although the compositor advertises the background-effect protocol. Alpha alone is not frosted glass. |

The current prototype inherits Breeze switches and sliders; it does not claim
complete Sequoia control or motion parity. It establishes that the shared
geometry/material language can reach both Qt stacks from CSS.

Union's documentation also limits layout control: applications own their layout
and final sizing. Its supported-CSS page lists transitions/animations as
unsupported. Those are documented constraints, not results of the rendering
probe. A CSS theme alone therefore cannot reproduce our custom dashboard,
floating toolbar/sidebar arrangement, popup lifecycle or Sequoia motion.

**Decision:** continue the shared popup QML rollout now. Union is a viable
experimental backend for common control painting, but retain the existing
Qt/KWin glass integration and layout components. Evaluate more controls and
real application coverage before replacing the compiled style.

References:

* [Working with Union styles](https://api.kde.org/working-with-styles.html)
* [What Union can and cannot style](https://api.kde.org/css-styling-applications.html)
* [Supported CSS](https://api.kde.org/css-support.html)
* [Plasma 6.8 release notes](https://kde.org/announcements/plasma/6/6.8.0/)
