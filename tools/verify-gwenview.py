#!/usr/bin/env python3
"""Verify the actual Gwenview override on an isolated session and capture its UI."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
import sys
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT / '.staging/gwenview-build/bin/gwenview')
parser.add_argument('--sidebar-preview', action='store_true')
parser.add_argument('--welcome-preview', action='store_true')
parser.add_argument('--browser-preview', action='store_true')
parser.add_argument('--fullscreen-preview', action='store_true')
parser.add_argument('--fullscreen-browser-preview', action='store_true')
parser.add_argument('--thumbnail-scroll', action='store_true')
parser.add_argument('--image-slide', action='store_true')
parser.add_argument('--panel-edges', action='store_true')
parser.add_argument('--popup-preview', action='store_true')
parser.add_argument('--dialog-preview', action='store_true')
parser.add_argument('--blur-disabled', action='store_true', help='Disable Blur only inside the private test compositor.')
parser.add_argument('--global-menu', action='store_true', help='Check Gwenview exports through the real private Plasma global-menu widget.')
parser.add_argument('--native-menus', action='store_true', help='Test actual popup grabs with real input on private KWin.')
parser.add_argument('--popup-lifecycle', action='store_true', help='Stress native popup hide/show and surface destruction.')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--transparent', action='store_true')
parser.add_argument('--vertical-thumbnails', action='store_true')
parser.add_argument('--debug', action='store_true')
parser.add_argument('--wayland-socket', type=Path, help='Use an isolated Wayland compositor instead of offscreen Qt.')
args = parser.parse_args()
if args.dialog_preview: args.popup_preview = True
if args.global_menu: args.native_menus = True
if args.native_menus: args.popup_lifecycle = True
if args.popup_lifecycle: args.popup_preview = True
helper = ROOT / '.staging/verify-gwenview.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test'], text=True))
if args.native_menus:
    flags += shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6DBus'], text=True))
    flags += ['-DGWENVIEW_NATIVE_MENU_INPUT']
    ei = subprocess.run(['pkg-config', '--cflags', '--libs', 'libei-1.0'], capture_output=True, text=True)
    if ei.returncode == 0:
        flags += shlex.split(ei.stdout)
    elif (ROOT / '.staging/test-sdk/libei.h').is_file():
        flags += ['-I' + str(ROOT / '.staging/test-sdk'), '-l:libei.so.1']
    else:
        raise RuntimeError('The private Wayland input test requires libei headers.')
subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-gwenview.cpp'), '-o', str(helper), *flags], check=True)
output = ROOT / ('preview/gwenview-' + ('dialogs-' if args.dialog_preview else 'popups-' if args.popup_preview else 'panel-edges-' if args.panel_edges else 'image-slide-' if args.image_slide else 'thumbnail-scroll-' if args.thumbnail_scroll else 'fullscreen-browser-' if args.fullscreen_browser_preview else 'fullscreen-' if args.fullscreen_preview else 'browser-' if args.browser_preview else 'welcome-' if args.welcome_preview else 'sidebar-') + ('dark' if args.dark else 'light') + ('-transparent' if args.transparent else '') if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll or args.image_slide or args.panel_edges or args.popup_preview else 'preview/gwenview'); output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='gwenview-test-', dir=ROOT / '.staging') as temp:
    base = Path(temp)
    env = dict(os.environ, KCOREADDONS_NO_XDG_DESKTOP_PORTAL='1', QT_NO_XDG_DESKTOP_PORTAL='1')
    if args.dialog_preview:
        # KDE helpers launched by dialog actions must not reach the live desktop.
        env.pop('DISPLAY', None)
        env.pop('WAYLAND_DISPLAY', None)
    if args.global_menu: env['SEQUOIA_GWENVIEW_GLOBAL_MENU'] = '1'
    env['SEQUOIA_GWENVIEW_THEMES'] = str(ROOT / 'themes/color-schemes')
    env['SEQUOIA_GWENVIEW_MATERIAL'] = 'transparent' if args.transparent else 'solid'
    env['SEQUOIA_GWENVIEW_TEST_BLUR'] = '0' if args.blur_disabled else '1'
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
        path = base / kind.lower(); path.mkdir(mode=0o700)
        env[f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'] = str(path)
    for name, color in [('one','steelblue'), ('two','coral')]:
        folder = base / name; folder.mkdir()
        Image.new('RGB',(120,80),color).save(folder / 'picture.png')
    env.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
               LD_PRELOAD=str(helper), SEQUOIA_GWENVIEW_VERIFY=str(output), SEQUOIA_GWENVIEW_FIXTURES=str(base), LC_ALL='C.UTF-8')
    if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll or args.image_slide or args.panel_edges or args.popup_preview:
        fixtures = [('one', (420,180), '#5783ad'), ('two', (180,420), '#e79463'),
                    ('three', (300,300), '#82a989'), ('four', (800,100), '#b497cd'),
                    ('five', (100,800), '#d1b369'), ('six', (90,60), '#729eaa')]
        if args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll or args.image_slide or args.panel_edges or args.popup_preview:
            gallery = base / 'Gallery'; gallery.mkdir()
            (gallery / 'Empty folder').mkdir()
            for index in range(2048 if args.thumbnail_scroll else 12):
                _, size, color = fixtures[index % len(fixtures)]
                picture = Image.new('RGB', size, color)
                draw = ImageDraw.Draw(picture)
                w, h = size
                draw.ellipse((w*.2, h*.2, w*.8, h*.8), fill='#f4eee2')
                picture.save(gallery / f'scene-{index:02d}.png')
            Image.new('RGB', (80, 80), 'coral').save(gallery / '.hidden.png')
        history = ['[Recent Files]']
        for number, (name, size, color) in enumerate(fixtures, 1):
            folder = base / name; folder.mkdir(exist_ok=True)
            picture = Image.new('RGB', size, color)
            draw = ImageDraw.Draw(picture)
            width, height = size
            draw.rectangle((0, 0, width//4, height), fill='#243347')
            draw.ellipse((width*.3, height*.2, width*.7, height*.8), fill='#f4eee2')
            picture.save(folder / 'picture.png')
            history += [f'File{number}={folder / "picture.png"}', f'Name{number}=picture.png']
        (base / 'config/gwenviewrc').write_text('\n'.join(history) + '\n')
        if args.vertical_thumbnails:
            with (base / 'config/gwenviewrc').open('a') as config:
                config.write('\n[General]\nThumbnailBarOrientation=Vertical\n')
        mode = 'Dark' if args.dark else 'Light'
        variant = 'Transparent' if args.transparent else 'Solid'
        (base / 'config/kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{mode}{variant}.colors').read_text())
        if args.dialog_preview:
            env['XDG_DATA_DIRS'] = str(ROOT / 'themes') + ':' + os.environ.get('XDG_DATA_DIRS', '/usr/local/share:/usr/share')
            with (base / 'config/kdeglobals').open('a') as config:
                config.write('\n[Icons]\nTheme=SequoiaTahoe' + ('-dark' if args.dark else '-light') + '\n')
        if os.environ.get('SEQUOIA_DIALOG_FILE_PICKER_ONLY'):
            with (base / 'config/kdeglobals').open('a') as config:
                config.write('\n[KFileDialog Settings]\nSpeedbar Width=285\nView Style=DetailTree\n')
        env.update(QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE='SequoiaEleven',
                   QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
    if args.wayland_socket:
        env.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY=str(args.wayland_socket.resolve()))
    if args.image_slide:
        large = base / 'Large'; large.mkdir()
        for number, color in enumerate(('#5783ad', '#e79463', '#82a989')):
            picture = Image.new('RGB', (8000, 6000), color)
            draw = ImageDraw.Draw(picture)
            draw.ellipse((2000, 1200, 6000, 4800), fill='#f4eee2')
            picture.save(large / f'large-{number}.jpg', quality=90)
    for mode in (('popups',) if args.popup_preview else ('panel-edges',) if args.panel_edges else ('image-slide',) if args.image_slide else ('thumbnail-scroll',) if args.thumbnail_scroll else ('fullscreen-browser',) if args.fullscreen_browser_preview else ('fullscreen',) if args.fullscreen_preview else ('browser', 'browser-restart') if args.browser_preview else ('welcome', 'welcome-restart', 'welcome-cleared') if args.welcome_preview else ('sidebar', 'bars-restart', 'bars-native-restart') if args.sidebar_preview else ('fresh', 'restart', 'disabled')):
        env['SEQUOIA_GWENVIEW_MODE'] = 'dialogs' if args.dialog_preview else 'global-menu' if args.global_menu else 'popup-lifecycle' if args.popup_lifecycle else mode
        if mode == 'disabled':
            config = base / 'config/gwenviewrc'
            text = config.read_text()
            if 'HistoryEnabled=' in text:
                import re
                text = re.sub(r'HistoryEnabled=.*', 'HistoryEnabled=false', text)
            elif '[General]' in text: text = text.replace('[General]', '[General]\nHistoryEnabled=false')
            else: text += '\n[General]\nHistoryEnabled=false\n'
            config.write_text(text)
        application = [str(args.binary.resolve())]
        if args.debug:
            application = ['gdb', '--batch', '-ex', 'set debuginfod enabled off', '-ex', 'run', '-ex', 'bt 35', '--args', *application]
        if args.native_menus:
            application = [sys.executable, str(ROOT / 'tools/run-gwenview-compositor.py'), *application]
        with (output / f'{mode}.log').open('w') as log:
            process = subprocess.Popen(['dbus-run-session', '--', *application], env=env, stdout=log, stderr=log, text=True, start_new_session=True)
            try:
                process.wait(timeout=200 if args.popup_lifecycle or args.dialog_preview else 60)
            finally:
                try: os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError: pass
                process.wait()
        text = (output / f'{mode}.log').read_text()
        print(text[-5000:])
        assert process.returncode == 0 and 'PASS:' in text

print(f'PASS: Gwenview GUI checks; captures in {output}')

if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll or args.image_slide or args.panel_edges or args.popup_preview: raise SystemExit(0)

# Check installation/removal and package-upgrade fallback outside the live account.
import importlib.util
with tempfile.TemporaryDirectory(prefix='gwenview-install-test-') as temp:
    base = Path(temp); data = base / 'user data'; bins = base / 'user bin'; bins.mkdir()
    launcher = bins / 'gwenview'
    launcher.write_text('#!/bin/sh\nexit 19\n'); launcher.chmod(0o755)
    command = ['python3', str(ROOT / 'tools/install-gwenview.py'), '--prefix', str(data), '--bin-prefix', str(bins), '--skip-build']
    subprocess.run(command, check=True)
    spec = importlib.util.spec_from_file_location('gwenview_launcher', ROOT / 'apps/gwenview/launch.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    arguments = ['image with spaces.png', '--fullscreen']
    actual = module.command(data, Path('/usr/bin/gwenview'), Path('/usr/lib64/libgwenviewlib.so.5'), arguments)
    assert actual == [str(data / 'sequoia-eleven/gwenview/gwenview'), *arguments]
    changed = base / 'changed-stock'; changed.write_text('package upgrade')
    assert module.command(data, changed, Path('/usr/lib64/libgwenviewlib.so.5'), arguments) == [str(changed), *arguments]
    dependencies = subprocess.check_output(['ldd', str(data / 'sequoia-eleven/gwenview/gwenview')], text=True)
    assert str(data / 'sequoia-eleven/gwenview/libgwenviewlib.so.5') in dependencies
    assert 'not found' not in dependencies and 'sequoia-gwenview-sdk' not in dependencies
    subprocess.run([*command, '--remove'], check=True)
    assert launcher.read_text() == '#!/bin/sh\nexit 19\n'
print('PASS: private library loading, argument forwarding, upgrade fallback, reversible installation')
