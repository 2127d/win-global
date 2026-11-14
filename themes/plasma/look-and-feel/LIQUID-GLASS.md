# Liquid Glass components

The broken `org.sequoia.eleven.light.glass` and
`org.sequoia.eleven.dark.glass` Global Theme wrappers have been retired.
They are no longer generated, installed, or offered in the theme pickers.

The Liquid Glass compositor effect, application style, window decorations,
Plasma styles, color schemes, and Appearance Hub material controls remain intact.
The former generator entry point `tools/glass_global_themes.py` now maintains
only the standalone Light/Dark Glass color schemes; `--install` installs only
those palettes and does not apply a theme.

Use Appearance Hub's Glass material controls for the existing Liquid Glass
system. The remaining Solid and Transparent global themes are unchanged.
