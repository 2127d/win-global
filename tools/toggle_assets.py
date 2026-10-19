#!/usr/bin/env python3
"""Generate native Plasma switch artwork with runtime color-scheme roles."""


def build_switch(base, palette, svg_type):
    svg = svg_type()
    svg.parts.append(f'''<style id="current-color-scheme" type="text/css">
.ColorScheme-Highlight {{ color: {palette['accent']}; }}
.ColorScheme-Text {{ color: {palette['text']}; }}
</style>''')
    # A neutral solid track remains legible even when KDE supplies a disabled
    # text/selection palette; the active fill alone follows the account accent.
    svg.frame('inactive', palette['track'], radius=12)
    start = len(svg.parts)
    svg.frame('active', 'currentColor', radius=12, border=palette['track'],
              stroke=0.5, border_opacity=0.4)
    for index in range(start, len(svg.parts)):
        svg.parts[index] = svg.parts[index].replace('fill="currentColor"',
            'class="ColorScheme-Highlight" fill="currentColor"')
    svg.hint('hint-bar-size', 42, 24)
    # Transparent bounds keep a two-pixel inset around the white thumb without
    # changing the native control's size, hit testing, or drag calculation.
    for index, name in enumerate(('handle', 'handle-active', 'handle-hover', 'handle-pressed')):
        svg.parts.append(f'''<g id="{name}" transform="translate({index * 36} 240)">
<rect width="24" height="24" fill="none"/>
<circle cx="12" cy="12" r="10" fill="#ffffff"/>
</g>''')
    svg.parts.append('''<g id="handle-shadow" transform="translate(150 240)">
<rect width="24" height="26" fill="none"/>
<circle cx="12" cy="14" r="10" fill="#000000" fill-opacity="0.18"/>
</g>
<g id="handle-focus" transform="translate(190 240)">
<rect width="28" height="28" fill="none"/>
<circle cx="14" cy="14" r="12.5" fill="none" stroke="currentColor" class="ColorScheme-Highlight" stroke-width="2"/>
</g>''')
    svg.row = 3
    svg.save(base / 'widgets/switch.svg')


if __name__ == '__main__':
    from build import ROOT, PALETTES, Svg
    for mode, palette in PALETTES.items():
        build_switch(ROOT / 'themes/plasma/desktoptheme' / f'sequoia-eleven-{mode}', palette, Svg)
    print('Generated accent-aware Plasma switches for light and dark themes.')
