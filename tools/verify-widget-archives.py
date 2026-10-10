#!/usr/bin/env python3
"""Validate archive installation, configuration isolation and native fork loading."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--folder', type=Path, default=ROOT / '.staging/widget-archives')
parser.add_argument('--native', action='store_true')
parser.add_argument('--functional', action='store_true', help='Exercise task settings and notification delivery too')
parser.add_argument('--tray-size', action='store_true', help='Exercise tray size settings, persistence and independent detail sizes')
args = parser.parse_args()
folder = args.folder.resolve()
spec = importlib.util.spec_from_file_location('archive_installer', ROOT / 'tools/install-widget-archives.py')
installer = importlib.util.module_from_spec(spec); spec.loader.exec_module(installer)

with tempfile.TemporaryDirectory(prefix='sequoia-fork-test-') as temporary:
    root = Path(temporary)
    config = root / 'guest config'; config.mkdir()
    panel = config / 'plasma-org.kde.plasma.desktop-appletsrc'
    panel.write_text('''[Containments][1]
plugin=org.kde.panel
[Containments][1][Applets][2]
plugin=org.kde.plasma.icontasks
[Containments][1][Applets][2][Configuration][General]
launchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop
[Containments][1][Applets][3]
plugin=org.kde.plasma.systemtray
[Containments][1][Applets][3][General]
knownItems=org.kde.plasma.notifications,org.kde.plasma.volume
extraItems=org.kde.plasma.notifications,org.kde.plasma.volume
hiddenItems=org.kde.plasma.notifications
[Containments][1][Applets][3][Applets][4]
plugin=org.kde.plasma.notifications
''')
    prefix = root / 'system'
    subprocess.run(['python3', str(folder / 'install-widgets.py'), '--folder', str(folder),
                    '--prefix', str(prefix), '--config-dir', str(config)], check=True)
    updated = panel.read_text()
    assert 'plugin=org.kde.plasma.icontasks\n' in updated
    assert 'plugin=org.kde.plasma.systemtray\n' in updated
    assert 'launchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop\n' in updated
    assert 'extraItems=org.kde.plasma.volume,org.sequoia.notifications\n' in updated
    assert 'hiddenItems=org.sequoia.notifications\n' in updated
    assert 'plugin=org.sequoia.notifications\n' in updated
    packages = installer.unpack_archives(folder, root / 'unpacked')
    assert installer.notification_settings(updated, packages) == updated
    migration = installer.fork_settings(panel.read_text(), packages)
    assert 'plugin=org.sequoia.systemtray\n' in migration
    assert 'plugin=org.sequoia.icontasks.animated\n' in migration
    assert 'launchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop\n' in migration
    assert 'extraItems=org.sequoia.volume,org.sequoia.notifications\n' in migration
    assert installer.fork_settings(migration, packages) == migration
    trash = '[Containments][7][Applets][8]\nplugin=org.kde.plasma.trash\n'
    assert 'plugin=org.sequoia.trash\n' in installer.fork_settings(trash, packages)
    disabled = '[Containments][1][Applets][3]\nplugin=org.kde.plasma.systemtray\n[Containments][1][Applets][3][General]\nknownItems=org.kde.plasma.notifications\nextraItems=org.kde.plasma.volume\n'
    assert 'extraItems=org.kde.plasma.volume\n' in installer.notification_settings(disabled, packages)
    applets = prefix / 'usr/share/plasma/plasmoids'
    assert not list(applets.glob('*/contents/bundled'))
    compact = applets / 'org.sequoia.systemtray/contents/applet/CompactApplet.qml'
    assert compact.is_file(), 'Tray children must not fall back to standalone shell popups'
    assert 'PlasmaCore.AppletPopup' not in compact.read_text()
    for name in packages:
        assert installer.package_id(applets / name) == name
    # A bad package must be rejected before an installer can replace anything.
    bad = root / 'bad'; bad.mkdir()
    missing = root / 'org.sequoia.incomplete'; missing.mkdir()
    (missing / 'metadata.json').write_text(json.dumps({'KPlugin': {'Id': missing.name}, 'KPackageStructure': 'Plasma/Applet'}))
    with tarfile.open(bad / 'incomplete.tar.gz', 'w:gz') as archive: archive.add(missing, arcname=missing.name)
    try: installer.unpack_archives(bad, root / 'bad-stage')
    except ValueError: pass
    else: raise AssertionError('Incomplete archive accepted')
    # A failed system update must restore every target already changed.
    sentinel = applets / 'org.sequoia.downloads/previous-install'
    sentinel.write_text('preserve')
    real_copy = installer.copy
    calls = 0
    def fail_during_update(source, target):
        global calls
        if target.parent.name.startswith('.sequoia-install-'):
            calls += 1
            if calls == 2: raise OSError('simulated copy failure')
        real_copy(source, target)
    installer.copy = fail_during_update
    try:
        try: installer.install_files(packages, prefix)
        except OSError: pass
        else: raise AssertionError('Failure fixture did not run')
    finally: installer.copy = real_copy
    assert sentinel.read_text() == 'preserve'
    # An earlier user-local copy must not shadow the system package.
    local_data = root / 'local-data'
    local = local_data / 'plasma/plasmoids/org.sequoia.downloads'
    local.mkdir(parents=True); (local / 'old').touch()
    hook = config / 'systemd/user/plasma-plasmashell.service.d/50-sequoia-control-center.conf'
    hook.parent.mkdir(parents=True); hook.write_text('# Sequoia Eleven Control Center\n')
    unrelated = hook.with_name('10-custom.conf'); unrelated.write_text('keep me')
    installer.retire_local_copies(packages, local_data, config)
    assert hook.exists(), 'Installing without migration must preserve the legacy customization'
    local.mkdir(parents=True); (local / 'old').touch()
    installer.retire_local_copies(packages, local_data, config, activate_forks=True)
    assert not local.exists() and not hook.exists() and unrelated.read_text() == 'keep me'
    assert any(local_data.glob('sequoia-widgets-backups/*/0/old'))
    print('PASS: incomplete archive rejection, failed-update rollback, local shadow retirement and unrelated service preservation')
    print('PASS: archive validation, independent IDs, system staging, notification selection, disabled notifications, unchanged tray/task manager and launchers')
    if args.native or args.functional or args.tray_size:
        native = prefix / installer.plugin_directory().relative_to('/').parent.parent
        probe = root / 'probe.so'
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick'], text=True))
        subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(ROOT / 'tools/verify-widget-packages.cpp'),
                        '-o', str(probe), *flags], check=True)
        runtime = root / 'run'; runtime.mkdir(mode=0o700)
        services = root / 'dbus-services'; services.mkdir()
        # The task model needs a current activity even for pinned launchers.
        # Allow only this service on the test bus, with the disposable HOME.
        shutil.copy2('/usr/share/dbus-1/services/org.kde.ActivityManager.service', services)
        bus = root / 'dbus.conf'
        bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir=' + str(root)
                       + '</listen><servicedir>' + str(services) + '</servicedir><policy context="default"><allow send_destination="*"/>'
                       '<allow eavesdrop="true"/><allow own="*"/></policy></busconfig>')
        known = ','.join([p.stem for p in installer.plugin_directory().glob('*.so')]
                         + list(packages) + ['org.kde.kdeconnect', 'org.kde.plasma.vault'])
        cases = [(name, probe, {}, 'PASS: loaded installed widget') for name in
                 ('org.sequoia.downloads', 'org.sequoia.dockseparator', 'org.sequoia.startmenu', 'org.sequoia.icontasks.animated',
                  'org.sequoia.systemtray', 'org.sequoia.notifications', 'org.sequoia.trash')]
        cases.append(('org.sequoia.systemtray', probe, {'SEQUOIA_VERIFY_TRAY_DEFAULTS': '1'},
                      'PASS: fresh tray loads custom notifications, volume and clipboard'))
        if args.tray_size:
            helper = root / 'tray-size.so'
            flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6Test'], text=True))
            subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(ROOT / 'tools/verify-tray-size.cpp'),
                            '-o', str(helper), *flags], check=True)
            if not (args.native or args.functional):
                cases = []
            cases.append(('org.sequoia.systemtray', helper,
                          {'SEQUOIA_VERIFY_TRAY_SIZE': str(ROOT / 'preview/tray-size'),
                           'SEQUOIA_VERIFY_DETAILS': 'volume,networkmanagement'},
                          'PASS: tray size settings'))
        if args.functional:
            for stem, name, variable, expected in [
                ('dock', 'org.sequoia.icontasks.animated', 'SEQUOIA_DOCK_TEST', 'PASS: native task delegates'),
                ('notifications', 'org.sequoia.notifications', 'SEQUOIA_NOTIFICATION_CAPTURE', 'PASS: native notification delivery'),
                ('control-center', 'org.sequoia.systemtray', 'SEQUOIA_CONTROL_CAPTURE', 'PASS: all detail menus and Back navigation')]:
                helper = root / (stem + '.so')
                flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6Test', 'Qt6DBus'], text=True))
                source = ROOT / 'tools' / ('verify-' + stem + '.cpp')
                extra = {variable: str(ROOT / '.staging' / ('fork-' + stem + '.png'))}
                if stem == 'control-center':
                    adapted = root / 'control-center.cpp'
                    adapted.write_text(source.read_text().replace('"org.kde.plasma."', '"org.sequoia."')
                                       .replace('"org.kde.merkuro.contact.applet"', '"org.sequoia.contacts"')
                                       .replace('#include <QApplication>', '#include <QApplication>\n#include <QClipboard>')
                                       .replace('qApp->setQuitOnLastWindowClosed(false);',
                                                'qApp->setQuitOnLastWindowClosed(false);\n'
                                                'QTimer::singleShot(700, qApp, [] { QGuiApplication::clipboard()->setText("Private clipboard fixture"); });'))
                    source = adapted
                    extra.update(SEQUOIA_VERIFY_DETAILS='volume,notifications,clipboard',
                                 SEQUOIA_DETAIL_CAPTURE=str(ROOT / '.staging/fork-detail'))
                subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(source),
                                '-o', str(helper), *flags, *(['-lnotificationmanager'] if stem == 'notifications' else [])], check=True)
                cases.append((name, helper, extra, expected))
        for name, helper, extra_env, expected in cases:
            settings = '[Applets][42]\nplugin=' + name + '\n'
            enabled = ','.join('org.sequoia.' + short for short in extra_env.get('SEQUOIA_VERIFY_DETAILS', 'notifications').split(','))
            for group in (() if 'SEQUOIA_VERIFY_TRAY_DEFAULTS' in extra_env else ('[Applets][42][General]', '[Applets][42][Configuration][General]')):
                settings += (group + '\nknownItems=' + known + '\nextraItems=' + enabled + '\n'
                             'launchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop\n')
            (config / 'plasmawindowedrc').write_text(settings)
            env = dict(os.environ, HOME=str(root), XDG_DATA_HOME=str(prefix / 'usr/share'),
                       XDG_CONFIG_HOME=str(config), XDG_STATE_HOME=str(root / 'state'),
                       XDG_CACHE_HOME=str(root / 'cache'), XDG_RUNTIME_DIR=str(runtime),
                       QT_PLUGIN_PATH=str(native), QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                       QT_STYLE_OVERRIDE='Fusion', QT_QPA_PLATFORMTHEME='generic', QT_FORCE_STDERR_LOGGING='1',
                       SEQUOIA_VERIFY_WIDGET_ENTRY=str(applets / name / 'contents/ui/main.qml'))
            env.update(extra_env)
            command = ['env', 'LD_PRELOAD=' + str(helper), 'plasmawindowed', name]
            if extra_env:
                # Qt's offscreen platform has no window-system task model.
                # Use a private virtual compositor, never the user's session.
                launcher = root / 'launch-test.sh'
                launcher.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexport LD_PRELOAD='
                                    + shlex.quote(str(helper)) + '\nexec plasmawindowed ' + shlex.quote(name) + '\n')
                launcher.chmod(0o755)
                env['LIBGL_ALWAYS_SOFTWARE'] = '1'
                command = ['kwin_wayland', '--virtual', '--width', '1280', '--height', '960',
                           '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities',
                           '--socket', 'sequoia-test', '--exit-with-session', str(launcher)]
            log = ROOT / '.staging' / (name + ('-defaults' if 'SEQUOIA_VERIFY_TRAY_DEFAULTS' in extra_env else '-functional' if extra_env else '') + '-fork.log')
            with log.open('w') as output:
                proc = subprocess.Popen(['dbus-run-session', '--config-file=' + str(bus), '--',
                                         *command],
                                        env=env, stdout=output, stderr=output, start_new_session=True)
                try: result = proc.wait(timeout=35 if extra_env else 15)
                finally:
                    try: os.killpg(proc.pid, signal.SIGTERM)
                    except ProcessLookupError: pass
                    proc.wait()
            output = log.read_text()
            if result or expected not in output or 'error when loading applet' in output:
                raise RuntimeError(output)
            print('PASS:', 'functional checks' if extra_env else 'native independent widget', name)
print('PASS: widget archives')
