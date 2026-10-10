#!/usr/bin/env python3
"""Vendor the installed MacTahoe family and give small monochrome icons more weight.

The derived SVG assets retain the upstream GPL-3.0 license (icons/*/COPYING).
Run with --source pointing to the directory containing MacTahoe-blue{,-light,-dark}.
"""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import shutil
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CONTEXTS = {'actions', 'status', 'devices', 'categories', 'places', 'emblems'}
SHAPES = {'path', 'rect', 'circle', 'ellipse', 'polygon', 'polyline', 'line'}
NEUTRALS = {'#363636', '#f2f2f2', '#2e3436', '#dedede', '#dfdfdf',
            '#232629', '#eff0f1', '#000', '#000000', '#fff', '#ffffff',
            'black', 'white', 'currentColor'}
ET.register_namespace('', 'http://www.w3.org/2000/svg')

APP_ALIASES = {
    'io.missioncenter.MissionCenterQt': 'io.missioncenter.MissionCenter',
    'org.kde.merkuro.calendar': 'calendar',
    'org.kde.merkuro': 'calendar',
    'org.kde.merkuro.contact': 'addressbook',
    'org.kde.merkuro.mail': 'internet-mail',
}


def add_app_aliases(theme):
    directory = theme / 'apps/scalable'
    for name, artwork in APP_ALIASES.items():
        target = directory / (artwork + '.svg')
        if not target.is_file():
            raise RuntimeError(f'Missing Tahoe artwork: {target}')
        alias = directory / (name + '.svg')
        alias.unlink(missing_ok=True)
        alias.symlink_to(target.name)


def properties(element, inherited):
    values = dict(inherited)
    values.update(element.attrib)
    values.update(part.strip().split(':', 1) for part in element.get('style', '').split(';') if ':' in part)
    return {key.strip(): value.strip() for key, value in values.items()}


def local_scale(transform):
    scale = 1.0
    for name, args in re.findall(r'(\w+)\s*\(([^)]+)\)', transform):
        numbers = [float(n) for n in re.findall(r'[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?', args)]
        if name == 'scale':
            scale *= math.sqrt(abs(numbers[0] * numbers[-1]))
        elif name == 'matrix':
            scale *= math.sqrt(abs(numbers[0] * numbers[3] - numbers[1] * numbers[2]))
    return scale


