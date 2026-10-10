# Preview images

This folder contains documentation images and selected visual references for
the desktop, applications, and widgets. Some historical captures are retained
because `VALIDATION.md`, `DEPLOYMENT.md`, or `DESKTOP-REPAIR.md` refers to them;
they are evidence of those sessions, not a guarantee of the current appearance.

Reusable QML scenes and sample SVG artwork belong in `tools/preview/`. Preview
and verification scripts read those inputs and write generated captures here.
The control-center fixture lives with its components in
`plasma/control-center/Preview.qml`.

## Regeneration

Run commands from the project root with the documented Qt/KDE dependencies:

```sh
python3 tools/preview.py --dark --output preview/dark.png
python3 tools/preview.py --output preview/light.png
python3 tools/preview-applications.py --dark
python3 tools/preview-fonts.py
python3 tools/preview-quick-islands.py --dark
python3 tools/preview-lockscreen.py
python3 tools/preview-login.py --mode light
python3 tools/preview-login.py --mode dark
python3 tools/wallpaper.py
```

Application and decoration previews require the compiled desktop plugins.
Native widget checks require their staged packages and, for some checks, a
Wayland session. See the main README and `plasma/control-center/README.md` for
setup and additional commands. Historical desktop screenshots must be captured
manually; they cannot be reproduced exactly by these commands.

## Cleanup

Logs (`*.log`) and UI structure dumps (`*.tree`) are temporary diagnostics. They
are ignored by Git and excluded from newly built source archives. They can be
deleted after the corresponding preview or test process exits.

Review new screenshots before retaining them. Keep images linked by the project
documentation, `light.png` and `dark.png` used by global-theme packaging, and
representative captures needed for visual review. Remove obsolete or redundant
captures after checking documentation references. Existing screenshots are not
automated pixel-comparison test baselines.

Keep this directory: several tools expect it to exist. Do not place source QML
or SVG fixtures here. Source archives include the retained images and this file.
