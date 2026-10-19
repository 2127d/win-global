"""Adapt the installed Merkuro Calendar's QML, preserving its package notices."""
import hashlib
import os
from pathlib import Path
import shlex
import subprocess


def build(root, destination, fingerprints, replace, background):
    binary = Path('/usr/bin/merkuro-calendar')
    if not binary.exists():
        return
    helper = root / '.staging/export-merkuro-qml.so'
    source = root / 'tools/export-merkuro-qml.cpp'
    helper.parent.mkdir(exist_ok=True)
    if not helper.exists() or helper.stat().st_mtime < source.stat().st_mtime:
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core'], text=True))
        subprocess.run(['c++', '-shared', '-fPIC', str(source), '-o', str(helper), *flags], check=True)
    module = destination / 'org/kde/merkuro/calendar'
    # LD_PRELOAD splits on spaces and colons and has no quoting syntax. An
    # inherited descriptor also works when the extracted checkout contains either.
    with helper.open('rb') as preload:
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen',
                   LD_PRELOAD=f'/proc/self/fd/{preload.fileno()}',
                   SEQUOIA_EXPORT_MERKURO=str(module))
        subprocess.run([str(binary)], env=env, pass_fds=(preload.fileno(),), check=True, timeout=15)
    for path in module.rglob('*'):
        if path.is_file():
            path.chmod(0o644)
    fingerprints[str(binary)] = hashlib.sha256(binary.read_bytes()).hexdigest()
    bridge_source = root / 'themes/application-style/quick/merkuro-qml.cpp'
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Qml'], text=True))
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(bridge_source), '-o',
                    str(destination / 'merkuro-qml.so'), *flags, '-ldl'], check=True)
    qmldir = module / 'qmldir'
    qmldir.write_text(''.join(line for line in qmldir.read_text().splitlines(True) if not line.startswith('prefer ')))

    def patch(relative, transform):
        path = module / 'qml' / relative
        path.write_text(transform(path.read_text()))

    island_import = 'import org.sequoia.eleven as Sequoia\n'
    for name in ('MonthView', 'HourlyView', 'ScheduleView', 'TodoView'):
        patch(f'MainViews/{name}.qml', lambda t:
              background(replace(t.replace('import QtQuick\n', 'import QtQuick\n' + island_import),
                                 '    padding: 0', '    padding: 12\n    contentItem.clip: true'),
                         '''    background: Sequoia.Island {
        objectName: "sequoiaMerkuroCalendar"
        color: Kirigami.Theme.backgroundColor
    }'''))

    def drawer(t):
        # Merkuro computes its compact width for an unpadded drawer. Reserve
        # the island margins as well so the navigation icons keep their space.
        t = replace(t,
                    '    readonly property int collapsedWidth: menu.Layout.minimumWidth + Kirigami.Units.smallSpacing',
                    '    readonly property int collapsedWidth: menu.Layout.minimumWidth + Kirigami.Units.smallSpacing + leftPadding + rightPadding')
        t = replace(t, '    leftPadding: 0\n    rightPadding: 0\n    topPadding: 0\n    bottomPadding: 0',
                    '    leftPadding: 12\n    rightPadding: 12\n    topPadding: 12\n    bottomPadding: 12')
        t = replace(t, '            id: toolbar', '            id: toolbar\n            background: Item {}')
        return t
    patch('Drawers/MainDrawer.qml', drawer)
    # Kirigami's content anchor omits the optional native menu bar. Merkuro
    # already compensates its drawer; align the calendar header with that drawer.
    patch('Main.qml', lambda t: replace(t, '    id: root', '''    id: root
    contentItem.anchors.topMargin: (header?.visible ? header.height : 0)
        + (menuBar?.visible ? menuBar.height : 0)'''))
