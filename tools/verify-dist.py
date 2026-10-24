#!/usr/bin/env python3
"""Audit the complete dist inventory, archive integrity, freshness and widget installs."""
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import posixpath
import subprocess
import tarfile
import tempfile
import xml.etree.ElementTree as ET
import zipfile

from dist_packages import ROOT, DIST, PACKAGES, QML_WIDGETS, module
from widget_picker import catalog


def verify_tar(archive):
    names = set()
    for member in archive.getmembers():
        path = PurePosixPath(member.name)
        assert not path.is_absolute() and '..' not in path.parts, member.name
        assert member.name not in names, f'Duplicate member: {member.name}'
        names.add(member.name)
        assert member.isfile() or member.isdir() or member.issym(), member.name
        if member.issym():
            target = posixpath.normpath(str(path.parent / member.linkname))
            assert not target.startswith(('/', '../')) and not PurePosixPath(member.linkname).is_absolute(), member.name
    return names


def compare_tar(archive, source_for, ignored=()):
    verify_tar(archive)
    for member in archive.getmembers():
        if member.name in ignored or member.isdir():
            continue
        source = source_for(member.name)
        assert source.exists(), f'Missing source for {member.name}'
        if member.issym():
            assert source.is_symlink() and os.readlink(source) == member.linkname, member.name
        else:
            assert source.read_bytes() == archive.extractfile(member).read(), f'Stale package member: {member.name}'


def main():
    expected = {'CONTENTS.txt', 'widgets/READ-ME.txt', 'widgets/install-widgets.py'} | set(PACKAGES) | {name + '.sha256' for name in PACKAGES}
    actual = {str(p.relative_to(DIST)) for p in DIST.rglob('*') if p.is_file()}
    assert actual == expected, f'Unexpected: {sorted(actual - expected)}; missing: {sorted(expected - actual)}'
    for name in PACKAGES:
        archive = DIST / name
        digest, filename = archive.with_name(archive.name + '.sha256').read_text().split()
        assert filename == archive.name and digest == hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest(), name
    definitions = catalog()
    with tempfile.TemporaryDirectory(prefix='sequoia-dist-check-') as temporary:
        stage = Path(temporary)
        for name, plugin in QML_WIDGETS.items():
            path = DIST / 'widgets' / (name + '.plasmoid')
            source = ROOT / 'widgets/plasmoids' / plugin
            files = {str(p.relative_to(source)): p.read_bytes() for p in source.rglob('*') if p.is_file()}
            if plugin == 'org.sequoia.gridtest':
                files['LICENSE'] = (ROOT / 'LICENSE').read_bytes()
            with zipfile.ZipFile(path) as archive:
                assert archive.testzip() is None, path
                assert len(archive.namelist()) == len(set(archive.namelist())), path
                assert {name: archive.read(name) for name in archive.namelist()} == files, f'Stale/incomplete {path}'
                meta = json.loads(archive.read('metadata.json'))
                assert meta['KPlugin']['Id'] == plugin and meta['KPackageStructure'] == 'Plasma/Applet', plugin
                assert 'contents/ui/main.qml' in files
                assert any(name == 'LICENSE' or name.startswith('LICENSES/') for name in files), plugin
                if 'contents/config/main.xml' in files:
                    ET.fromstring(files['contents/config/main.xml'])
                if meta.get('X-Sequoia-Grid'):
                    assert plugin in definitions and 'grid.json' in files
            env = dict(os.environ, HOME=str(stage), XDG_DATA_HOME=str(stage / 'data'),
                       XDG_CONFIG_HOME=str(stage / 'config'), XDG_CACHE_HOME=str(stage / 'cache'),
                       DBUS_SESSION_BUS_ADDRESS='unix:path=' + str(stage / 'no-live-session'))
            subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet', '--install', str(path),
                            '--packageroot', str(stage / 'plasmoids')], env=env, check=True,
                           capture_output=True, text=True)
            for relative, content in files.items():
                assert (stage / 'plasmoids' / plugin / relative).read_bytes() == content
        with tarfile.open(DIST / 'sequoia-widgets.tar.gz') as bundle:
            verify_tar(bundle)
            bundle.extractall(stage / 'bundle', filter='data')
        widgets = stage / 'bundle/widgets'
        published = {str(p.relative_to(DIST / 'widgets')): p.read_bytes() for p in (DIST / 'widgets').rglob('*') if p.is_file()}
        extracted = {str(p.relative_to(widgets)): p.read_bytes() for p in widgets.rglob('*') if p.is_file()}
        assert published == extracted, 'dist/widgets and the extracted bundle must match byte for byte'
        archives = [*widgets.glob('*.tar.gz'), *widgets.glob('*.plasmoid')]
        assert len(archives) == 8, 'Widget bundle must contain the five existing widgets plus three new widgets'
        assert (widgets / 'install-widgets.py').read_bytes() == (ROOT / 'tools/install-widget-archives.py').read_bytes()
        for archive in archives:
            checksum = archive.with_name(archive.name + '.sha256').read_text().split()[0]
            assert checksum == hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest(), archive
        packages = module('install-widget-archives').unpack_archives(widgets, stage / 'unpacked')
        assert set(QML_WIDGETS.values()) <= packages.keys()
        for name, source in packages.items():
            assert (source / 'LICENSE').is_file() or any((source / 'LICENSES').glob('*.txt')), f'Missing license: {name}'
            checked_in = ROOT / 'widgets/plasmoids' / name
            if checked_in.exists():
                for original in checked_in.rglob('*'):
                    if original.is_file():
                        assert (source / original.relative_to(checked_in)).read_bytes() == original.read_bytes(), str(original)
        for archive_name in ('icons-only-task-manager-animated.tar.gz', 'start-menu.tar.gz'):
            standalone = DIST / ('widgets' if archive_name == 'start-menu.tar.gz' else '') / archive_name
            assert standalone.read_bytes() == (widgets / archive_name).read_bytes(), archive_name
    for mode in ('light', 'dark'):
        with tarfile.open(DIST / f'sequoia-eleven-{mode}-2.0.0.tar.gz') as archive:
            compare_tar(archive, lambda name: ROOT / 'themes/plasma/desktoptheme' / name)
    with tarfile.open(DIST / 'sequoia-eleven-wallpaper-2.0.0.tar.gz') as archive:
        compare_tar(archive, lambda name: ROOT / 'themes/wallpapers' / name)
    with tarfile.open(DIST / 'sequoia-tahoe-icons-2.0.0.tar.gz') as archive:
        compare_tar(archive, lambda name: ROOT / ('tools' if name in {'build-tahoe-icons.py', 'tahoe_material.py'} else 'themes/icons') / name)
    exporter = module('export-installer')
    with tarfile.open(DIST / 'Sequoia-Eleven-Installer.tar.xz') as archive:
        compare_tar(archive, lambda name: ROOT / Path(name).relative_to(exporter.NAME),
                    ignored=(exporter.NAME + '/START-HERE.txt',))
        names = {m.name for m in archive.getmembers()}
        for plugin in QML_WIDGETS.values():
            for path in (ROOT / 'widgets/plasmoids' / plugin).rglob('*'):
                if path.is_file():
                    assert exporter.NAME + '/' + str(path.relative_to(ROOT)) in names, str(path)
        assert archive.extractfile(exporter.NAME + '/START-HERE.txt').read().decode() == exporter.INTRO
    print(f'PASS: all {len(PACKAGES)} distribution packages have a purpose, valid checksums and current payloads; '
          'three plasmoids install in isolation, widget folder and bundle match byte for byte, native manifests match this system')


if __name__ == '__main__':
    main()
