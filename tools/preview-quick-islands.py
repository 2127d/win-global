#!/usr/bin/env python3
"""Capture a real KDE app in an isolated offscreen session with Quick islands."""
import argparse
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import uuid
import signal
import time
from quick_islands import ROOT, build
from typography import font

APPS = {'systemmonitor': 'plasma-systemmonitor', 'systemsettings': 'systemsettings', 'discover': 'plasma-discover',
        'kinfocenter': 'kinfocenter', 'settingspage': 'kcmshell6', 'merkuro': 'merkuro-calendar'}


def verify_card_opacity(tree, capture, scale):
    # Locate actual application cards, including nested layouts and scroll offsets.
    ancestors = []
    checked = 0
    for line in tree.splitlines():
        match = re.fullmatch(r'( *)(\S+) (.*?) ([-\d.]+),([-\d.]+) ([-\d.]+)x([-\d.]+)', line)
        if not match:
            continue
        indent, kind, _, x, y, width, height = match.groups()
        ancestors = ancestors[:len(indent)]
        x, y, width, height = map(float, (x, y, width, height))
        if ancestors:
            x += ancestors[-1][0]
            y += ancestors[-1][1]
        ancestors.append((x, y))
        if not kind.startswith('DefaultCardBackground') or width < 60 or height < 40:
            continue
        px, py = round((x + width / 2) * scale), round((y + height - 16) * scale)
        if not (3 < px < capture.width()-3 and 3 < py < capture.height()-3):
            continue
        alpha = sorted(capture.pixelColor(px+dx, py+dy).alpha()
                       for dx in range(-3, 4) for dy in range(-3, 4))[24]
        assert alpha >= 254, ('card', px, py, alpha)
        checked += 1
        if checked == 3:
            break
    assert checked > 0, 'No visible card backgrounds were checked'
    print(f'Checked {checked} opaque card backgrounds')


def verify_discover_sidebar(tree):
    # Inspect the actual painted style items, which can extend beyond their
    # delegate's background. Coordinates here are logical pixels, even at HiDPI.
    ancestors = []
    checked = 0
    for line in tree.splitlines():
        match = re.fullmatch(r'( *)(\S+) (.*?) ([-\d.]+),[-\d.]+ ([-\d.]+)x[-\d.]+', line)
        if not match:
            continue
        indent, kind, name, x, width = match.groups()
        depth = len(indent)
        ancestors = ancestors[:depth]
        absolute_x = float(x) + (ancestors[-1][2] if ancestors else 0)
        entry = (kind, name, absolute_x, float(width))
        drawer = next((item for item in ancestors if item[1] == 'DiscoverDrawer'), None)
        if drawer and kind == 'KQuickStyleItem' and any(
                item[0].startswith('DefaultListItemBackground') for item in ancestors):
            assert absolute_x >= drawer[2] + 6, f'Sidebar highlight crosses left border: {line}'
            assert absolute_x + float(width) <= drawer[2] + drawer[3] - 6, \
                f'Sidebar highlight crosses right border: {line}'
            checked += 1
        ancestors.append(entry)
    assert checked > 0, 'No Discover sidebar highlights were checked'
    print(f'Checked {checked} sidebar selection/hover surfaces inside the drawer border')


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', choices=APPS, default='systemmonitor')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--direct', action='store_true', help='Verify native-style activation without launcher QML environment overrides.')
parser.add_argument('--launch-via-kcm', action='store_true', help='Launch System Settings through the KCMLauncher API used by Plasma buttons (requires --direct).')
parser.add_argument('--material', choices=['solid', 'transparent'])
parser.add_argument('--scale', default='1')
parser.add_argument('--rtl', action='store_true')
parser.add_argument('--platform', choices=['offscreen', 'wayland'], default='offscreen')
parser.add_argument('--kcm', default='kcm_landingpage', help='Module to capture with --app settingspage')
parser.add_argument('--category', help='Discover category to capture instead of its home page')
parser.add_argument('--scroll-sidebar', action='store_true', help='Capture with the sidebar scrolled partway down')
parser.add_argument('--height', type=int, default=850, help='Preview window height in logical pixels')
parser.add_argument('--calendar-view', choices=['month', 'week', 'schedule', 'tasks'], default='month')
parser.add_argument('--collapse-sidebar', action='store_true', help='Capture Merkuro after collapsing its sidebar')
args = parser.parse_args()
if args.launch_via_kcm and (not args.direct or args.app != 'systemsettings'):
    parser.error('--launch-via-kcm requires --app systemsettings --direct')
