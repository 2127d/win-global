#!/usr/bin/env python3
"""Verify the actual Gwenview override on an isolated session and capture its UI."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
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
parser.add_argument('--dark', action='store_true')
parser.add_argument('--transparent', action='store_true')
parser.add_argument('--vertical-thumbnails', action='store_true')
parser.add_argument('--debug', action='store_true')
args = parser.parse_args()
helper = ROOT / '.staging/verify-gwenview.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test'], text=True))
subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-gwenview.cpp'), '-o', str(helper), *flags], check=True)
output = ROOT / ('preview/gwenview-' + ('thumbnail-scroll-' if args.thumbnail_scroll else 'fullscreen-browser-' if args.fullscreen_browser_preview else 'fullscreen-' if args.fullscreen_preview else 'browser-' if args.browser_preview else 'welcome-' if args.welcome_preview else 'sidebar-') + ('dark' if args.dark else 'light') + ('-transparent' if args.transparent else '') if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll else 'preview/gwenview'); output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='gwenview-test-', dir=ROOT / '.staging') as temp:
    base = Path(temp)
    env = dict(os.environ)
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
        path = base / kind.lower(); path.mkdir(mode=0o700)
        env[f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'] = str(path)
    for name, color in [('one','steelblue'), ('two','coral')]:
        folder = base / name; folder.mkdir()
        Image.new('RGB',(120,80),color).save(folder / 'picture.png')
    env.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
               LD_PRELOAD=str(helper), SEQUOIA_GWENVIEW_VERIFY=str(output), SEQUOIA_GWENVIEW_FIXTURES=str(base), LC_ALL='C.UTF-8')
    if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll:
        fixtures = [('one', (420,180), '#5783ad'), ('two', (180,420), '#e79463'),
                    ('three', (300,300), '#82a989'), ('four', (800,100), '#b497cd'),
                    ('five', (100,800), '#d1b369'), ('six', (90,60), '#729eaa')]
        if args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll:
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
        env.update(QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE='SequoiaEleven',
                   QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
    for mode in (('thumbnail-scroll',) if args.thumbnail_scroll else ('fullscreen-browser',) if args.fullscreen_browser_preview else ('fullscreen',) if args.fullscreen_preview else ('browser', 'browser-restart') if args.browser_preview else ('welcome', 'welcome-restart', 'welcome-cleared') if args.welcome_preview else ('sidebar', 'bars-restart', 'bars-native-restart') if args.sidebar_preview else ('fresh', 'restart', 'disabled')):
        env['SEQUOIA_GWENVIEW_MODE'] = mode
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
        with (output / f'{mode}.log').open('w') as log:
            process = subprocess.Popen(['dbus-run-session', '--', *application], env=env, stdout=log, stderr=log, text=True, start_new_session=True)
            try:
                process.wait(timeout=60)
            finally:
                try: os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError: pass
                process.wait()
        text = (output / f'{mode}.log').read_text()
        print(text[-5000:])
        assert process.returncode == 0 and 'PASS:' in text

print(f'PASS: Gwenview GUI checks; captures in {output}')

if args.sidebar_preview or args.welcome_preview or args.browser_preview or args.fullscreen_preview or args.fullscreen_browser_preview or args.thumbnail_scroll: raise SystemExit(0)

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
