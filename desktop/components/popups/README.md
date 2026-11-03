# Shared Sequoia popup components

The canonical QML implementation lives here. Control Center, the Start menu and
the Digital Clock calendar use it through local `SequoiaPopup` imports.

* `Surface`: opaque card or panel material, optional translucent material,
  palette-aware tint, rim, focus ring, hover/pressed/selected feedback and shadow.
* `ActionSurface`: transparent or raised button background using the same states.
* `HeaderButton`: native Plasma tool button with the shared background; labels,
  icons, accessibility, key handling and actions remain native.
* `tokens.json`: the common geometry and opacity values. `Tokens.qml` and the
  Union prototype's `tokens.css` are generated from this file.

Use `panel: true` for 24px panels; content cards default to 18px. Controls use
10px corners and a 2px keyboard focus ring. Individual layouts can specify a
smaller radius for compact nested controls. `glass: true` applies 72% material
opacity to the background only; opaque islands are the default. This is not a
blur implementation. Plasma's native popup continues to own blur regions,
window shadows, input masks and lifecycle. Do not lower the whole popup's
opacity: text and controls should remain opaque.

The surface explicitly resolves the desktop Window palette, including when a
Control creates a background before parenting it. Hosts with a deliberate local
palette (the calendar, for example) set `Kirigami.Theme.inherit: true` on the
surface. Motion follows KDE's short duration and becomes immediate when KDE
disables animations.

## Making a shared change

1. Edit these canonical components or `tokens.json`.
2. Run `python3 tools/popup_components.py` from the repository root.
3. Run `python3 tools/popup_components.py --check` before packaging.

The tool produces byte-identical local copies for the two standalone plasmoids,
Control Center and its detail components. This is deliberate: exported widgets
must work without a system QML module or a checkout-relative import. The Start
menu, Digital Clock and Control Center builders reject stale copies. The native
widget-fork builder calls the Control Center builder, so it checks them too.
Do not edit generated copies. Removed files are removed from generated folders
on the next sync.

## Integrated surfaces

| Surface | Shared pieces | Behavior retained |
| --- | --- | --- |
| Control Center | Dashboard cards, tiles, header buttons, detail panels | Device actions, sliders, native detail models, navigation |
| Start menu | Search field surface, account panel, section panels, actions | Search, pins, layout, popup positioning, dismissal, native blur |
| Digital Clock | Agenda and month cards, navigation/configuration/pin buttons | Dates, clocks, agenda, calendar navigation, settings, keyboard actions |

The source-compatible `MenuSurface`, `DetailSurface` and `ControlHeaderButton`
names are thin adapters, not separate implementations. Additional surfaces can
adopt these components without converting their service models or actions.

## Validation

```sh
python3 tools/popup_components.py --check
python3 tools/preview-control-center.py
python3 tools/preview-control-center.py --dark --large-font --rtl
SEQUOIA_START_TEST_BACKEND=rhi python3 tools/verify-start-menu-native.py
python3 tools/verify-digital-clock.py --native
python3 tools/build-widget-forks.py --destination .staging/popup-pilot
python3 tools/verify-widget-archives.py --folder .staging/popup-pilot --native --tray-size
```

The previews use isolated configuration and test actions. The native widget
checks install into a temporary prefix; they do not activate the pilot in the
running desktop. Use the main installer for a later desktop rollout so ownership
receipts and removal remain consistent.

See [the Union feasibility study](../../../themes/union/README.md) for the
experimental CSS backend and its remaining limitations.
