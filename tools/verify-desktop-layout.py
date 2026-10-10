#!/usr/bin/env python3
"""Verify packaging; optionally apply all six layouts in a private headless Plasma."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('theme_installer', ROOT / 'tools/install-theme.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)
THEMES = sorted(p.name for p in (ROOT / 'plasma/look-and-feel').iterdir())


def packaging(base):
    subprocess.run([sys.executable, str(ROOT / 'tools/build-desktop-layout.py'), '--check'], check=True)
    stage, live = base / 'stage', base / 'live'
    installer.stage_user(stage, live, {'global-theme', 'widgets'})
    for name in THEMES:
        script = stage / 'plasma/look-and-feel' / name / 'contents/layouts/org.kde.plasma.desktop-layout.js'
        assert script.is_file(), name
        assert '/home/' not in script.read_text()
    for name in installer.LAYOUT_WIDGETS:
        package = stage / 'plasma/plasmoids' / name
        metadata = json.loads((package / 'metadata.json').read_text())
        assert metadata['KPlugin']['Id'] == name
        assert metadata['KPackageStructure'] == 'Plasma/Applet'
        assert (package / 'contents/ui/main.qml').is_file()
    # Installation, refresh and removal must preserve independently owned widgets.
    custom = live / 'plasma/plasmoids/org.kpple.kppleMenu/metadata.json'
    custom.parent.mkdir(parents=True)
    custom.write_text('independently installed widget')
    own_stage = base / 'own-stage'
    installer.stage_user(own_stage, live, {'global-theme', 'widgets'})
    assert not (own_stage / 'plasma/plasmoids/org.kpple.kppleMenu').exists()
    receipt = installer.Receipt(live, installer.USER_STATE, installer.user_allowed)
    receipt.install(own_stage, scope=lambda name: installer.component_for(name) in {'global-theme', 'widgets'})
    refreshed = base / 'refreshed'
    installer.stage_user(refreshed, live, {'global-theme', 'widgets'})
    assert (refreshed / 'plasma/plasmoids/org.sequoia.startmenu/metadata.json').is_file()
    receipt.install(refreshed, scope=lambda name: installer.component_for(name) in {'global-theme', 'widgets'})
    receipt.uninstall()
    assert custom.read_text() == 'independently installed widget'
    assert not (live / 'plasma/plasmoids/org.sequoia.startmenu').exists()
    print('PASS: six standalone layouts, widget payloads, refresh and ownership-safe removal', flush=True)
    return stage


QUERY = r'''
function config(a, group, keys) {
    a.currentConfigGroup = group;
    var result = {};
    var defaults = {launchers:[], folderUrl:'', iconSpacing:0, hiddenItems:[],
        showAllItems:false, boldText:false, fontSize:0, customDateFormat:''};
    keys.forEach(function (key) { result[key] = a.readConfig(key, defaults[key]); });
    return result;
}
print(JSON.stringify(panels().map(function (p) {
    var views = new ConfigFile('plasmashellrc', 'PlasmaViews');
    var view = new ConfigFile(views, 'Panel ' + p.id);
    return {location:p.location, height:p.height, floating:p.floating,
        floatingApplets:typeof p.floatingApplets === 'undefined' ? Number(view.readEntry('floatingApplets')) === 1 : p.floatingApplets,
        lengthMode:p.lengthMode,
        alignment:p.alignment, hiding:p.hiding, opacity:p.opacity, screen:p.screen,
        applets:p.widgets().map(function (w) {
            var general = ['launchers', 'folderUrl', 'iconSpacing', 'hiddenItems', 'showAllItems'];
            return {plugin:w.type, general:config(w, ['General'], general),
                appearance:config(w, ['Appearance'], ['boldText','fontSize','customDateFormat'])};
        })};
})));
'''


def dbus(method, *args):
    result = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                             'org.kde.PlasmaShell.' + method, *args],
                            capture_output=True, text=True, timeout=15, check=True)
    return result.stdout.strip()


def stop(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def native(base):
    assert os.environ.get('SEQUOIA_LAYOUT_TEST') == str(base)
    expected = json.loads((ROOT / 'plasma/layouts/desktop-layout.json').read_text())['panels']
    log = (base / 'plasma.log').open('w')
    compositor = shell = None
    try:
        compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1920', '--height', '1080',
                                      '--output-count', '2', '--no-lockscreen', '--no-global-shortcuts',
                                      '--socket', 'sequoia-layout-test'], stdout=log, stderr=log)
        for _ in range(100):
            if (base / 'runtime/sequoia-layout-test').exists():
                break
            if compositor.poll() is not None:
                raise RuntimeError('Headless compositor failed; see ' + str(base / 'plasma.log'))
            time.sleep(.1)
        else:
            raise RuntimeError('Headless compositor did not start')
        os.environ.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-layout-test')
        shell = subprocess.Popen(['plasmashell', '--no-respawn'], stdout=log, stderr=log)
        for _ in range(100):
            try:
                if dbus('evaluateScript', 'print(desktops().length);') == '2':
                    break
            except subprocess.SubprocessError:
                pass
            time.sleep(.1)
        else:
            raise RuntimeError('Private Plasma did not start with two desktops')
        for theme in THEMES:
            # Exercise the real Global Theme reset path, including removal of old
            # panels, instead of merely evaluating the script on an empty mock.
            dbus('loadLookAndFeelDefaultLayout', theme)
            for _ in range(40):
                time.sleep(.25)
                actual = json.loads(dbus('evaluateScript', QUERY))
                if len(actual) == 2 and all(p['screen'] == 0 for p in actual):
                    break
            assert len(actual) == 2, (theme, actual)
            for want in expected:
                got = next(p for p in actual if p['location'] == want['location'])
                for key in ('height', 'floating', 'floatingApplets', 'lengthMode', 'alignment', 'hiding', 'opacity'):
                    assert got[key] == want[key], (theme, key, got[key], want[key])
                assert got['screen'] == 0, (theme, got)
                assert [w['plugin'] for w in got['applets']] == [w['plugin'] for w in want['applets']]
                for entry in want['applets']:
                    widget = next(w for w in got['applets'] if w['plugin'] == entry['plugin'])
                    for key in ('launchers', 'folderUrl', 'iconSpacing', 'hiddenItems', 'showAllItems'):
                        value = entry['config'].get('/General', {}).get(key)
                        if value is not None:
                            observed = widget['general'][key]
                            if isinstance(value, list) and isinstance(observed, str):
                                observed = observed.split(',') if observed else []
                            assert observed == value, (theme, entry['plugin'], key, observed, value)
                    for key, value in entry['config'].get('/Appearance', {}).items():
                        if key in ('boldText', 'fontSize', 'customDateFormat'):
                            assert widget['appearance'][key] == value, (theme, key, widget)
            assert dbus('evaluateScript', 'print(desktopsForActivity(currentActivity()).length);') == '2'
            print('PASS: Plasma applied ' + theme + ' with exact panel geometry, widget order and settings', flush=True)
    finally:
        stop(shell)
        subprocess.run(['akonadictl', '--instance', os.environ['AKONADI_INSTANCE'], '--wait', 'stop'],
                       stdout=log, stderr=log, timeout=20, check=False)
        stop(compositor)
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', action='store_true', help='Also run Plasma in a private headless Wayland session')
    parser.add_argument('--child', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.child:
        native(args.child)
        return
    with tempfile.TemporaryDirectory(prefix='sequoia-layout-test-', ignore_cleanup_errors=True) as temporary:
        base = Path(temporary)
        stage = packaging(base)
        if args.native:
            native_stage = base / 'native'
            installer.widget_payload.stage(native_stage, installer.PLUGINDIR)
            env = dict(os.environ, SEQUOIA_LAYOUT_TEST=str(base), QT_QPA_PLATFORM='offscreen',
                       QT_QUICK_BACKEND='software', QT_STYLE_OVERRIDE='Fusion', QT_QPA_PLATFORMTHEME='generic',
                       XDG_DATA_DIRS=str(stage) + ':' + str(native_stage / 'usr/share') + ':/usr/local/share:/usr/share', KWIN_COMPOSE='Q',
                       AKONADI_INSTANCE=base.name, QT_FORCE_STDERR_LOGGING='1')
            for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH'):
                env.pop(key, None)
            env['HOME'] = str(base)
            env['QT_PLUGIN_PATH'] = str(native_stage / 'usr' / installer.PLUGINDIR)
            for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
                path = base / kind.lower()
                path.mkdir(mode=0o700)
                env['XDG_' + kind + ('_DIR' if kind == 'RUNTIME' else '_HOME')] = str(path)
            with (base / 'session.log').open('w') as session_log:
                result = subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()),
                                         '--child', str(base)], env=env, stdout=session_log,
                                        stderr=session_log, timeout=120)
            output = (base / 'session.log').read_text()
            print('\n'.join(line for line in output.splitlines() if line.startswith('PASS:')))
            if result.returncode:
                print(output, file=sys.stderr)
                print((base / 'plasma.log').read_text()[-10000:], file=sys.stderr)
                raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
