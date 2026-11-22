# Sequoia Firefox blur

Native Firefox-only blur and rounded corners, built for KWin 6.8.2 or 6.8.3.
This preserves the v2 implementation previously maintained in the November 1
Firefox upgrade workspace. Original KDE GPL-2.0-or-later notices and shaders
are retained. Firefox supplies transparency; this effect does not alter browser
pixels or window opacity. Normal Firefox windows are matched by window class;
fullscreen windows and other applications are excluded.

Build all three Sequoia effects with the installed development package:

```sh
python3 tools/rebuild-kwin-effects.py
```

The Firefox build produces `sequoia_firefox_blur_v2.so` and `enable-blur.sh`.
CMake generates the startup script's exact version guard from `KWin_VERSION`,
so the guard stays paired with the compiled plugin. Install the binary in Qt's
`kwin/effects/plugins` directory and the helper in
`~/.local/share/sequoia-eleven/firefox-glass/enable-blur.sh`, retaining the existing
autostart entry. The system plugin installation requires administrator access.
Unload the effect before replacing it, and replace the file atomically.

Verification on KWin 6.8.3:

```sh
python3 tools/verify-firefox-blur.py \
  --build-dir .staging/kwin-6.8.3/firefox-blur \
  --effects-build-root .staging/kwin-6.8.3 \
  --output .staging/kwin-6.8.3/firefox-check
```

The fixture uses synthetic Firefox-class windows in a private compositor,
not a real browser profile. It verifies visible backdrop blur, unchanged opaque
content, exact restoration of pixels and corners on unload/reload, fullscreen
exclusion, unrelated-application exclusion, and loading all three effects together.
No rendering changes were required for 6.8.3; both blur effects compile against
the matching headers and publish the matching KWin factory version.
