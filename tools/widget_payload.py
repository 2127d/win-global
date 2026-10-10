"""Build native panel forks on the destination machine for the theme receipt."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NAMES = {'org.sequoia.' + name for name in (
    'systemtray', 'icontasks.animated', 'trash', 'volume', 'brightness', 'battery',
    'bluetooth', 'networkmanagement', 'notifications', 'clipboard', 'devicenotifier',
    'printmanager', 'cameraindicator', 'mediacontroller', 'weather', 'contacts')}
REQUIRED = {'org.sequoia.' + name for name in ('systemtray', 'icontasks.animated', 'trash')}


def module(filename):
    spec = importlib.util.spec_from_file_location(filename.replace('-', '_'), ROOT / 'tools' / (filename + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def allowed(name, plugin_dir):
    return (name in {f'usr/{plugin_dir}/plasma/applets/{n}.so' for n in NAMES}
            or any(name.startswith(f'usr/share/plasma/plasmoids/{n}/') for n in NAMES))


def validate(files, plugin_dir):
    packages = {n for n in NAMES if f'usr/share/plasma/plasmoids/{n}/metadata.json' in files}
    if not REQUIRED <= packages:
        raise RuntimeError('Native panel payload is missing the tray, task manager or trash.')
    if 'usr/share/plasma/plasmoids/org.sequoia.systemtray/contents/applet/CompactApplet.qml' not in files:
        raise RuntimeError('Native tray is missing its shared-popup compact applet component.')
    for name in packages:
        root = f'usr/share/plasma/plasmoids/{name}/'
        metadata = json.loads(files[root + 'metadata.json'].read_text())
        if metadata.get('KPlugin', {}).get('Id') != name or metadata.get('KPackageStructure') != 'Plasma/Applet':
            raise RuntimeError(f'Invalid native widget metadata: {name}')
        for entry in (root + 'contents/ui/main.qml', f'usr/{plugin_dir}/plasma/applets/{name}.so'):
            if entry not in files:
                raise RuntimeError(f'Incomplete native widget payload: {entry}')


def stage(destination, plugin_dir):
    with tempfile.TemporaryDirectory(prefix='sequoia-theme-widgets-') as temporary:
        work = Path(temporary)
        module('build-widget-forks').build(work / 'archives')
        packages = module('install-widget-archives').unpack_archives(work / 'archives', work / 'unpacked')
        for name, source in packages.items():
            if name not in NAMES:
                continue  # Pure QML layout widgets belong to the user's receipt.
            target = destination / 'usr/share/plasma/plasmoids' / name
            shutil.copytree(source, target)
            shutil.rmtree(target / 'contents/bundled', ignore_errors=True)
            native = destination / 'usr' / plugin_dir / 'plasma/applets'
            native.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / 'contents/native' / (name + '.so'), native / (name + '.so'))
