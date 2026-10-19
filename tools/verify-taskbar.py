#!/usr/bin/env python3
"""Exercise the taskbar in a private Wayland session, without editing the desktop."""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def wait_for(check, message, seconds=20):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            result = check()
            if result:
                return result
        except (OSError, ValueError, subprocess.SubprocessError):
            pass
        time.sleep(.2)
    raise RuntimeError(message)

def stop(process):
    if process and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

def child(base, payload):
    shell = compositor = fixture = None
    with (base / 'native.log').open('w') as log:
        try:
            subprocess.run([str(ROOT / '.staging/taskbar-build/taskbar-kcm-test'), str(base / 'settings.png')],
                stdout=log, stderr=log, timeout=20, check=True)
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '800',
                '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities', '--inputmethod', '/bin/true',
                '--socket', 'sequoia-taskbar-test'], stdout=log, stderr=log)
            wait_for(lambda: (Path(os.environ['XDG_RUNTIME_DIR']) / 'sequoia-taskbar-test').exists(), 'Compositor did not start')
            os.environ.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-taskbar-test')
            env = dict(os.environ, LD_PRELOAD=str(base / 'probe.so'), SEQUOIA_TASKBAR_PROBE=str(base))
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=env, stdout=log, stderr=log)
            def script(source):
                return subprocess.check_output(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                    'org.kde.PlasmaShell.evaluateScript', source], text=True, timeout=8).strip()
            wait_for(lambda: script('print(desktops().length);') == '1', 'Plasma did not start')
            script('panels().forEach(p => p.remove());')
            template = ROOT / 'desktop/taskbar/template/contents/layout.js'
            print(script(template.read_text() + '\nprint(JSON.stringify(panels().map(p => ({id:p.id,type:p.type}))));'), flush=True)
            def state():
                return json.loads((base / 'state.json').read_text())
            def settings(values):
                writer = base / 'write.qml'
                statements = ''.join('s.setValue(' + json.dumps(key) + ', ' + json.dumps(value) + ');' for key, value in values.items())
                writer.write_text('import QtQuick\nimport org.sequoia.taskbar 1.0\nItem { Settings { id: s }\n'
                    'Component.onCompleted: { ' + statements + 's.save(); Qt.quit(); } }\n')
                subprocess.run(['qml-qt6', '-platform', 'offscreen', str(writer)], env=os.environ,
                    stdout=log, stderr=log, timeout=15, check=True)
            initial = wait_for(lambda: (lambda s: s if s.get('taskCount', 0) > 0 else None)(state()), 'Integrated task strip did not load')
            assert initial['effectiveIconSize'] == 32, initial
            assert initial['builtInMenu'] is False, 'Taskbar must not create a built-in start menu'
            wait_for(lambda: state().get('validIcons', 0) >= 3, 'Launcher icons are not rendered')
            assert script('print(panels()[0].widgets().length);') == '0', 'Tasks must not be widgets'
            assert script('print(panels()[0].type);') == 'org.sequoia.taskbar'
            (base / 'capture').touch()
            wait_for(lambda: (base / 'panel.png').exists(), 'Panel capture missing')
            def command(value):
                temporary = base / 'command.tmp'
                temporary.write_text(json.dumps(value))
                temporary.replace(base / 'command.json')
                wait_for(lambda: not (base / 'command.json').exists(),
                    f'Probe did not dispatch {value}')
            hovered_name = state()['buttons'][0]['text']
            def hovered_task():
                return next((b for b in state().get('buttons', []) if b['text'] == hovered_name), {})
            initial_button = hovered_task()
            command({'hoverTask': hovered_name})
            wait_for(lambda: hovered_task().get('scale', 0) > 1.1 and hovered_task().get('motionOffset', 0) < 0,
                'Native task hover motion did not run')
            assert hovered_task()['width'] == initial_button['width'] and hovered_task()['height'] == initial_button['height'], 'Motion changed button hit geometry'
            settings({'dockAnimations': False})
            wait_for(lambda: hovered_task().get('scale') == 1 and hovered_task().get('motionOffset') == 0,
                'Disabling animations did not reset the hovered task')
            settings({'dockAnimations': True})
            fixture_qml = base / 'window.qml'
            fixture_qml.write_text('import QtQuick\nimport QtQuick.Window\nWindow { visible: true; width: 400; height: 260; title: "Sequoia Taskbar Test"; Timer { interval: 60000; running: true; onTriggered: Qt.quit() } }\n')
            applications = base / 'data/applications'
            applications.mkdir(parents=True, exist_ok=True)
            executable_path = str(fixture_qml).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%')
            (applications / 'org.sequoia.taskbartest.desktop').write_text('[Desktop Entry]\nType=Application\nName=Sequoia Taskbar Test\n'
                'Icon=application-x-executable\nExec=qml-qt6 "' + executable_path + '"\n')
            subprocess.run(['kbuildsycoca6', '--noincremental'], stdout=log, stderr=log, check=True, timeout=20)
            settings({'launchers': ['preferred://browser', 'applications:org.kde.dolphin.desktop', 'applications:org.kde.konsole.desktop', 'applications:org.sequoia.taskbartest.desktop']})
            wait_for(lambda: any(b['text'] == 'Sequoia Taskbar Test' and not b['isRunning'] for b in state().get('buttons', [])), 'Pinned test launcher did not appear')
            command({'clickTask': 'Sequoia Taskbar Test', 'running': False})
            def active_fixture():
                return next((b for b in state().get('buttons', []) if b['text'] == 'Sequoia Taskbar Test' and b['isRunning']), None)
            wait_for(lambda: active_fixture() and active_fixture()['isActiveTask'], 'Running application indicator did not appear')
            command({'mouseClickTask': 'Sequoia Taskbar Test'})
            wait_for(lambda: active_fixture() and active_fixture()['isMinimized'], 'Click did not minimize the active window')
            command({'mouseClickTask': 'Sequoia Taskbar Test'})
            wait_for(lambda: active_fixture() and active_fixture()['isActiveTask'], 'Click did not restore the window')
            # Only the native length policy should change with the mode preset.
            script('var p = panels()[0]; p.floating = false; p.alignment = "center"; p.hiding = "none";')
            def editor_state():
                return json.loads(script('var p = panels()[0]; print(JSON.stringify({floating:p.floating,alignment:p.alignment,hiding:p.hiding,height:p.height,location:p.location}));'))
            original_editor = editor_state()
            settings({'surfaceMode': 'dock'})
            wait_for(lambda: state().get('lengthMode') == 1 and 100 < state().get('windowWidth', 0) < 500,
                'Dock mode did not fit its contents')
            assert editor_state() == original_editor, 'Dock mode overwrote panel editor settings'
            dock_length = state()['windowWidth']
            settings({'launchers': ['applications:org.kde.dolphin.desktop', 'applications:org.sequoia.taskbartest.desktop']})
            wait_for(lambda: state().get('windowWidth', 1280) < dock_length - 50, 'Dock did not shrink after unpinning applications')
            settings({'launchers': ['preferred://browser', 'applications:org.kde.dolphin.desktop', 'applications:org.kde.konsole.desktop', 'applications:org.sequoia.taskbartest.desktop']})
            wait_for(lambda: abs(state().get('windowWidth', 0) - dock_length) < 2, 'Dock did not grow when applications were pinned')
            settings({'sizeMode': 'fill'})
            wait_for(lambda: abs(state().get('windowWidth', 0) - dock_length) < 2,
                'Fill icon sizing fed back into dock length')
            (base / 'capture').touch()
            wait_for(lambda: not (base / 'capture').exists(), 'Dock capture did not complete')
            (base / 'dock.png').write_bytes((base / 'panel.png').read_bytes())
            settings({'surfaceMode': 'panel'})
            wait_for(lambda: state().get('lengthMode') == 0 and state().get('windowWidth', 0) > 1200,
                'Panel mode did not return to full width')
            assert editor_state() == original_editor, 'Panel mode overwrote panel editor settings'
            settings({'sizeMode': 'fill'})
            wait_for(lambda: state().get('cellLength', 0) > 100, 'Fill mode did not expand the buttons')
            assert state()['effectiveIconSize'] == 32, state()
            settings({'sizeMode': 'fixed', 'iconSize': 128, 'spacing': 64})
            print(script('var p = panels()[0]; p.lengthMode = "custom"; p.minimumLength = 320; p.maximumLength = 320; p.length = 320; print(JSON.stringify({mode:p.lengthMode,length:p.length,min:p.minimumLength,max:p.maximumLength}));'), flush=True)
            wait_for(lambda: state().get('contentLength', 0) > state().get('taskLength', 0), 'Overflow must scroll rather than shrinking icons')
            settings({'iconSize': 32, 'spacing': 6})
            script('var p = panels()[0]; p.minimumLength = 0; p.maximumLength = 1280; p.lengthMode = "fill";')
            script('panels()[0].addWidget("org.kde.plasma.digitalclock");')
            assert script('print(panels()[0].widgets()[0].type);') == 'org.kde.plasma.digitalclock'
            wait_for(lambda: state().get('clockVisible'), 'Added widget did not render')
            command({'addLauncher': True})
            wait_for(lambda: state().get('alternativesVisible'), 'Add Launcher did not open alternatives')
            choices = state()['alternatives']
            assert 'org.kde.plasma.kickoff' in choices and 'org.kde.plasma.kicker' in choices, choices
            assert 'org.kde.plasma.digitalclock' not in choices, 'Launcher picker offers ordinary widgets'
            command({'cancelAlternatives': True})
            wait_for(lambda: not state().get('alternativesVisible'), 'Escape did not dismiss launcher picker')
            assert script('print(panels()[0].widgets().length);') == '1', 'Cancel created an unwanted launcher'
            command({'addLauncher': True})
            wait_for(lambda: state().get('alternativesVisible'), 'Launcher picker did not reopen')
            command({'selectAlternative': 'org.kde.plasma.kickoff'})
            wait_for(lambda: state().get('launcherWidgetVisible'), 'Selected application launcher did not render')
            def slot():
                return next((a for a in state().get('applets', []) if a['inLauncherSlot']), None)
            wait_for(lambda: slot() and slot()['plugin'] == 'org.kde.plasma.kickoff', 'Launcher is not in its reserved slot')
            assert slot()['x'] + slot()['width'] <= state()['taskX'], 'Launcher does not precede tasks'
            command({'addLauncher': True})
            wait_for(lambda: state().get('alternativesVisible'), 'Existing launcher did not open native alternatives')
            command({'selectAlternative': 'org.kde.plasma.kicker'})
            wait_for(lambda: slot() and slot()['plugin'] == 'org.kde.plasma.kicker', 'Alternative did not replace the leading launcher')
            wait_for(lambda: script('print(panels()[0].widgets().length);') == '2', 'Alternatives left a duplicate launcher')
            command({'addLauncher': True})
            wait_for(lambda: state().get('alternativesVisible'), 'Cannot select Sequoia Start Menu')
            command({'selectAlternative': 'org.sequoia.startmenu'})
            wait_for(lambda: slot() and slot()['plugin'] == 'org.sequoia.startmenu', 'Sequoia Start Menu did not load')
            def matching_launcher_size():
                s = state()
                launcher = next((a for a in s.get('applets', []) if a['inLauncherSlot']), {})
                return launcher.get('iconWidth') == s.get('effectiveIconSize') == launcher.get('iconHeight')
            wait_for(matching_launcher_size, 'Start menu icon does not match the task icons')
            settings({'iconSize': 24})
            wait_for(lambda: state().get('effectiveIconSize') == 24 and matching_launcher_size(), 'Start menu did not shrink with icon size')
            settings({'iconSize': 40})
            wait_for(lambda: state().get('effectiveIconSize') == 40 and matching_launcher_size(), 'Start menu did not grow with icon size')
            settings({'iconSize': 32})
            wait_for(lambda: state().get('effectiveIconSize') == 32 and matching_launcher_size(), 'Start menu size did not reset')
            # Reordering normal widgets cannot move them before the task strip.
            script('var p = panels()[0]; var w = p.widgets().find(w => w.type === "org.kde.plasma.digitalclock"); w.index = 0;')
            clock = next(a for a in state()['applets'] if a['plugin'] == 'org.kde.plasma.digitalclock')
            assert not clock['inLauncherSlot'] and clock['x'] >= state()['taskX'] + state()['width'], clock
            script('panels()[0].height = 84;')
            wait_for(lambda: state().get('height', 0) >= 70, 'Panel resize did not arrive')
            assert state()['effectiveIconSize'] == 32, state()
            wait_for(matching_launcher_size, 'Panel resize enlarged a fixed-size launcher')
            # Write through the same Settings QML plugin as the KCM, in another process.
            settings({'sizeMode': 'panel'})
            wait_for(lambda: state().get('effectiveIconSize', 0) > 32, 'Live settings notification did not resize icons')
            wait_for(matching_launcher_size, 'Start menu did not follow panel-relative sizing')
            settings({'sizeMode': 'fill', 'iconSize': 40})
            wait_for(lambda: state().get('effectiveIconSize') == 40 and matching_launcher_size(), 'Fill mode enlarged the launcher beyond the task icons')
            script('panels()[0].location = "left";')
            wait_for(lambda: state().get('horizontal') is False, 'Vertical panel did not rotate')
            settings({'sizeMode': 'fixed', 'surfaceMode': 'dock'})
            wait_for(lambda: state().get('lengthMode') == 1 and 100 < state().get('windowHeight', 0) < 600,
                'Vertical dock did not fit contents with an added widget')
            assert state()['clockVisible'], 'Dock lost the added widget'
            assert slot() and slot()['plugin'] == 'org.sequoia.startmenu', 'Vertical dock lost the leading launcher'
            wait_for(matching_launcher_size, 'Vertical launcher does not match the task icons')
            assert slot()['y'] + slot()['height'] <= state()['taskY'], 'Vertical launcher does not precede tasks'
            (base / 'capture').touch()
            wait_for(lambda: not (base / 'capture').exists(), 'Vertical dock capture did not complete')
            (base / 'vertical-dock.png').write_bytes((base / 'panel.png').read_bytes())
            # User length overrides made in the regular editor must survive restart.
            script('var p = panels()[0]; p.lengthMode = "custom"; p.minimumLength = 500; p.maximumLength = 500; p.length = 500;')
            wait_for(lambda: state().get('lengthMode') == 2, 'Custom length override failed')
            settings({'spacing': 8})
            time.sleep(.5)
            wait_for(lambda: state().get('lengthMode') == 2 and state().get('clockVisible'),
                'Unrelated settings reapplied the dock preset')
            stop(shell)
            (base / 'state.json').unlink(missing_ok=True)
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=env, stdout=log, stderr=log)
            wait_for(lambda: state().get('lengthMode') == 2 and state().get('horizontal') is False,
                'Restart overwrote the editor length choice')
            assert editor_state()['floating'] is False
            wait_for(lambda: slot() and slot()['plugin'] == 'org.sequoia.startmenu' and state().get('clockVisible'),
                'Restart did not restore the reserved launcher and ordinary widgets')
            wait_for(matching_launcher_size, 'Restart lost launcher icon sizing')
            script('panels()[0].widgets().find(w => w.type === "org.sequoia.startmenu").remove();')
            wait_for(lambda: slot() is None and state().get('clockVisible'), 'Removing the launcher damaged ordinary widgets')
            command({'addLauncher': True})
            wait_for(lambda: state().get('alternativesVisible'), 'Cannot add a launcher again after removal')
            command({'selectAlternative': 'org.kde.plasma.kickoff'})
            wait_for(lambda: slot() and slot()['plugin'] == 'org.kde.plasma.kickoff', 'Cannot refill the launcher slot')
            # A newly added taskbar inherits the global Dock preset.
            script('panels().forEach(p => p.remove());')
            (base / 'state.json').unlink(missing_ok=True)
            script(template.read_text())
            wait_for(lambda: state().get('lengthMode') == 1 and state().get('horizontal') is True
                and 100 < state().get('windowWidth', 0) < 600, 'New taskbar did not inherit Dock mode')
            print('PASS: native KCM, launcher picker/cancel/replacement/removal, reserved launcher position/restart, task hover/disable, fixed hit geometry, icons, window actions, sizing, dock growth/shrink, both orientations, editor preservation/restart, and new dock creation', flush=True)
        finally:
            stop(fixture)
            stop(shell)
            stop(compositor)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--payload', type=Path, default=ROOT / '.staging/taskbar-payload')
    parser.add_argument('--output', type=Path, default=ROOT / '.staging/taskbar-verification')
    parser.add_argument('--child', action='store_true')
    args = parser.parse_args()
    base, payload = args.output.resolve(), args.payload.resolve()
    if args.child:
        child(base, payload)
        return
    base.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / 'widgets/plasmoids/org.sequoia.startmenu',
        base / 'data/plasma/plasmoids/org.sequoia.startmenu', dirs_exist_ok=True)
    for relative in ('config/sequoiataskbarrc', 'config/plasma-org.kde.plasma.desktop-appletsrc',
                     'config/plasmashellrc', 'state.json', 'panel.png', 'capture', 'command.json'):
        (base / relative).unlink(missing_ok=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-taskbar.cpp'), '-o', str(base / 'probe.so'), *flags], check=True)
    env = dict(os.environ)
    for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH'):
        env.pop(name, None)
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
        directory = base / kind.lower()
        directory.mkdir(mode=0o700, exist_ok=True)
        env['XDG_' + kind + ('_DIR' if kind == 'RUNTIME' else '_HOME')] = str(directory)
    env.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_STYLE_OVERRIDE='Fusion',
        QT_FORCE_STDERR_LOGGING='1',
        QT_QPA_PLATFORMTHEME='generic', KWIN_COMPOSE='Q', LIBGL_ALWAYS_SOFTWARE='1',
        XDG_DATA_DIRS=str(payload / 'usr/share') + ':/usr/local/share:/usr/share',
        QT_PLUGIN_PATH=str(payload / Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip()).relative_to('/')),
        QML_IMPORT_PATH=str(payload / Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_QML'], text=True).strip()).relative_to('/')))
    with tempfile.TemporaryDirectory(prefix='sequoia-tb-') as runtime:
        env['XDG_RUNTIME_DIR'] = runtime
        result = subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()),
            '--child', '--payload', str(payload), '--output', str(base)], env=env, timeout=150)
    if result.returncode:
        if (base / 'native.log').exists():
            print((base / 'native.log').read_text()[-12000:], file=sys.stderr)
        raise SystemExit(result.returncode)

if __name__ == '__main__':
    main()
