#!/usr/bin/env python3
"""Exercise both native greeters on two private Wayland outputs, without locking.

Login uses mock authentication. The locker only waits for its native PAM prompt;
it never submits credentials. All XDG state and the session bus are disposable.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def session(kind, output):
    base = Path(os.environ['XDG_DATA_HOME']).parent
    if kind == 'lock':
        subprocess.run([sys.executable, str(ROOT/'tools/install-lockscreen.py'),
                        '--prefix', str(base/'data')], check=True)
        shell = base/'data/plasma/shells/org.kde.plasma.desktop'
        main = shell/'contents/lockscreen/LockScreenUi.qml'
        command = ['/usr/libexec/kscreenlocker_greet', '--testing', '--shell', str(shell)]
        target = 'lockScreenUi'
        checks = '''
                wallpaperLocal: !!wallpaperSource && wallpaperSource.Window.window === lockScreenUi.Window.window,
                wallpaperWidth: wallpaperSource ? wallpaperSource.width : 0,
                promptReady: authenticator.hadPrompt && authenticator.promptForSecret.length > 0,
                passwordEnabled: mainBlock.mainPasswordBox.enabled,
                passwordVisible: mainBlock.mainPasswordBox.visible,
                locked: !authenticator.unlocked,
        '''
        setup = 'lockScreenRoot.uiVisible = true;'
    else:
        theme = base/'theme'
        subprocess.run([sys.executable, str(ROOT/'tools/build-login.py'), '--output', str(theme),
                        '--runtime', str(theme)], check=True)
        main = theme/'qml/Main.qml'
        command = ['/usr/libexec/plasma-login-greeter', '--test']
        target = 'root'
        # Expose the wallpaper image status in the disposable test copy only.
        screen = theme/'qml/LoginScreen.qml'
        screen.write_text(screen.read_text().replace('    id: screen',
            '    id: screen\n    readonly property bool testWallpaperReady: testWallpaper.status === Image.Ready', 1)
            .replace('Image { anchors.fill: parent; source: screen.wallpaper;',
                     'Image { id: testWallpaper; anchors.fill: parent; source: screen.wallpaper;', 1))
        checks = 'passwordEnabled: passwordField.enabled, wallpaperLoaded: testWallpaperReady,'
        setup = ''
    main.write_text(main.read_text().replace('    id: ' + target, '    id: ' + target + '''
    Timer { interval: 1200; running: true; onTriggered: { SETUP } }
    Timer {
        interval: 4000; running: true
        onTriggered: {
            const name = TARGET.Screen.name;
            console.warn("SEQUOIA_SCREEN_STATE", JSON.stringify({
                screen: name, width: TARGET.width, height: TARGET.height,
                accelerated: TARGET.GraphicsInfo.api !== GraphicsInfo.Software,
                CHECKS
            }));
            root.grabToImage(function(result) {
                if (result.saveToFile(OUTPUT + "/KIND-" + name + ".png"))
                    console.warn("SEQUOIA_SCREEN_CAPTURE", name);
                else console.error("SEQUOIA_CAPTURE_FAILED", name);
            });
        }
    }
'''.replace('SETUP', setup).replace('TARGET', target).replace('CHECKS', checks)
        .replace('OUTPUT', json.dumps(str(output))).replace('KIND', kind), 1))
    with (output/f'{kind}-kwin.log').open('w') as log:
        compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '720',
            '--output-count', '2', '--no-lockscreen', '--no-global-shortcuts', '--socket', 'sequoia-screen-test'],
            stdout=log, stderr=log)
        try:
            for _ in range(100):
                if (base/'runtime/sequoia-screen-test').exists():
                    break
                if compositor.poll() is not None:
                    raise RuntimeError('Private compositor exited')
                time.sleep(.1)
            else:
                raise RuntimeError('Private compositor did not start')
            env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-screen-test',
                       QT_QUICK_BACKEND='rhi')
            if kind == 'login':
                env['LD_PRELOAD'] = str(base/'theme/bridge.so')
            with (output/f'{kind}-greeter.log').open('w') as greeter_log:
                greeter = subprocess.Popen(command, env=env, stdout=greeter_log, stderr=greeter_log)
                try:
                    time.sleep(7)
                    assert greeter.poll() is None, 'Greeter exited before test completed'
                finally:
                    stop(greeter)
        finally:
            stop(compositor)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--kind', choices=['lock', 'login'])
    parser.add_argument('--session', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if args.session:
        session(args.kind, output)
        return
    for kind in ([args.kind] if args.kind else ['lock', 'login']):
        for path in output.glob(f'{kind}-*.png'):
            path.unlink()
        with tempfile.TemporaryDirectory(prefix='sequoia-screen-') as tmp:
            base = Path(tmp)
            env = dict(os.environ, QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                       QSG_RHI_BACKEND='opengl', LIBGL_ALWAYS_SOFTWARE='1', QT_FORCE_STDERR_LOGGING='1')
            for key in ('LD_PRELOAD', 'DISPLAY', 'WAYLAND_DISPLAY', 'QML_IMPORT_PATH', 'QT_PLUGIN_PATH',
                        'QT_QUICK_CONTROLS_STYLE', 'QT_QUICK_BACKEND'):
                env.pop(key, None)
            for name in ('config', 'data', 'cache', 'state', 'runtime'):
                path = base/name
                path.mkdir(mode=0o700)
                env['XDG_' + name.upper() + ('_DIR' if name == 'runtime' else '_HOME')] = str(path)
            (base/'config/kscreenlockerrc').write_text('[Greeter]\nWallpaperPlugin=org.kde.image\n'
                '[Greeter][Wallpaper][org.kde.image][General]\nImage=' + str(ROOT/'themes/wallpapers/sequoia-forest.jpg') + '\n')
            with (output/f'{kind}-session.log').open('w') as log:
                process = subprocess.Popen(['dbus-run-session', '--', sys.executable, __file__,
                    '--session', '--kind', kind, '--output', str(output)], env=env,
                    stdout=log, stderr=log, start_new_session=True)
                try:
                    assert process.wait(timeout=55) == 0, (output/f'{kind}-session.log').read_text()
                finally:
                    try:
                        os.killpg(process.pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
        log = (output/f'{kind}-greeter.log').read_text()
        errors = [line for line in log.splitlines() if any(marker in line for marker in (
            'same item on different windows', 'ReferenceError:', 'TypeError:', 'Cannot assign',
            'Failed to load lockscreen', 'SEQUOIA_CAPTURE_FAILED', 'Sequoia login:',
            'Updates can only be scheduled', 'no signal of the target matches', 'Binding loop'))]
        assert not errors, '\n'.join(errors)
        states = [json.loads(line.split('SEQUOIA_SCREEN_STATE ', 1)[1])
                  for line in log.splitlines() if 'SEQUOIA_SCREEN_STATE ' in line]
        assert len(states) == 2 and len({s['screen'] for s in states}) == 2, log
        assert len(list(output.glob(f'{kind}-*.png'))) == 2, log
        for state in states:
            assert state['width'] == 1280 and state['height'] == 720 and state['accelerated'], state
            assert state['passwordEnabled'], state
            if kind == 'lock':
                assert all(state[key] for key in ('wallpaperLocal', 'promptReady', 'passwordVisible', 'locked')), state
                assert state['wallpaperWidth'] == state['width'], state
            else:
                assert state['wallpaperLoaded'], state
        (output/f'{kind}-states.json').write_text(json.dumps(states, indent=2) + '\n')
        print(f'PASS: {kind}, two accelerated Wayland outputs, per-window rendering and input readiness', flush=True)


if __name__ == '__main__':
    main()
