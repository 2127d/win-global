#!/usr/bin/env python3
"""Verify and capture the actual settings fork without touching live configuration."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import signal
import shutil

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT / '.staging/settings-build/bin/systemsettings')
parser.add_argument('--skip-install-check', action='store_true', help='Use when --binary is the installed launcher.')
parser.add_argument('--startup', help='Exercise an existing module deep link on startup.')
parser.add_argument('--capture-group', help='Open and capture every native page in a category; use all for every category.')
parser.add_argument('--third-party', action='store_true')
parser.add_argument('--debug', action='store_true')
parser.add_argument('--composited', action='store_true', help='Render the real compositor backdrop and cycle material providers in the isolated session.')
parser.add_argument('--theme-plugin-path', type=Path, help='Test freshly built style and decoration plugins before system installation.')
parser.add_argument('--overlay', action='store_true', help='Include the installed Quick islands overlay.')
parser.add_argument('--hub', action='store_true', help='Verify the separate Appearance Hub preview.')
parser.add_argument('--layout-only', action='store_true', help='Check appearance geometry stability around the scrolling threshold.')
parser.add_argument('--corners-only', action='store_true', help='Check the application backdrop corners through page and native-window transitions.')
parser.add_argument('--screen-width', default='1400')
parser.add_argument('--screen-height', default='1000')
parser.add_argument('--desktop-preview', action='store_true', help='Verify real rendered desktop loading and rounded corners.')
parser.add_argument('--panels-only', action='store_true', help='Test panel controls against isolated Plasma panels')
parser.add_argument('--colors-only', action='store_true', help='Exercise the redesigned Colors page')
parser.add_argument('--plasma-only', action='store_true', help='Exercise the redesigned Plasma Style page')
parser.add_argument('--style-only', action='store_true', help='Exercise the redesigned Application Style page')
parser.add_argument('--global-only', action='store_true', help='Exercise the redesigned Global Theme page')
parser.add_argument('--quick-only', action='store_true', help='Exercise the redesigned native Quick Settings page')
parser.add_argument('--popup-only', action='store_true', help='Focus on the appearance dropdown interactions.')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--rtl', action='store_true')
parser.add_argument('--scale', default='1')
args = parser.parse_args()
helper = ROOT / '.staging/verify-settings-navigation.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6QuickWidgets', 'Qt6Quick', 'Qt6Test', 'Qt6DBus'], text=True))
subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-settings-navigation.cpp'), '-o', str(helper), '-I/usr/include/KF6/KWindowSystem', '-lKF6WindowSystem', '-I/usr/include/Plasma', '-I/usr/include/KF6/KConfigCore', '-I/usr/include/KF6', '-I/usr/include/KF6/KConfig', '-I/usr/include/KF6/Kirigami/Platform', '-lPlasma', *flags], check=True)
mode = 'dark' if args.dark else 'light'
output = ROOT / 'preview' / (('settings-hub-' if args.hub else 'settings-navigation-') + mode + ('-composited' if args.composited else '') + ('-third-party' if args.third_party else '') + ('-rtl' if args.rtl else '') + ('-' + args.scale if args.scale != '1' else ''))
output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='sequoia-settings-verification-') as temp:
    folder = Path(temp)
    env = dict(os.environ)
    if args.layout_only: env['SEQUOIA_VERIFY_LAYOUT'] = '1'
    if args.corners_only:
        assert args.hub and args.composited, '--corners-only requires --hub --composited'
        env['SEQUOIA_VERIFY_CORNERS'] = '1'
    if args.colors_only: env['SEQUOIA_COLORS_ONLY'] = '1'
    if args.plasma_only: env['SEQUOIA_PLASMA_ONLY'] = '1'
    if args.style_only: env['SEQUOIA_STYLE_ONLY'] = '1'
    if args.global_only: env['SEQUOIA_GLOBAL_ONLY'] = '1'
    if args.quick_only: env['SEQUOIA_QUICK_ONLY'] = '1'
    if args.popup_only: env['SEQUOIA_POPUP_ONLY'] = '1'
    if args.panels_only: env['SEQUOIA_PANELS_ONLY'] = '1'
    if args.hub: env['SEQUOIA_APPEARANCE_HUB'] = '1'
    else: env.pop('SEQUOIA_APPEARANCE_HUB', None)
    env['QT_FORCE_STDERR_LOGGING'] = '1'
    env['SEQUOIA_DESKTOP_RENDERING'] = '0'  # Real renderer is verified separately, never recursively.
    if args.overlay: env['QML_IMPORT_PATH'] = str(Path.home()/'.local/share/sequoia-eleven/quick-islands/qml')
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE'):
        path = folder / kind.lower(); path.mkdir()
        env[f'XDG_{kind}_HOME'] = str(path)
    (folder / 'config/kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}Solid.colors').read_text() + f'\n[Icons]\nTheme=SequoiaTahoe-{mode}\n')
    runtime = folder / 'runtime'; runtime.mkdir(mode=0o700)
    env['XDG_RUNTIME_DIR'] = str(runtime)
    defaults = folder / 'system-config'
    defaults.mkdir()
    (defaults / 'kdeglobals').write_text('[General]\nColorScheme=BreezeDark\n')
    env['XDG_CONFIG_DIRS'] = (str(folder / 'config/kdedefaults') + ':' if args.global_only else '') + str(defaults) + ':/etc/xdg'
    env['LIBGL_ALWAYS_SOFTWARE'] = '1'
    if args.third_party:
        external = folder / 'data/plasma/systemsettings/externalmodules'
        external.mkdir(parents=True)
        (external / 'sequoia-test-extra.desktop').write_text('[Desktop Entry]\nType=Application\nName=Additional Settings\nComment=Third-party category coverage test\nIcon=preferences-system\nExec=/bin/true\nX-KDE-System-Settings-Parent-Category=unregistered-vendor-category\n')
    if args.colors_only:
        shutil.copytree(ROOT/'themes/color-schemes',folder/'data/color-schemes')
        base=(ROOT/'themes/color-schemes/SequoiaElevenLightSolid.colors').read_text()
        (folder/'data/color-schemes/SequoiaTestRemovable.colors').write_text(base.replace('Name=Sequoia Eleven Light Solid','Name=Disposable test colors'))
        fixture=folder/'SequoiaTestInstalled.colors'
        fixture.write_text(base.replace('Name=Sequoia Eleven Light Solid','Name=Installed test colors'))
        env['SEQUOIA_COLORS_INSTALL_FIXTURE']=str(fixture)
    else:
        (folder / 'data/color-schemes').symlink_to(ROOT / 'themes/color-schemes', target_is_directory=True)
    (folder / 'data/icons').symlink_to(ROOT / 'themes/icons', target_is_directory=True)
    if args.style_only:
        fixture = folder / 'data/union/styles/sequoia-test-removable'
        shutil.copytree('/usr/share/union/styles/breeze', fixture)
        metadata = json.loads((fixture / 'metadata.json').read_text())
        metadata['name'] = 'Removable Union test style'
        (fixture / 'metadata.json').write_text(json.dumps(metadata))
    if args.global_only:
        shutil.copytree(ROOT / 'themes/plasma/look-and-feel', folder / 'data/plasma/look-and-feel')
        fixture = folder / 'data/plasma/look-and-feel/org.sequoia.test.removable'
        shutil.copytree(ROOT / 'themes/plasma/look-and-feel/org.sequoia.eleven.dark.transparent', fixture)
        metadata = json.loads((fixture / 'metadata.json').read_text())
        metadata['KPlugin'].update(Id='org.sequoia.test.removable', Name='Removable test theme')
        (fixture / 'metadata.json').write_text(json.dumps(metadata))
        layout = fixture / 'contents/layouts'
        layout.mkdir(exist_ok=True)
        (layout / 'org.kde.plasma.desktop-layout.js').write_text('// Isolated test layout; never applied.\n')
    if args.quick_only:
        packs = folder / 'data/plasma'
        packs.mkdir(parents=True, exist_ok=True)
        (packs / 'look-and-feel').symlink_to(ROOT / 'themes/plasma/look-and-feel', target_is_directory=True)
    if args.hub:
        looks = folder / 'data/systemsettings/appearance-looks'
        looks.mkdir(parents=True)
        for i in range(18):
            (looks / f'{i:02}.json').write_text(json.dumps({'name': f'Saved appearance {i+1} with a long descriptive name', 'material': 'glass', 'mode': 'dark'}))

    if args.composited:
        assert args.hub, '--composited is for the Appearance Hub'
        env['SEQUOIA_HUB_COMPOSITED'] = '1'
        env['KWIN_SCREENSHOT_NO_PERMISSION_CHECKS'] = '1'
        env['KWIN_COMPOSE'] = 'O2'
        env['SEQUOIA_HUB_WALLPAPER'] = str(Path.home()/'.local/share/wallpapers/MichiganGlassNatural/contents/images/3440x1440.png')
        (folder / 'config/kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}Transparent.colors').read_text())
        (folder / 'config/kwinrc').write_text('[Plugins]\nsequoia_liquid_glass_v6Enabled=true\nblurEnabled=false\n[Windows]\nPlacement=Centered\n[org.kde.kdecoration2]\nlibrary=org.kde.sequoiaeleven.glass-adjustable\n')
        (folder / 'config/plasma-org.kde.plasma.desktop-appletsrc').write_text('[Containments][1][Wallpaper][org.kde.image][General]\nImage=' + env['SEQUOIA_HUB_WALLPAPER'] + '\n')
        themes=folder/'data/plasma';themes.mkdir(parents=True,exist_ok=True)
        if not args.plasma_only:
            (themes/'desktoptheme').symlink_to(Path.home()/'.local/share/plasma/desktoptheme')

    if args.plasma_only:
        # Native removal/install must operate only on copies in this private data home.
        shutil.copytree(Path.home()/'.local/share/plasma/desktoptheme', folder/'data/plasma/desktoptheme')
        fixture = folder/'data/plasma/desktoptheme/sequoia-test-removable'
        shutil.copytree(ROOT/'themes/plasma/desktoptheme/sequoia-eleven-dark', fixture)
        metadata = json.loads((fixture/'metadata.json').read_text())
        metadata['KPlugin'].update(Id='sequoia-test-removable', Name='Disposable Plasma style')
        (fixture/'metadata.json').write_text(json.dumps(metadata))
        install_fixture = folder/'install/sequoia-test-installed'
        shutil.copytree(ROOT/'themes/plasma/desktoptheme/sequoia-eleven-light', install_fixture)
        metadata['KPlugin'].update(Id='sequoia-test-installed', Name='Installed Plasma test style')
        (install_fixture/'metadata.json').write_text(json.dumps(metadata))
        env['SEQUOIA_PLASMA_INSTALL_FIXTURE'] = shutil.make_archive(str(folder/'install-test'), 'gztar', folder/'install')

    if args.desktop_preview:
        assert args.hub
        env['SEQUOIA_VERIFY_DESKTOP'] = '1'
        env.pop('SEQUOIA_DESKTOP_RENDERING', None)
        env['XDG_DATA_DIRS'] = str(Path.home()/'.local/share') + ':' + os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share')
        spec=importlib.util.spec_from_file_location('desktop_renderer',ROOT/'apps/settings/appearance-desktop-render.py')
        renderer=importlib.util.module_from_spec(spec);spec.loader.exec_module(renderer)
        for name in renderer.CONFIG_FILES:
            source=Path.home()/'.config'/name
            if source.is_file():
                (folder/'config'/name).parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(source,folder/'config'/name)
        for name in renderer.STATE_FILES:
            source=Path.home()/'.local/state'/name
            if source.is_file(): shutil.copy2(source,folder/'state'/name)
        weather_cache=Path.home()/'.cache/kweather/cache'
        if weather_cache.is_dir(): shutil.copytree(weather_cache,folder/'cache/kweather/cache')

    env.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='kde',
               QT_QUICK_CONTROLS_STYLE='org.kde.desktop', QT_STYLE_OVERRIDE='SequoiaEleven', QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'),
               QT_SCALE_FACTOR=args.scale, LC_ALL='C.UTF-8')
    if args.composited: env.pop('QT_QUICK_BACKEND', None)
    if args.theme_plugin_path: env['QT_PLUGIN_PATH'] = str(args.theme_plugin_path.resolve()) + ':' + env['QT_PLUGIN_PATH']
    env.pop('LD_PRELOAD', None)
    if args.rtl: env['SEQUOIA_SETTINGS_RTL'] = '1'
    application = [str(args.binary.resolve())] + ([args.startup] if args.startup else [])
    if args.startup: env['SEQUOIA_SETTINGS_STARTUP'] = args.startup
    if args.capture_group: env['SEQUOIA_SETTINGS_CAPTURE_GROUP'] = args.capture_group
    if args.debug:
        application = ['gdb', '--batch', '-ex', 'set debuginfod enabled off', '-ex', 'run', '-ex', 'thread apply all bt', '--args', *application]
    launcher = folder / 'launch-test.sh'
    launcher.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\n'
                        + ('plasmashell --no-respawn > '+shlex.quote(str(output/'panels-shell.log'))+' 2>&1 &\n' if args.panels_only else '')
                        + '/usr/bin/systemsettings --list > ' + shlex.quote(str(output / 'stock-modules.txt')) + '\n'
                        + shlex.quote(str(args.binary.resolve())) + ' --list > ' + shlex.quote(str(output / 'fork-modules.txt')) + '\n'
                        + 'export LD_PRELOAD=' + shlex.quote(str(helper))
                        + '\nexport SEQUOIA_SETTINGS_VERIFY=' + shlex.quote(str(output))
                        + '\nexec ' + shlex.join(application) + '\n')
    launcher.chmod(0o755)
    bus = folder / 'bus.conf'
    bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir=' + str(runtime) + '</listen>'
                   '<policy context="default"><allow send_destination="*"/><allow receive_sender="*"/>'
                   '<allow own="*"/></policy></busconfig>')
    command = ['dbus-run-session', '--config-file=' + str(bus), '--', 'kwin_wayland', '--virtual', '--width', args.screen_width, '--height', args.screen_height,
               '--inputmethod', '/bin/true', '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities', '--socket', 'sequoia-settings-test',
               '--exit-with-session', str(launcher)]
    with (output / 'verification.log').open('w') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=log, text=True, start_new_session=True)
        try:
            process.wait(timeout=180 if args.capture_group == 'all' or args.composited or args.desktop_preview else 60)
        finally:
            try: os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError: pass
            process.wait()
    result = subprocess.CompletedProcess(command, process.returncode, (output / 'verification.log').read_text())
    print(result.stdout[-6000:])
    result.check_returncode()
    if args.plasma_only:
        assert 'PASS: Plasma Style' in result.stdout
        assert not any(error in result.stdout for error in ('TypeError:', 'ReferenceError:', 'Error loading QML', 'Unable to assign', 'Binding loop')), 'Plasma Style emitted QML errors'
        raise SystemExit(0)
    if args.style_only:
        assert 'PASS: Application Style' in result.stdout
        assert not any(error in result.stdout for error in ('TypeError:', 'ReferenceError:', 'Error loading QML', 'Unable to assign')), 'Application Style emitted QML errors'
        raise SystemExit(0)
    if args.colors_only:
        assert 'PASS: Colors native' in result.stdout
        assert not any(error in result.stdout for error in ('TypeError:', 'ReferenceError:', 'Error loading QML', 'Unable to assign')), 'Colors emitted QML errors'
        raise SystemExit(0)
    if args.global_only:
        assert 'PASS: Global Theme' in result.stdout
        assert not any(error in result.stdout for error in ('TypeError:', 'ReferenceError:', 'Error loading QML', 'Unable to assign')), 'Global Theme emitted QML errors'
        raise SystemExit(0)
    if args.quick_only:
        assert 'PASS: Quick Settings' in result.stdout
        raise SystemExit(0)
    if args.corners_only:
        assert 'PASS: square application top corners' in result.stdout
        raise SystemExit(0)
    if args.layout_only:
        assert 'PASS: stable appearance geometry' in result.stdout
        raise SystemExit(0)
    if args.desktop_preview:
        assert 'PASS: real desktop images and antialiased rounded corners' in result.stdout
        raise SystemExit(0)
    if args.panels_only:
        assert 'PASS: panel settings grid' in result.stdout
        raise SystemExit(0)
    if args.popup_only:
        assert 'PASS: below-button glass popup' in result.stdout
        raise SystemExit(0)
    assert 'unique modules; complete category and search coverage' in result.stdout, 'GUI checks did not complete'
    stock = (output / 'stock-modules.txt').read_text()
    fork = (output / 'fork-modules.txt').read_text()
    assert stock == fork, 'Upstream and fork module discovery differ'
    discovered = {line.split(' - ', 1)[0].strip() for line in stock.splitlines()[1:] if ' - ' in line}
    actual = {entry['id'] for entry in json.loads((output / 'inventory.json').read_text())}
    assert discovered == actual, f'Module coverage differs: missing={discovered-actual}, extra={actual-discovered}'
    print(f'PASS: all {len(actual)} upstream modules are reachable. Captures: {output}')

if args.skip_install_check:
    raise SystemExit(0)

# Installation must preserve an existing user launcher and tolerate upgrades.
with tempfile.TemporaryDirectory(prefix='sequoia-settings-install-check-') as temp:
    import importlib.util
    import hashlib
    base = Path(temp)
    data, bins = base / 'user data', base / 'user bin'
    bins.mkdir()
    original = '#!/bin/sh\nprintf "original launcher\\n"\n'
    user_launcher = bins / 'systemsettings'
    user_launcher.write_text(original)
    user_launcher.chmod(0o755)
    install = ['python3', str(ROOT / 'tools/install-settings-navigation.py'), '--prefix', str(data), '--bin-prefix', str(bins)]
    subprocess.run([*install, '--binary', str(args.binary.resolve())], check=True)
    subprocess.run([*install, '--binary', str(args.binary.resolve())], check=True)
    spec = importlib.util.spec_from_file_location('settings_launcher', ROOT / 'apps/settings/launch.py')
    launcher_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(launcher_module)
    command = launcher_module.command(data, Path('/usr/bin/systemsettings'), ['kcm_colors', '--args', 'argument with spaces'])
    assert command == [str(data / 'sequoia-eleven/settings-navigation/systemsettings'), 'kcm_colors', '--args', 'argument with spaces']
    fake_stock = base / 'updated-systemsettings'
    fake_stock.write_text('new KDE package')
    assert launcher_module.command(data, fake_stock, ['kcm_keyboard']) == [str(fake_stock), 'kcm_keyboard']
    subprocess.run([*install, '--remove'], check=True)
    assert user_launcher.read_text() == original, 'Removal did not restore the prior launcher'
    assert os.access(user_launcher, os.X_OK), 'Removal lost original launcher permissions'
    assert not (data / 'sequoia-eleven/settings-navigation/systemsettings').exists()
    print('PASS: install/reinstall/removal, original launcher restoration, argument forwarding, and upgrade fallback')
