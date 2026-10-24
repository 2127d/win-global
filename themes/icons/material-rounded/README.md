# Rounded Sequoia Tahoe controls

Selected Google Material Symbols Rounded SVGs, pinned to the revision and SHA-256
digests in `sources.json`. These use weight 400 at optical sizes 20 and 24,
with filled variants for selected favorites. Original Sequoia battery, Wi-Fi
and microphone state artwork uses the same rounded visual language. Peripheral
batteries and cellular/wired/Bluetooth/device connection states preserve their
levels, network technologies, locks, warnings and trust indicators.
See `LICENSE` (Apache-2.0); existing MacTahoe artwork keeps its own license.

`python3 tools/tahoe_material.py` applies this layer to the checked-in Tahoe family
without a network connection. `tools/build-tahoe-icons.py` also applies it after
the upstream Tahoe transformation, so rebuilding preserves the new controls.

Explicit mappings in `tools/tahoe_material.py` and `tools/tahoe_catalog.py` cover
navigation, formatting, drawing, mail/calendar, media, settings categories,
devices, weather and tray indicators. `tools/tahoe_states.py` preserves battery
percentages/charging, Wi-Fi strength/lock/warning and microphone sensitivity.
Standard and symbolic names follow KDE theme colors; error, warning and success
roles remain distinct. `optical-sizes.json` records insets checked with Qt at
16/22/24/32 pixels. Full-color app and large folder artwork is preserved.

`coverage.json` records converted names and retained small monochrome candidates,
including aliases. This is not a claim that every glyph is converted: specialized
mathematical/engineering notation, brand marks, input-language badges and states
without a reviewed equivalent retain their existing artwork. MIME icons are
only eligible in symbolic directories. The Apps context is excluded. Other extra contexts require monochrome
artwork; complex multi-color assets are excluded.

Maintenance: `tools/fetch-tahoe-material.py` fetches curated sources from the
pinned upstream revision. Run `tools/calibrate-tahoe-material.py` after adding
designs, then apply the overlay and run `tools/audit-tahoe-material.py`. Fetching
requires network access; calibration and visual verification require PySide6.
Normal overlay builds use vendored sources and Python's standard library only.

`SOFT_CONTROLS` in the overlay builder uses original Sequoia geometry for rounded
clipboard, copy, document, save and window outlines. The power-management gauge
retains its Material silhouette with 25% more contour weight; explicit small
`speedometer-symbolic` controls prevent a legacy Apps icon from winning Qt lookup.

Validation: `python3 tools/verify-tahoe-icons.py` checks rendering, theme lookup,
aliases, checksums, licenses, palette variants and distinct device states.
`tools/preview-tahoe-material.py --expanded` creates the before/after sheet;
capture its baseline with `--expanded --capture-before` before applying changes.
`tools/preview-tahoe-states.py` renders device states in both palettes.

Application identities and controls have separate rules. Bare names backed by
Tahoe application artwork retain that artwork; known action namespaces such as
`edit-*` and `document-*` remain controls. `view-app-grid` is explicitly a launcher,
and `user-trash` / `user-trash-full` retain their full-color place artwork.
Existing small aliases for protected names resolve to the same Tahoe source so
Qt cannot prefer an outline at toolbar/taskbar sizes. Colliding UI controls use
explicit `-symbolic` names; `configure`, `edit-delete` and similar action names
remain Material controls. `protected_identities` in the manifest records this
decision, and verification checks actual Qt lookup at 16/22/32/48/64 pixels.
Changes to the identity policy require a clean `build-tahoe-icons.py` rebuild to
remove obsolete generated aliases, then the normal managed installer removes
obsolete installed files. The standalone overlay is for ordinary design updates.