application = shutil.which(APPS[args.app], path=os.defpath if args.direct else None)
if application is None:
    parser.error(f'{APPS[args.app]} is not installed')
if args.collapse_sidebar and args.app != 'merkuro':
    parser.error('--collapse-sidebar requires --app merkuro')
overlay = build(ROOT / '.staging/quick-islands')
helper = ROOT / '.staging/quick-preview-capture.so'
source = ROOT / 'tools/quick-preview-capture.cpp'
if not helper.exists() or helper.stat().st_mtime < source.stat().st_mtime:
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6QuickWidgets', 'Qt6Widgets', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-shared', '-fPIC', str(source), '-o', str(helper), *flags], check=True)
mode = 'dark' if args.dark else 'light'
suffix = '-' + args.material if args.material else ''
if args.direct:
    suffix += '-direct'
if args.launch_via_kcm:
    suffix += '-kcm-launch'
preview_name = args.kcm if args.app == 'settingspage' else args.app
if args.app == 'systemsettings' and args.kcm != 'kcm_landingpage':
    preview_name += '-' + args.kcm
if args.category:
    assert args.app == 'discover', '--category requires --app discover'
    preview_name += '-category'
if args.scroll_sidebar:
    preview_name += '-scrolled'
if args.app == 'merkuro':
    preview_name += '-' + args.calendar_view
    if args.collapse_sidebar:
        preview_name += '-collapsed'
