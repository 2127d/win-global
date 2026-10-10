#!/usr/bin/env python3
"""Verify and capture the actual settings fork without touching live configuration."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import signal

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT / '.staging/settings-build/bin/systemsettings')
parser.add_argument('--skip-install-check', action='store_true', help='Use when --binary is the installed launcher.')
parser.add_argument('--startup', help='Exercise an existing module deep link on startup.')
parser.add_argument('--capture-group', help='Open and capture every native page in a category; use all for every category.')
parser.add_argument('--third-party', action='store_true')
parser.add_argument('--debug', action='store_true')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--rtl', action='store_true')
parser.add_argument('--scale', default='1')
args = parser.parse_args()
helper = ROOT / '.staging/verify-settings-navigation.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6QuickWidgets', 'Qt6Quick', 'Qt6Test'], text=True))
subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-settings-navigation.cpp'), '-o', str(helper), *flags], check=True)
mode = 'dark' if args.dark else 'light'
output = ROOT / 'preview' / ('settings-navigation-' + mode + ('-third-party' if args.third_party else '') + ('-rtl' if args.rtl else '') + ('-' + args.scale if args.scale != '1' else ''))
output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='sequoia-settings-verification-') as temp:
    folder = Path(temp)
    env = dict(os.environ)
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE'):
        path = folder / kind.lower(); path.mkdir()
        env[f'XDG_{kind}_HOME'] = str(path)
    (folder / 'config/kdeglobals').write_text((ROOT / f'color-schemes/SequoiaEleven{mode.title()}Solid.colors').read_text() + f'\n[Icons]\nTheme=SequoiaTahoe-{mode}\n')
    runtime = folder / 'runtime'; runtime.mkdir(mode=0o700)
    env['XDG_RUNTIME_DIR'] = str(runtime)
    env['LIBGL_ALWAYS_SOFTWARE'] = '1'
    if args.third_party:
        external = folder / 'data/plasma/systemsettings/externalmodules'
        external.mkdir(parents=True)
        (external / 'sequoia-test-extra.desktop').write_text('[Desktop Entry]\nType=Application\nName=Additional Settings\nComment=Third-party category coverage test\nIcon=preferences-system\nExec=/bin/true\nX-KDE-System-Settings-Parent-Category=unregistered-vendor-category\n')
    (folder / 'data/color-schemes').symlink_to(ROOT / 'color-schemes', target_is_directory=True)
    (folder / 'data/icons').symlink_to(ROOT / 'icons', target_is_directory=True)
    env.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='kde',
               QT_QUICK_CONTROLS_STYLE='org.kde.desktop', QT_STYLE_OVERRIDE='SequoiaEleven', QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'),
               QT_SCALE_FACTOR=args.scale, LC_ALL='C.UTF-8')
    env.pop('LD_PRELOAD', None)
    if args.rtl: env['SEQUOIA_SETTINGS_RTL'] = '1'
    application = [str(args.binary.resolve())] + ([args.startup] if args.startup else [])
    if args.startup: env['SEQUOIA_SETTINGS_STARTUP'] = args.startup
    if args.capture_group: env['SEQUOIA_SETTINGS_CAPTURE_GROUP'] = args.capture_group
    if args.debug:
        application = ['gdb', '--batch', '-ex', 'set debuginfod enabled off', '-ex', 'run', '-ex', 'thread apply all bt', '--args', *application]
    launcher = folder / 'launch-test.sh'
    launcher.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\n'
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
    command = ['dbus-run-session', '--config-file=' + str(bus), '--', 'kwin_wayland', '--virtual', '--width', '1400', '--height', '1000',
               '--inputmethod', '/bin/true', '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities', '--socket', 'sequoia-settings-test',
               '--exit-with-session', str(launcher)]
    with (output / 'verification.log').open('w') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=log, text=True, start_new_session=True)
        try:
            process.wait(timeout=180 if args.capture_group == 'all' else 60)
        finally:
            try: os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError: pass
            process.wait()
    result = subprocess.CompletedProcess(command, process.returncode, (output / 'verification.log').read_text())
    print(result.stdout[-6000:])
    result.check_returncode()
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
