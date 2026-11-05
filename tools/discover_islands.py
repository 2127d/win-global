"""Build a version-checked Discover home page using its installed QML and models."""
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess

from PySide6.QtCore import QLibraryInfo


def build(root, destination, fingerprints):
    binary = Path('/usr/bin/plasma-discover')
    if not binary.exists():
        return
    helper = root / '.staging/export-discover-qml.so'
    source = root / 'tools/export-discover-qml.cpp'
    helper.parent.mkdir(exist_ok=True)
    if not helper.exists() or helper.stat().st_mtime < source.stat().st_mtime:
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core'], text=True))
        subprocess.run(['c++', '-shared', '-fPIC', str(source), '-o', str(helper), *flags], check=True)

    module = destination / 'org/kde/discover/qml'
    with helper.open('rb') as preload:
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen',
                   LD_PRELOAD=f'/proc/self/fd/{preload.fileno()}',
                   SEQUOIA_EXPORT_DISCOVER=str(module))
        subprocess.run([str(binary)], env=env, pass_fds=(preload.fileno(),), check=True, timeout=15)
    for path in module.rglob('*'):
        if path.is_file():
            path.chmod(0o644)

    home = (module / 'BrowsingPage.qml').read_text()
    navigation = (module / 'Navigation.qml').read_text()
    if not all(token in home for token in ('DiscoverApp.FeaturedModel', 'DiscoverApp.OdrsAppsModel',
                                           'DiscoverApp.LimitedRowCountProxyModel', 'recentlyUpdatedModelInstantiator')):
        raise RuntimeError('Discover home models changed; review the home page before refreshing.')
    if 'function openApplication(' not in navigation or 'function openApplicationList(' not in navigation:
        raise RuntimeError('Discover navigation changed; review the home page before refreshing.')

    def patch(name, old, new):
        path = module / name
        text = path.read_text()
        if text.count(old) != 1:
            raise RuntimeError(f'Discover {name} changed; review its header before refreshing.')
        path.write_text(text.replace(old, new, 1))

    # Opt only Discover into the shared toolbar's unframed title treatment.
    patch('DiscoverWindow.qml', '    property string currentTopLevel',
          '    readonly property bool sequoiaDiscoverHeaders: true\n\n    property string currentTopLevel')
    patch('DiscoverPage.qml', '    id: root',
          '    id: root\n\n    property real sequoiaTitleInset: leftPadding')
    patch('ApplicationsListPage.qml', '    id: page',
          '    id: page\n\n    sequoiaTitleInset: appsView.x + appsView.leftMargin')
    patch('DiscoverDrawer.qml', '    header: Kirigami.AbstractApplicationHeader {',
          '''    header: Kirigami.AbstractApplicationHeader {
        minimumHeight: searchField.implicitHeight + 12
        preferredHeight: minimumHeight
        maximumHeight: minimumHeight
        leftPadding: 4
        rightPadding: 4''')
    patch('DiscoverDrawer.qml', '        contentItem: SearchField {',
          '        contentItem: SidebarSearchField {')
    patch('ActionListItem.qml', 'import QtQuick\n',
          'import QtQuick\nimport org.sequoia.eleven as Sequoia\n')
    patch('ActionListItem.qml', '    id: item',
          '''    id: item

    readonly property bool sequoiaDiscoverSidebar: true
    font.weight: action?.objectName === "discover" ? Font.Normal : Font.Bold''')
    patch('ActionListItem.qml', '''        KD.IconTitleSubtitle {
            Layout.fillWidth: true
            icon: icon.fromControlsIcon(item.icon)''', '''        Sequoia.SidebarIcon {
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            source: item.icon.name || item.icon.source
            selected: item.checked || item.highlighted
            hovered: item.hovered
        }
        KD.TitleSubtitle {
            Layout.fillWidth: true''')
    patch('ActionListItem.qml', '            selected: item.highlighted || item.pressed',
          '''            selected: false
            color: (item.checked || item.highlighted) ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor''')

    for path in (root / 'themes/application-style/quick/discover').glob('*.qml'):
        shutil.copy2(path, module / path.name)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Qml', 'Qt6Gui'], text=True))
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC',
                    str(root / 'themes/application-style/quick/discover-qml.cpp'),
                    '-o', str(destination / 'discover-qml.so'), *flags, '-ldl'], check=True)
    # The bridge uses this Qt constructor ABI as well as the app's QML resources.
    qt_qml = Path(QLibraryInfo.path(QLibraryInfo.LibrariesPath)) / 'libQt6Qml.so.6'
    for path in (binary, qt_qml):
        fingerprints[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
