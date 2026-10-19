#!/usr/bin/env python3
"""Verify appearance sync, inherited KDE defaults, and preservation of unrelated settings."""
from pathlib import Path
import tempfile

from login_theme import apply_appearance, desktop_mode, read_config

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sequoia-login-check-') as temporary:
    base = Path(temporary)
    desktop = base/'desktop'
    greeter = base/'greeter'
    (desktop/'kdedefaults').mkdir(parents=True)
    greeter.mkdir()
    (desktop/'kdedefaults/kdeglobals').write_text('[General]\nColorScheme=SequoiaElevenLight\n')
    (desktop/'kdeglobals').write_text('[KDE]\nLookAndFeelPackage=org.sequoia.eleven.light\n')
    assert desktop_mode(desktop) == 'light'
    (greeter/'kdeglobals').write_text('[Unrelated]\nKeep=original\n')
    (greeter/'plasmarc').write_text('[Unrelated]\nKeep=original\n')
    for mode in ('light', 'dark', 'light'):
        (desktop/'kdeglobals').write_text(f'[General]\nColorScheme=SequoiaEleven{mode.title()}\n')
        assert desktop_mode(desktop) == mode
        apply_appearance(greeter, ROOT / 'themes', mode)
        config = read_config(greeter/'kdeglobals')
        palette = read_config(ROOT/'themes/color-schemes'/f'SequoiaEleven{mode.title()}.colors')
        assert config['Colors:Window']['BackgroundNormal'] == palette['Colors:Window']['BackgroundNormal']
        assert config['General']['ColorScheme'] == f'SequoiaEleven{mode.title()}'
        assert config['General']['font'].startswith('Adwaita Sans,10,')
        assert config['Unrelated']['Keep'] == 'original'
        plasma = read_config(greeter/'plasmarc')
        assert plasma['Theme']['name'] == f'sequoia-eleven-{mode}'
        assert plasma['Unrelated']['Keep'] == 'original'
        modified = (greeter/'kdeglobals').stat().st_mtime_ns
        apply_appearance(greeter, ROOT / 'themes', mode)
        assert (greeter/'kdeglobals').stat().st_mtime_ns == modified
    for mode in ('light', 'dark'):
        for material in ('Solid', 'Transparent'):
            (desktop/'kdeglobals').write_text(f'[General]\nColorScheme=SequoiaEleven{mode.title()}{material}\n')
            assert desktop_mode(desktop) == mode
    (desktop/'kdeglobals').write_text('[General]\nColorScheme=BreezeDark\n')
    assert desktop_mode(desktop) is None
    (desktop/'kdeglobals').unlink()
    (desktop/'kdedefaults/kdeglobals').unlink()
    assert desktop_mode(desktop) is None
print('PASS: inherited defaults, light/dark/light switching, palette/font selection, unrelated settings, idempotence, other themes.')