output = ROOT / 'preview' / f'{preview_name}-islands-{mode}{suffix}.png'
with tempfile.TemporaryDirectory(prefix='sequoia-quick-preview-') as temporary:
    config = Path(temporary)
    colors = (ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}{args.material.title() if args.material else ""}.colors').read_text()
    (config / 'kdeglobals').write_text(colors + '\n[General]\nfont=' + font() + f'\n[Icons]\nTheme=SequoiaTahoe-{mode}\n')
    (config / 'data').mkdir()
    (config / 'data/icons').symlink_to(ROOT / 'themes/icons', target_is_directory=True)
    env = dict(os.environ, XDG_CONFIG_HOME=temporary, XDG_CACHE_HOME=str(config / 'cache'),
               XDG_DATA_HOME=str(config / 'data'), XDG_STATE_HOME=str(config / 'state'),
               QT_QPA_PLATFORM=args.platform, QT_QPA_PLATFORMTHEME='kde', QSG_RHI_BACKEND='opengl',
               QT_QUICK_BACKEND='software' if args.app == 'merkuro' and args.platform == 'offscreen' else 'rhi', QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
               QT_STYLE_OVERRIDE='SequoiaEleven', QML_IMPORT_PATH=str(overlay), QT_SCALE_FACTOR=args.scale,
               QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
    env.pop('LD_PRELOAD', None)
    if args.direct:
        quick = config / 'data/sequoia-eleven/quick-islands'
        quick.mkdir(parents=True)
        (quick / 'qml').symlink_to(overlay, target_is_directory=True)
        for key in ('QML_IMPORT_PATH', 'QML2_IMPORT_PATH', 'QT_QUICK_CONTROLS_STYLE', 'SEQUOIA_MERKURO_QML'):
            env.pop(key, None)
        # KCMLauncher resolves commands via PATH. Exclude user wrappers so this
        # test proves the native style works on its own.
        env['PATH'] = os.defpath
    if args.app == 'merkuro':
        env['AKONADI_INSTANCE'] = 'sequoia-preview-' + uuid.uuid4().hex[:10]
        if not args.direct:
            env['SEQUOIA_MERKURO_QML'] = str(overlay / 'org/kde/merkuro/calendar')
    preload = str(helper) + (':' + str(overlay / 'merkuro-qml.so') if args.app == 'merkuro' and not args.direct else '')
    command = ['dbus-run-session', '--', 'python3', str(ROOT / 'tools/quick-preview-session.py'),
               'env', f'LD_PRELOAD={preload}', f'SEQUOIA_CAPTURE_PATH={output}']
    command.append('SEQUOIA_VERIFY_SCROLL_CLIPPING=1')
    command.append('SEQUOIA_CAPTURE_APP=' + Path(application).resolve().name)
    command.append(f'SEQUOIA_CAPTURE_HEIGHT={args.height}')
    if args.scroll_sidebar:
        command.append('SEQUOIA_SCROLL_SIDEBAR=1')
    if args.material and args.app in ('discover', 'systemmonitor', 'merkuro'):
        command.append(f'SEQUOIA_EXPECT_WINDOW_TINT={184 if args.material == "transparent" else 255}')
    if args.app == 'merkuro':
        command.append('SEQUOIA_MERKURO_VIEW=' + args.calendar_view)
        if args.collapse_sidebar:
            command.append('SEQUOIA_COLLAPSE_MERKURO=1')
    if args.app == 'systemmonitor':
        command.append('SEQUOIA_VERIFY_NAVIGATION=1')
    if args.app in ('systemsettings', 'kinfocenter'):
        command.append('SEQUOIA_VERIFY_SETTINGS_SIDEBAR=1')
    if args.launch_via_kcm:
        from PySide6.QtCore import QLibraryInfo
        launcher = config / 'launch-settings.qml'
        launcher.write_text('import QtQuick\nimport org.kde.kcmutils\nItem {\n'
                           '    Component.onCompleted: KCMLauncher.openSystemSettings("kcm_landingpage")\n'
                           '    Timer { interval: 12000; running: true; onTriggered: Qt.quit() }\n}\n')
        command.extend([str(Path(QLibraryInfo.path(QLibraryInfo.BinariesPath)) / 'qml'), str(launcher)])
    else:
        command.append(application)
    if args.app == 'settingspage':
        command.append(args.kcm)
    elif args.app == 'systemsettings' and args.kcm != 'kcm_landingpage':
        command.append(args.kcm)
    if args.category:
        command.extend(['--category', args.category])
    if args.rtl:
        command.append('-reverse')
    log = output.with_suffix('.log')
    # Activated D-Bus services may outlive the app; a regular log file avoids
    # waiting for inherited pipe descriptors after the capture has completed.
    with log.open('w') as stream:
        process = subprocess.Popen(command, env=env, stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            result = subprocess.CompletedProcess(command, process.wait(timeout=40))
        finally:
            # Shut down only this preview's process group, including activated
            # portals that could otherwise race temporary-directory cleanup.
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            time.sleep(0.3)
    if result.returncode != 0:
        raise RuntimeError(f'{args.app} preview failed ({result.returncode}); see {log}')
    errors = [line for line in log.read_text().splitlines() if any(token in line for token in
              ['TypeError:', 'ReferenceError:', 'is not a type', 'is not installed', 'Cannot assign', 'Binding loop'])]
    if errors:
        raise RuntimeError('\n'.join(errors))
    tree = Path(str(output) + '.tree').read_text()
    if args.app == 'merkuro' and not args.direct:
        assert 'sequoiaMerkuroCalendar' in tree, 'Merkuro still uses its embedded unstyled page'
        expected_view = {'month': 'MonthView_', 'week': 'HourlyView_', 'schedule': 'ScheduleView_', 'tasks': 'TodoView_'}[args.calendar_view]
        assert expected_view in tree, ('Calendar navigation did not select', args.calendar_view)
        from PySide6.QtGui import QImage
        capture = QImage(str(output))
        scale = float(args.scale)
        sidebar_inset = round((24 if args.collapse_sidebar else 100) * scale)
        sidebar_x = capture.width() - sidebar_inset if args.rtl else sidebar_inset
        assert capture.pixelColor(sidebar_x, round(capture.height() * 0.65)).alpha() == 255
        margin_x = capture.width() - 1 if args.rtl else 0
        expected = 184 if args.material == 'transparent' else 255
        for fraction in (0.15, 0.4, 0.6, 0.85):
            assert abs(capture.pixelColor(margin_x, round(capture.height() * fraction)).alpha() - expected) <= 1
        print('Merkuro: floating calendar loaded, sidebar opaque, outer canvas matches material')
    assert ('sequoiaCanvas' if args.app == 'settingspage' else 'sequoiaIsland') in tree, f'The app did not load the floating components; see {log}'
    if args.app in ('settingspage', 'systemsettings', 'kinfocenter'):
        canvases = re.findall(r' sequoiaCanvas [-\d.]+,[-\d.]+ ([-\d.]+)x([-\d.]+)', tree)
        assert any(float(w) > 300 and float(h) > 100 for w, h in canvases), 'Settings page has no laid-out canvas'
        if args.app in ('systemsettings', 'kinfocenter'):
            navigation = re.search(r'CategoriesPage\S*  [-\d.]+,[-\d.]+ ([\d.]+)x([\d.]+)', tree)
            assert navigation and all(float(size) > 100 for size in navigation.groups()), 'Navigation page is not laid out'
        if args.material:
            from PySide6.QtGui import QImage
            capture = QImage(str(output))
            scale = float(args.scale)
            points = {'canvas': (capture.width() - round(30 * scale), round(capture.height() * 0.7))}
            if args.app in ('systemsettings', 'kinfocenter'):
                sidebar_x = capture.width() - round(100 * scale) if args.rtl else round(100 * scale)
                points.update(sidebar=(sidebar_x, min(round(600 * scale), capture.height() - round(40 * scale))),
                              sidebar_join=(sidebar_x, round(44 * scale)),
                              toolbar=(capture.width() - round(50 * scale), round(20 * scale)))
                if args.rtl:
                    points['canvas'] = (round(30 * scale), round(capture.height() * 0.7))
                    points['toolbar'] = (round(50 * scale), round(20 * scale))
                # QWidget::grab repaints the native parent below the embedded
                # scene, combining both tints in this diagnostic image. The
                # C++ check independently requires the actual sidebar scene
                # backing to use exactly the Window palette color and alpha.
                margin_x = capture.width() - 1 if args.rtl else 0
                margin = capture.pixelColor(margin_x, round(capture.height() * 0.7)).getRgb()
                expected_alpha = 235 if args.material == 'transparent' else 255
                assert abs(margin[3] - expected_alpha) <= 1, ('sidebar margin', margin, expected_alpha)
                print(f'Sidebar outer background RGBA: {margin}')
            sampled_colors = {}
            for name, (px, py) in points.items():
                pixels = [capture.pixelColor(px+dx, py+dy).getRgb()
                          for dx in range(-3, 4) for dy in range(-3, 4)]
                sampled_colors[name] = tuple(sorted(pixel[i] for pixel in pixels)[24] for i in range(4))
                alpha = sampled_colors[name][3]
                expected = 184 if args.material == 'transparent' and name == 'canvas' else 255
                assert abs(alpha - expected) <= 1, (name, alpha, expected)
                print(f'{name} alpha: {alpha}/255')
    if args.material and args.app in ('discover', 'systemmonitor'):
        from PySide6.QtGui import QImage
        verify_card_opacity(tree, QImage(str(output)), float(args.scale))
    if args.app == 'discover':
        verify_discover_sidebar(tree)
        if args.material:
            from PySide6.QtGui import QImage
            capture = QImage(str(output))
            # Sample clear sidebar and page background areas between controls.
            # The item tree supplies logical geometry; the PNG uses device pixels.
            drawer_line = next(line for line in tree.splitlines() if ' DiscoverDrawer ' in line)
            match = re.search(r' ([-\d.]+),[-\d.]+ ([-\d.]+)x([-\d.]+)$', drawer_line)
            x, width, height = map(float, match.groups())
            scale = float(args.scale)
            for name, px, py in [('sidebar', x + width / 2, height * 0.47),
                                 ('page', 24 if args.rtl else capture.width() / scale - 24, height * 0.36)]:
                # Use the median of a small patch to ignore text/antialiased edges.
                alphas = sorted(capture.pixelColor(round(px * scale) + dx, round(py * scale) + dy).alpha()
                                for dx in range(-3, 4) for dy in range(-3, 4))
                alpha = alphas[len(alphas)//2]
                glass = args.material == 'transparent' and name == 'page'
                # Allow one alpha step of scene-graph gradient/blending rounding.
                assert (0 < alpha < 250) if glass else alpha >= 254, (name, alpha)
                print(f'{name} alpha: {alpha}/255')
    print(f'Rendered {args.app} with real floating components: {output}')
