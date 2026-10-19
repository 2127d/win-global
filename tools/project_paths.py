"""Translate installed KDE asset paths to repository source paths.

Installation receipts always use KDE's layout; only sources are categorized.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def asset_source(relative, root=ROOT):
    relative = Path(relative)
    if relative.parts[:2] == ('plasma', 'plasmoids'):
        return root / 'widgets' / relative.relative_to('plasma')
    if relative.parts[:1] in {('icons',), ('wallpapers',), ('fonts',), ('color-schemes',)} or relative.parts[:2] in {
            ('plasma', 'desktoptheme'), ('plasma', 'look-and-feel')}:
        return root / 'themes' / relative
    raise ValueError(f'No repository asset source for {relative}')
