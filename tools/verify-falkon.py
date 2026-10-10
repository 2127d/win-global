#!/usr/bin/env python3
"""Exercise the native Falkon makeover using an isolated profile and local pages."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import sqlite3
import subprocess
import tempfile
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'apps/falkon'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--private', action='store_true')
parser.add_argument('--style', default='Fusion', choices=['Fusion', 'SequoiaEleven'])
parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/falkon-native-build')
parser.add_argument('--sdk', type=Path, default=ROOT / '.staging/falkon-sdk/usr')
args = parser.parse_args()
build = args.build_dir.resolve()
helper = build / ('verify-falkon-' + ('dark' if args.dark else 'light') + ('-private' if args.private else '') + ('-native' if args.style == 'SequoiaEleven' else '') + '.so')
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test', 'Qt6Network', 'Qt6PrintSupport'], text=True))
includes = [f'-I{p}' for p in (SOURCE / 'src/lib').iterdir() if p.is_dir()]
includes += [f'-I{build / "src/lib"}']
for component in ('QtWebEngineCore', 'QtWebEngineWidgets'):
    includes += [f'-I{args.sdk / "include/qt6" / component}']
includes += [f'-I{args.sdk / "include/qt6"}']
subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(ROOT / 'tools/verify-falkon.cpp'), '-o', str(helper),
                *includes, *flags, f'-L{build / "bin"}', '-lFalkonPrivate',
                f'-L{args.sdk / "lib64"}', '-lQt6WebEngineWidgets', '-lQt6WebEngineCore',
                f'-Wl,-rpath,{build / "bin"}'], check=True)
output = ROOT / 'preview/falkon' / (('dark' if args.dark else 'light') + ('-private' if args.private else '') + ('-native' if args.style == 'SequoiaEleven' else ''))
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='falkon-test-', dir=ROOT / '.staging') as tmp:
    base = Path(tmp)
    env = dict(os.environ)
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
        path = base / kind.lower(); path.mkdir(mode=0o700)
        env[f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'] = str(path)
    themes = base / 'data/falkon/themes'; themes.parent.mkdir(parents=True); themes.symlink_to(SOURCE / 'themes')
    (base / 'data/icons').symlink_to(ROOT / 'icons')
    profile = base / 'config/falkon/profiles/sequoia-eleven'; profile.mkdir(parents=True)
    (profile / 'version').write_text('26.11.71')
    if args.private:
        with sqlite3.connect(profile / 'browsedata.db') as database:
            database.executescript((SOURCE / 'src/lib/data/data/browsedata.sql').read_text())
    thumbs = profile / 'thumbnails'; thumbs.mkdir()
    # Fixed local fixtures avoid fetching thumbnails from external sites during tests.
    urls = ['https://www.falkon.org', 'https://store.falkon.org', 'https://www.kde.org', 'https://planet.kde.org']
    for url, label, color in zip(urls, ['Falkon', 'Extensions', 'KDE', 'Community'], ['#bccced','#cec6e9','#b8d6d3','#e4c9b8']):
        picture = Image.new('RGB', (460, 260), color)
        draw = ImageDraw.Draw(picture); draw.rounded_rectangle((40,35,420,240),radius=12,fill='#f7f9fc'); draw.text((66,60),label,fill='#33415c'); draw.rectangle((66,95,340,100),fill=color); draw.rectangle((66,115,300,120),fill=color)
        digest = subprocess.check_output(['openssl','dgst','-md4','-provider','legacy'],input=url.encode()).decode().strip().split()[-1]
        picture.save(thumbs / f'{digest}.png')
    (profile / 'settings.ini').write_text('[Themes]\nactiveTheme=sequoia\n[Web-URL-Settings]\nhomepage=falkon:speeddial\n[Browser-Tabs-Settings]\nAskOnClosing=false\n')
    for name,title in [('alpha','Design notes'),('beta','Reading list')]:
        (base / f'{name}.html').write_text(f'<!doctype html><title>{title}</title><h1>{title}</h1><p>Local browser verification.</p>')
    mode = 'Dark' if args.dark else 'Light'
    (base / 'config/kdeglobals').write_text((ROOT / f'color-schemes/SequoiaEleven{mode}Solid.colors').read_text())
    env.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE=args.style,
               XDG_DATA_DIRS=str(build / 'share') + ':/usr/local/share:/usr/share',
               FALKON_PLUGIN_PATH=str(build / 'bin/plugins'), LD_PRELOAD=str(helper), SEQUOIA_FALKON_VERIFY=str(output), SEQUOIA_FALKON_FIXTURES=str(base),
               QTWEBENGINE_CHROMIUM_FLAGS='--disable-gpu', LC_ALL='C.UTF-8')
    if args.style == 'SequoiaEleven':
        env['QT_PLUGIN_PATH'] = str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins')
    command = ['dbus-run-session','--',str(build / 'bin/falkon'),'--profile','sequoia-eleven','falkon:speeddial']
    if args.private:
        command.append('--private-browsing')
        env['SEQUOIA_FALKON_PRIVATE'] = '1'
    with (output / 'verify.log').open('w') as log:
        proc = subprocess.Popen(command, env=env, stdout=log,stderr=log,start_new_session=True)
        try: proc.wait(timeout=85)
        finally:
            try: os.killpg(proc.pid,signal.SIGTERM)
            except ProcessLookupError: pass
            proc.wait()
    result = (output / 'verify.log').read_text()
    print(result[-6500:])
    if proc.returncode or 'PASS:' not in result: raise SystemExit('Falkon verification failed')
print(f'PASS: captures in {output}')
