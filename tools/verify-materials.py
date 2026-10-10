#!/usr/bin/env python3
"""Verify that material presets install as complete, consistently paired themes."""
import configparser
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='sequoia-material-check-') as temporary:
    data = Path(temporary) / 'data with spaces'
    subprocess.run(['bash', str(ROOT / 'tools/install-theme-assets.sh'), '--prefix', str(data)], check=True)
    for mode in ('light', 'dark'):
        for material in ('solid', 'transparent'):
            scheme_id = f'SequoiaEleven{mode.title()}{material.title()}'
            package = data / 'plasma/look-and-feel' / f'org.sequoia.eleven.{mode}.{material}'
            metadata = json.loads((package / 'metadata.json').read_text())
            assert metadata['KPlugin']['Id'] == package.name
            defaults = configparser.ConfigParser()
            defaults.read(package / 'contents/defaults')
            assert defaults['kdeglobals][General']['ColorScheme'] == scheme_id
            decoration_id = defaults['kwinrc][org.kde.kdecoration2']['library']
            plugin = ROOT / 'application-style/SequoiaEleven/kdecoration' / f'sequoiaeleven-{material}.json'
            assert json.loads(plugin.read_text())['KPlugin']['Id'] == decoration_id
            scheme = configparser.ConfigParser()
            scheme.read(data / 'color-schemes' / f'{scheme_id}.colors')
            for role in ('Window', 'Header', 'View'):
                rgba = list(map(int, scheme[f'Colors:{role}']['BackgroundNormal'].split(',')))
                if material == 'solid' or role == 'View':
                    assert len(rgba) == 3 or rgba[3] == 255
                else:
                    assert len(rgba) == 4 and 0 < rgba[3] < 255
            for group in scheme.sections():
                for key, value in scheme[group].items():
                    if key.startswith('foreground'):
                        assert len(value.split(',')) == 3, 'Text must remain opaque'
            assert len(scheme['Colors:Button']['BackgroundNormal'].split(',')) == 3
    assert (data / 'color-schemes/SequoiaElevenLight.colors').is_file()
    assert (data / 'plasma/look-and-feel/org.sequoia.eleven.dark').is_dir()
    for path in [*data.glob('color-schemes/SequoiaEleven*.colors'),
                 *data.glob('plasma/desktoptheme/sequoia-eleven-*/colors'),
                 ROOT / 'application-style/SequoiaEleven/colors/SequoiaEleven.colors']:
        scheme = configparser.ConfigParser()
        scheme.read(path)
        assert scheme.getfloat('KDE', 'frameContrast') == 0, path
        for key in ('BackgroundNormal', 'BackgroundAlternate'):
            assert scheme['Colors:View'][key].split(',')[:3] == scheme['Colors:Window'][key].split(',')[:3], (path, key)
print('PASS: all four installed material presets, decoration pairing, background alpha, opaque text/buttons, legacy themes')
print('PASS: zero frame contrast, matching view/window RGB, opaque content over transparent canvases')
