#!/usr/bin/env python3
"""Install or remove the settings navigation fork for this account, reversibly."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from theme_files import Receipt

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('settings_build', ROOT / 'tools/build-settings-navigation.py')
settings_build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(settings_build)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--bin-prefix', type=Path, default=Path.home() / '.local/bin')
    parser.add_argument('--binary', type=Path, help='Install an already built and verified binary instead of building.')
    parser.add_argument('--remove', action='store_true')
    parser.add_argument('--check', action='store_true', help='Validate removal without changing files.')
    parser.add_argument('--force', action='store_true', help='Replace/remove edited managed files.')
    parser.add_argument('--sdk', type=Path)
    args = parser.parse_args()
    if args.check and not args.remove:
        parser.error('--check requires --remove')
    data, bin_dir = args.prefix.absolute(), args.bin_prefix.absolute()
    root = Path(os.path.commonpath([data, bin_dir]))
    relative_data = data.relative_to(root)
    target = relative_data / 'sequoia-eleven/settings-navigation'
    launcher = bin_dir.relative_to(root) / 'systemsettings'
    desktop = relative_data / 'applications/org.sequoia.eleven.systemsettings.desktop'
    allowed = {str(launcher), str(desktop), str(target / 'systemsettings'), str(target / 'build.json')}
    receipt = Receipt(root, str(target / 'installation'), allowed.__contains__)
    receipt.validate(force=args.force)
    if args.check:
        return
    if args.remove:
        receipt.uninstall(force=args.force)
        print('Removed the custom settings navigation and restored the previous launcher.')
        return
    version = settings_build.installed_version()
    if version != settings_build.VERSION:
        raise RuntimeError(f'Installed KDE version {version} differs from the fork version {settings_build.VERSION}.')
    binary = args.binary.resolve() if args.binary else settings_build.build(ROOT / '.staging/settings-build', args.sdk)
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
    env.pop('LD_PRELOAD', None)
    result = subprocess.check_output([str(binary), '--version'], env=env, text=True, timeout=15)
    if result.strip().split()[-1] != version:
        raise RuntimeError('The selected binary does not match the installed KDE version.')
    with tempfile.TemporaryDirectory(prefix='sequoia-settings-install-') as temp:
        stage = Path(temp)
        (stage / target).mkdir(parents=True)
        (stage / launcher).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, stage / target / 'systemsettings')
        (stage / target / 'build.json').write_text(json.dumps({
            'version': version,
            'upstream_sha256': hashlib.sha256(Path('/usr/bin/systemsettings').read_bytes()).hexdigest(),
            'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
        }, indent=2) + '\n')
        source = (ROOT / 'apps/settings/launch.py').read_text()
        source = source.replace("'__SEQUOIA_SETTINGS_DATA__'", repr(str(data)))
        (stage / launcher).write_text(source)
        (stage / launcher).chmod(0o755)
        # A separately owned entry works even when ~/.local/bin is absent from
        # the desktop PATH, without competing with the theme's Quick receipt.
        executable = str(root / launcher).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%')
        (stage / desktop).parent.mkdir(parents=True, exist_ok=True)
        (stage / desktop).write_text('[Desktop Entry]\nType=Application\n'
            'Name=System Settings (Sequoia Eleven)\nIcon=preferences-system\n'
            'Exec="' + executable + '"\nTerminal=false\nCategories=Qt;KDE;Settings;\n')
        receipt.install(stage, force=args.force)
    print(f'Installed: {root / launcher}')
    print('Close System Settings, then open System Settings (Sequoia Eleven) from the application menu.')
    print('No settings modules or personal configuration were replaced.')


if __name__ == '__main__':
    main()
