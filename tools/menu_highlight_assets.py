#!/usr/bin/env python3
"""Build Plasma menu/list highlights using live accent roles and glass rims."""
import re


def build_viewitem(base, palette, svg_type):
    svg = svg_type()
    svg.parts.append(f'''<style id="current-color-scheme" type="text/css">
.ColorScheme-Highlight {{ color: {palette['accent']}; }}
</style>''')
    svg.frame('normal', palette['surface'], 7, opacity=0, margin=6)
    for state, alpha in [('hover', 0.20), ('selected', 0.25), ('selected+hover', 0.30)]:
        start = len(svg.parts)
        svg.frame(state, 'currentColor', 7, 'currentColor', alpha, 6, border_opacity=0.45)
        for i in range(start, len(svg.parts)):
            part = svg.parts[i]
            if not part.startswith('<g '):
                continue
            # A translucent specular layer sits over the accent and its rim.
            # Reuse the exact nine-slice geometry to avoid seams at corners.
            content = part[part.index('>') + 1:-4]
            sheen = content.replace('fill="currentColor"', 'fill="#ffffff"')
            sheen = re.sub(r'fill-opacity="(?!0")([^"]+)"',
                           lambda m: 'fill-opacity="' + ('0.20' if m[1] == '0.45' else '0.08') + '"', sheen)
            part = part.replace('fill="currentColor"', 'class="ColorScheme-Highlight" fill="currentColor"')
            svg.parts[i] = part[:-4] + sheen + '</g>'
    svg.save(base / 'widgets/viewitem.svg')


if __name__ == '__main__':
    from build import ROOT, PALETTES, Svg
    for mode, palette in PALETTES.items():
        build_viewitem(ROOT / 'themes/plasma/desktoptheme' / f'sequoia-eleven-{mode}', palette, Svg)
