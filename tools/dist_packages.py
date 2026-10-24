"""Distribution inventory and shared packaging helpers."""
import hashlib
import importlib.util
from pathlib import Path
import shutil
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / 'dist'
PACKAGES = {
    'Sequoia-Eleven-Installer.tar.xz': 'Recommended complete installer, editable sources, grid/picker integration; builds native components on the destination machine.',
    'sequoia-widgets.tar.gz': 'Widget-only bundle: eight widget archives and installer. Native widgets require matching Qt/KDE. Grid placement needs the main installer’s Plasma style.',
    'icons-only-task-manager-animated.tar.gz': 'Standalone animated task manager, retaining the checked-in package and its native compatibility manifest.',
    'sequoia-eleven-light-2.0.0.tar.gz': 'Standalone light Plasma style.',
    'sequoia-eleven-dark-2.0.0.tar.gz': 'Standalone dark Plasma style.',
    'sequoia-eleven-wallpaper-2.0.0.tar.gz': 'Standalone adaptive wallpaper.',
    'sequoia-tahoe-icons-2.0.0.tar.gz': 'Standalone icon family with light/dark variants and licenses.',
    'widgets/start-menu.tar.gz': 'Standalone Sequoia Start Menu for the widget archive installer.',
    'widgets/downloads.tar.gz': 'Downloads widget; also included in the matching bundle.',
    'widgets/separator.tar.gz': 'Dock Separator widget; also included in the matching bundle.',
    'widgets/icons-only-task-manager-animated.tar.gz': 'Animated task manager with native compatibility manifest.',
    'widgets/system-tray.tar.gz': 'System Tray including its styled child widgets and Trash; native compatibility checked before installation.',
    'widgets/sequoia-analog-clock.plasmoid': 'Standalone Analog Clock: Sequoia and Original presets; install with kpackagetool6 --type Plasma/Applet --install PACKAGE.',
    'widgets/sequoia-digital-clock.plasmoid': 'Standalone Digital Clock review package, including settings and calendar; install with kpackagetool6.',
    'widgets/sequoia-grid-test.plasmoid': 'Optional grid development/test widget; install with kpackagetool6. Guided placement requires the matching grid/picker integration.',
}
QML_WIDGETS = {'sequoia-analog-clock': 'org.sequoia.analogclock',
               'sequoia-digital-clock': 'org.sequoia.digitalclock',
               'sequoia-grid-test': 'org.sequoia.gridtest'}


def module(name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'), ROOT / 'tools' / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def checksum(path):
    digest = hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()
    path.with_name(path.name + '.sha256').write_text(f'{digest}  {path.name}\n')


def package_tree(source, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(output, 'w:gz') as archive:
        archive.add(source, arcname=source.name, filter=lambda member: None if
                    '__pycache__' in Path(member.name).parts or member.name.endswith(('.pyc', '.pyo')) else member)
    checksum(output)


def assets():
    for mode in ('light', 'dark'):
        name = f'sequoia-eleven-{mode}'
        package_tree(ROOT / 'themes/plasma/desktoptheme' / name, DIST / f'{name}-2.0.0.tar.gz')
    package_tree(ROOT / 'themes/wallpapers/SequoiaEleven', DIST / 'sequoia-eleven-wallpaper-2.0.0.tar.gz')
    icons()


def icons():
    output = DIST / 'sequoia-tahoe-icons-2.0.0.tar.gz'
    with tarfile.open(output, 'w:gz') as archive:
        for theme in sorted((ROOT / 'themes/icons').glob('SequoiaTahoe*')):
            archive.add(theme, arcname=theme.name)
        archive.add(ROOT / 'tools/build-tahoe-icons.py', arcname='build-tahoe-icons.py')
        archive.add(ROOT / 'themes/icons/bold-icons.json', arcname='bold-icons.json')
        archive.add(ROOT / 'themes/icons/material-icons.json', arcname='material-icons.json')
        archive.add(ROOT / 'themes/icons/material-rounded', arcname='material-rounded')
        archive.add(ROOT / 'tools/tahoe_material.py', arcname='tahoe_material.py')
    checksum(output)


def widget_bundle():
    with tempfile.TemporaryDirectory(prefix='sequoia-dist-widgets-') as temporary:
        stage = Path(temporary) / 'widgets'
        module('build-widget-forks').build(stage)
        # The checked-in task manager includes newer animation changes than
        # the upstream overlay generator. Preserve that reviewed package.
        manager = 'icons-only-task-manager-animated.tar.gz'
        package_tree(ROOT / 'widgets/plasmoids/org.sequoia.icontasks.animated', stage / manager)
        for name in QML_WIDGETS:
            shutil.copy2(DIST / 'widgets' / (name + '.plasmoid'), stage / (name + '.plasmoid'))
        for archive in [*stage.glob('*.tar.gz'), *stage.glob('*.plasmoid')]:
            checksum(archive)
        module('install-widget-archives').unpack_archives(stage, Path(temporary) / 'validated')
        shutil.copy2(stage / manager, DIST / manager)
        checksum(DIST / manager)
        (stage / 'READ-ME.txt').write_text(
            'Eight independent widgets: System Tray (including children and Trash), '
            'animated task manager, Downloads, Dock Separator, Start Menu, Analog Clock, '
            'Digital Clock and Grid Test.\n\n'
            'Run python3 install-widgets.py --check, then python3 install-widgets.py.\n'
            'The installer reads all five .tar.gz packages and three .plasmoid packages.\n'
            'This folder is identical to the widgets/ folder inside sequoia-widgets.tar.gz.\n'
            'Native adapters are checked against the target Qt/KDE before installation.\n'
            'The grid/picker is a Plasma shell integration, not part of a widget package. '
            'For guided presets and snapping, install Plasma style from Sequoia-Eleven-Installer.tar.xz.\n'
            'Existing desktop clock instances are not migrated by this bundle.\n')
        destination = DIST / 'widgets'
        expected = {p.name for p in stage.iterdir()}
        obsolete = {name + suffix for name in QML_WIDGETS for suffix in ('.tar.gz', '.tar.gz.sha256')}
        unexpected = {p.name for p in destination.iterdir()} - expected - obsolete
        if unexpected:
            raise RuntimeError(f'Unrecognized widget distribution files: {sorted(unexpected)}')
        for name in obsolete:
            (destination / name).unlink(missing_ok=True)
        shutil.copytree(stage, destination, dirs_exist_ok=True)
        # Archive the published folder itself so formats, instructions, and
        # checksums cannot diverge between the two delivery options.
        package_tree(destination, DIST / 'sequoia-widgets.tar.gz')


def contents():
    (DIST / 'CONTENTS.txt').write_text(
        'SEQUOIA DISTRIBUTION PACKAGES\n\n'
        'Use Sequoia-Eleven-Installer.tar.xz for a complete installation.\n'
        'The other archives support separate component installation; they are optional.\n'
        'dist/widgets/ and the extracted widgets/ folder from sequoia-widgets.tar.gz contain identical files.\n'
        'Every archive has a matching .sha256 file. Verify it from that archive’s folder '
        'with sha256sum -c FILENAME.sha256.\n\n' +
        '\n\n'.join(f'{name}\n  {purpose}' for name, purpose in PACKAGES.items()) +
        '\n\nRebuild: python3 tools/build-dist.py\nAudit: python3 tools/verify-dist.py\n'
        'The obsolete desktop source archive and task-progress repair ZIP were retired. '
        'Their source/tools are available in the complete installer.\n')