def embolden(path):
    tree = ET.parse(path)
    root = tree.getroot()
    # Restrict this to small UI glyphs, preserving full-color Tahoe artwork.
    view = root.get('viewBox', '').replace(',', ' ').split()
    size = float(view[2]) if len(view) == 4 else float(re.sub(r'px$', '', root.get('width', '0')))
    if not 12 <= size <= 32:
        return False
    if path.stem == 'window-close':
        # The X is transparent negative space in the upstream badge. Widen
        # the cutout itself, rather than expanding the surrounding red disc.
        namespace = '{http://www.w3.org/2000/svg}'
        for child in list(root):
            if child.tag != namespace + 'defs':
                root.remove(child)
        c, radius = size / 2, size * 8 / 22
        arm, half = size * 3.8 / 22, size * 1.2 / 22
        points = [(-arm, -arm+half), (-arm+half, -arm), (0, -half),
                  (arm-half, -arm), (arm, -arm+half), (half, 0),
                  (arm, arm-half), (arm-half, arm), (0, half),
                  (-arm+half, arm), (-arm, arm-half), (-half, 0)]
        polygon = 'M ' + ' L '.join(f'{c+x:.5f},{c+y:.5f}' for x, y in points) + ' Z'
        ET.SubElement(root, namespace + 'path', {
            'class': 'ColorScheme-NegativeText', 'fill': 'currentColor', 'fill-rule': 'evenodd',
            'd': f'M {c-radius},{c} a {radius},{radius} 0 1 0 {2*radius},0 a {radius},{radius} 0 1 0 {-2*radius},0 Z ' + polygon})
        tree.write(path, encoding='unicode')
        return True
    if any(e.tag.rsplit('}', 1)[-1] in {'linearGradient', 'radialGradient', 'image', 'filter', 'mask', 'clipPath', 'use'} for e in root.iter()):
        return False
    shapes = []

    def collect(element, inherited, scale=1):
        tag = element.tag.rsplit('}', 1)[-1]
        if tag in {'defs', 'metadata', 'namedview'}:
            return
        props = properties(element, inherited)
        scale *= local_scale(element.get('transform', ''))
        if tag in SHAPES:
            shapes.append((element, props, scale))
        for child in element:
            collect(child, props, scale)

    collect(root, {'fill': 'black', 'stroke': 'none', 'stroke-width': '1'})
    paints = {p.get(key, 'none') for _, p, _ in shapes for key in ('fill', 'stroke')} - {'none'}
    if not paints or not paints & NEUTRALS:
        return False
    # A 1px outline becomes about 1.65px at a 22px toolbar size. Expanding
    # the vector contour also rounds its tips without a raster blur/filter.
    weight = 0.65 * size / 22
    changed = False
    for element, props, scale in shapes:
        if scale < 0.001 or props.get('opacity') == '0' or props.get('fill-opacity') == '0':
            continue
        fill, stroke = props.get('fill', 'black'), props.get('stroke', 'none')
        if (fill not in NEUTRALS | {'none'} or stroke not in NEUTRALS | {'none'}
                or any(cls.startswith('ColorScheme-') and cls != 'ColorScheme-Text'
                       for cls in props.get('class', '').split())):
            continue
        if fill == 'none' and stroke == 'none':
            continue
        styles = dict(part.strip().split(':', 1) for part in element.get('style', '').split(';') if ':' in part)
        styles = {k.strip(): v.strip() for k, v in styles.items()}
        if stroke == 'none':
            styles.update(stroke=fill, **{'stroke-width': f'{weight / scale:.5f}',
                                        'stroke-opacity': props.get('fill-opacity', '1')})
        else:
            try:
                width = float(props.get('stroke-width', '1'))
            except ValueError:
                continue
            styles['stroke-width'] = f'{width + weight / scale:.5f}'
        styles.update({'stroke-linecap': 'round', 'stroke-linejoin': 'round'})
        element.set('style', ';'.join(f'{k}:{v}' for k, v in styles.items()))
        changed = True
    if changed:
        tree.write(path, encoding='unicode', xml_declaration=False)
    return changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path.home() / '.local/share/icons')
    parser.add_argument('--output', type=Path, default=ROOT / 'icons')
    args = parser.parse_args()
    counts = Counter()
    changed = []
    for suffix in ('', '-light', '-dark'):
        original = args.source / ('MacTahoe-blue' + suffix)
        target = args.output / ('SequoiaTahoe' + suffix)
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(original, target, symlinks=True, ignore=shutil.ignore_patterns('icon-theme.cache'))
        # Retarget the upstream family's shared-directory links to our bundled
        # family, so this also works without the user's original theme installed.
        for link in target.rglob('*'):
            if link.is_symlink():
                value = link.readlink().as_posix().replace('MacTahoe-blue', 'SequoiaTahoe')
                link.unlink()
                link.symlink_to(value)
        index = target / 'index.theme'
        text = index.read_text().replace('Name=MacTahoe-blue' + suffix,
            'Name=Sequoia Tahoe' + (suffix.replace('-', ' ').title() if suffix else ''))
        text = text.replace('Comment=MacOSX Tahoe style icon theme for linux.',
            'Comment=MacTahoe icons with smooth, heavier action and status glyphs for Sequoia Eleven.')
        index.write_text(text)
        add_app_aliases(target)
        for context in CONTEXTS:
            directory = target / context
            if directory.is_symlink():
                continue
            for path in sorted(directory.rglob('*.svg')):
                if not path.is_symlink() and embolden(path):
                    counts[target.name] += 1
                    changed.append(str(path.relative_to(args.output)))
        (target / 'MODIFICATIONS.md').write_text(
            '# Sequoia Tahoe\n\nDerived from Vince Liuice’s MacTahoe-blue icon theme.\n'
            'See AUTHORS for attribution and COPYING for the GPL-3.0 license.\n\n'
            'Modified 2026-09-21 for Sequoia Eleven: small monochrome action, status,\n'
            'device, category, place and emblem contours receive proportional extra\n'
            'weight and round caps/joins. Full-color application/folder art is retained.\n'
            'Theme names and shared-family links are updated for independent installation.\n'
            'Reproducible transformation: tools/build-tahoe-icons.py in the parent project.\n')
    (args.output / 'bold-icons.json').write_text(json.dumps({'counts': dict(counts), 'modified': changed}, indent=2) + '\n')
    print(json.dumps(dict(counts)))


if __name__ == '__main__':
    main()
