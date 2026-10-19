#!/usr/bin/env python3
"""Build independently named Plasma packages and five distributable archives."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import tarfile
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
IDS = {'tray': 'org.sequoia.systemtray', 'taskmanager': 'org.sequoia.icontasks.animated'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(destination):
    from PySide6.QtCore import QCoreApplication, QPluginLoader, qVersion
    app = QCoreApplication.instance() or QCoreApplication([])
    plugin_root = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
    applets = plugin_root / 'plasma/applets'
    spec = importlib.util.spec_from_file_location('overlay_builder', ROOT / 'tools/install-control-center.py')
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core'], text=True))
    flags += ['-I/usr/include/KF6/KCoreAddons', '-lKF6CoreAddons']
    moc = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_LIBEXECS'], text=True).strip()) / 'moc'
    with tempfile.TemporaryDirectory(prefix='sequoia-forks-') as temporary:
        temp = Path(temporary)
        overlay = temp / 'overlay'
        builder.build(overlay)
        folders = sorted(p.name for p in overlay.iterdir() if p.is_dir() and (p / 'main.qml').is_file())
        upstream = {short: 'org.kde.plasma.' + ('systemtray' if short == 'tray' else short) for short in folders}
        upstream['contacts'] = 'org.kde.merkuro.contact.applet'
        ids = {short: IDS.get(short, 'org.sequoia.' + short) for short in folders}
        replacements = {upstream[short]: ids[short] for short in folders}
        replacements['org.kde.plasma.icontasks'] = IDS['taskmanager']
        metadata = {short: QPluginLoader(str(applets / (upstream[short] + '.so'))).metaData()['MetaData'] for short in folders}
        known, enabled = [], []
        for path in sorted(applets.glob('*.so')):
            meta = QPluginLoader(str(path)).metaData().get('MetaData', {})
            if 'X-Plasma-NotificationAreaCategory' not in meta:
                continue
            known.append(path.stem)
            if meta.get('KPlugin', {}).get('EnabledByDefault', False):
                enabled.append(replacements.get(path.stem, path.stem))
        # Children are explicitly enabled only in the fork tray, not added to
        # every existing stock tray merely by installing these packages.
        child_names = [short for short in folders if short not in ('tray', 'taskmanager', 'trash')]
        known += list(ids.values())
        packages = temp / 'packages'; packages.mkdir()
        for short in folders:
            package = packages / ids[short]
            ui = package / 'contents/ui'
            shutil.copytree(overlay / short, ui)
            if short != 'taskmanager':
                shutil.copytree(overlay / 'details', package / 'contents/details')
            config = package / 'contents/config'; config.mkdir()
            for filename in ('main.xml', 'config.qml'):
                path = ui / filename
                if path.exists(): path.rename(config / filename)
            for path in package.rglob('*'):
                if path.suffix in ('.qml', '.js'):
                    text = path.read_text()
                    for old, new in replacements.items():
                        for quote in ('"', "'"):
                            text = text.replace(quote + old + quote, quote + new + quote)
                    if short == 'taskmanager' and 'TaskManagerApplet.LayoutMetrics' in text:
                        # The upstream module's script only recognizes KDE's
                        # icons-only ID. Use our rewritten script so spacing
                        # follows this fork's configuration, including live edits.
                        text = builder.replace(text, 'import QtQuick\n',
                                               'import QtQuick\nimport "LayoutMetrics.js" as LayoutMetrics\n')
                        text = text.replace('TaskManagerApplet.LayoutMetrics', 'LayoutMetrics')
                    path.write_text(text)
            meta = metadata[short]
            meta['KPlugin'] = {k: v for k, v in meta['KPlugin'].items() if not k.startswith(('Name[', 'Description['))}
            meta['KPlugin'].update(Id=ids[short], Version='2.1.1', EnabledByDefault=False,
                                  Name='Icons-Only Task Manager Animated' if short == 'taskmanager' else 'Sequoia ' + meta['KPlugin']['Name'])
            meta['KPackageStructure'] = 'Plasma/Applet'
            meta.pop('X-Plasma-RootPath', None)
            meta['X-Sequoia-NativePlugin'] = upstream[short]
            (package / 'metadata.json').write_text(json.dumps(meta, indent=2) + '\n')
            if short == 'tray':
                # Containment::compactApplet() looks up this KPackage entry,
                # not ui/CompactApplet.qml. Without it each tray child falls
                # back to the shell's standalone popup, competing with our
                # shared popup for focus and leaving an empty blurred window.
                compact = package / 'contents/applet/CompactApplet.qml'
                compact.parent.mkdir()
                compact.write_text((ui / 'CompactApplet.qml').read_text().replace(
                    'import QtQuick\n', 'import QtQuick\nimport "../ui"\n', 1))
                path = config / 'main.xml'
                tree = ET.parse(path)
                ns = {'k': 'http://www.kde.org/standards/kcfg/1.0'}
                for key, values in [('knownItems', known), ('extraItems', enabled)]:
                    tree.find(f'.//k:entry[@name="{key}"]/k:default', ns).text = ','.join(dict.fromkeys(values))
                ET.register_namespace('', ns['k'])
                tree.write(path, encoding='utf-8', xml_declaration=True)
            native = package / 'contents/native'; native.mkdir()
            shutil.copy2(ROOT / 'widgets/widget-forks/native-factory.cpp', native)
            native_meta = json.loads(json.dumps(meta))
            native_meta['KPlugin'].pop('Id')
            (native / 'native-metadata.json').write_text(json.dumps(native_meta, indent=2) + '\n')
            subprocess.run([str(moc), *[f for f in flags if f.startswith(('-I', '-D'))], str(native / 'native-factory.cpp'),
                            '-o', str(native / 'native-factory.moc')], check=True)
            subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(native / 'native-factory.cpp'),
                            '-o', str(native / (ids[short] + '.so')), *flags], check=True)
            shutil.copytree(ROOT / 'widgets/widget-forks/LICENSES', package / 'LICENSES')
            shutil.copy2(ROOT / 'LICENSE', package / 'LICENSES/MIT.txt')
        tray = packages / IDS['tray']
        bundled = tray / 'contents/bundled'; bundled.mkdir()
        for short in child_names + ['trash']:
            shutil.move(packages / ids[short], bundled / ids[short])
        # Each package carries its editable QML and the native adapter source.
        fingerprints = json.loads((overlay / 'manifest.json').read_text())['fingerprints']
        for short in ('tray', 'taskmanager'):
            package = packages / IDS[short]
            compatibility = fingerprints if short == 'tray' else {
                str(applets / 'org.kde.plasma.taskmanager.so'): digest(applets / 'org.kde.plasma.taskmanager.so')}
            manifest = {'format': 1, 'architecture': platform.machine(), 'qt_version': qVersion(),
                        'upstream_sha256': compatibility,
                        'files': {str(p.relative_to(package)): digest(p) for p in sorted(package.rglob('*')) if p.is_file()}}
            (package / 'sequoia-package.json').write_text(json.dumps(manifest, indent=2) + '\n')
        for name in ('org.sequoia.downloads', 'org.sequoia.dockseparator'):
            shutil.copytree(ROOT / 'widgets/plasmoids' / name, packages / name)
            shutil.copy2(ROOT / 'LICENSE', packages / name / 'LICENSE')
        shutil.copytree(ROOT / 'widgets/plasmoids/org.sequoia.startmenu', packages / 'org.sequoia.startmenu')
        destination.mkdir(parents=True, exist_ok=True)
        archives = {'downloads': 'org.sequoia.downloads', 'separator': 'org.sequoia.dockseparator',
                    'icons-only-task-manager-animated': IDS['taskmanager'], 'system-tray': IDS['tray'],
                    'start-menu': 'org.sequoia.startmenu'}
        for filename, name in archives.items():
            with tarfile.open(destination / (filename + '.tar.gz'), 'w:gz') as archive:
                archive.add(packages / name, arcname=name)
        shutil.copy2(ROOT / 'tools/install-widget-archives.py', destination / 'install-widgets.py')
        print(f'Built five widget archives and installer: {destination}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, default=ROOT / '.staging/widget-archives')
    build(parser.parse_args().destination.resolve())
