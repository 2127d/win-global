#!/usr/bin/env python3
"""Rebuild the three native Sequoia effects without installing or activating them."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    version = subprocess.check_output(['kwin_wayland', '--version'], text=True).strip().removeprefix('kwin ')
    if version not in ('6.8.2', '6.8.3'):
        parser.error(f'KWin {version} has not been reviewed for these blur effects')
    build = ROOT / '.staging' / ('kwin-' + version)
    for component in ('liquid-glass', 'modal-dimming', 'firefox-blur'):
        configure = ['cmake', '-S', str(ROOT / 'desktop/effects' / component),
                     '-B', str(build / component), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release']
        if component != 'liquid-glass':
            configure.append('-DSEQUOIA_BUILD_TESTS=ON')
        subprocess.run(configure, check=True)
        subprocess.run(['cmake', '--build', str(build / component), '--parallel', str(args.jobs)], check=True)
    from PySide6.QtCore import QPluginLoader
    plugins = [build / 'liquid-glass/lib/kwin/effects/plugins/sequoia_liquid_glass_v6.so',
               build / 'modal-dimming/lib/kwin/effects/plugins/sequoia_modal_dimming.so',
               build / 'firefox-blur/sequoia_firefox_blur_v2.so']
    for plugin in plugins:
        iid = QPluginLoader(str(plugin)).metaData().get('IID')
        if iid != 'org.kde.kwin.EffectPluginFactory' + version:
            raise RuntimeError(f'{plugin.name}: SDK/runtime mismatch: {iid}')
    print(f'Rebuilt and checked plugin ABI for KWin {version}: {build}')


if __name__ == '__main__':
    main()
