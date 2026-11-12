# Liquid Glass light/dark pair

`org.sequoia.eleven.light.glass` and `org.sequoia.eleven.dark.glass` are native
KDE Global Themes for the existing Liquid Glass setup. Each package embeds its
color scheme, so KDE Quick Settings classifies it in the correct Light or Dark
theme picker without relying on a separately installed palette.

Install or update the pair with `python3 tools/glass_global_themes.py --install`.
This checks for the already-installed glass application style, adjustable glass
decoration, and both glass Plasma styles. The main portable installer still
installs the standard Solid/Transparent set; the glass native components remain
part of the separate Liquid Glass setup.

In System Settings → Quick Settings, open the arrow beside each theme thumbnail
and choose **Sequoia Eleven Liquid Glass Light** or **Sequoia Eleven Liquid Glass
Dark**, then Apply. The same pair can be used by KDE's automatic day/night mode.
Installing the packages does not change the current theme or picker choices.

The palettes use the same canvas opacity as Appearance Hub: 148/255 for Light
and 168/255 for Dark. Content views stay opaque. Switching coordinates colors,
application style, Plasma style, icons, window decoration, and splash screen.
It preserves fonts, wallpaper, desktop layout, titlebar buttons, and rim settings.

KDE's native Global Theme manager does not apply arbitrary `[kwinrc][Plugins]`
entries. These packages therefore retain the existing Liquid Glass compositor
configuration; they do not claim to install or enable the effect. The existing
glass setup must already have `sequoia_liquid_glass_v6` enabled and stock blur
disabled. Light/Dark switching leaves those settings intact.

Regenerate package metadata, defaults, palettes, and splash files with
`python3 tools/glass_global_themes.py`. The normal asset builder also calls this
generator. Preview images are real isolated Plasma/Dolphin renders and are
preserved during regeneration.
