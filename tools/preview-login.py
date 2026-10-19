#!/usr/bin/env python3
"""Capture the custom theme in the real greeter's isolated mock-authentication mode."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('build_login', ROOT/'tools/build-login.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def capture(mode, width, height, output, renderer="software", backdrop=None, demo_status=False, status_panel=None, user_switcher=False):
    output.parent.mkdir(parents=True, exist_ok=True)
    output.unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-login-preview-') as temporary:
        staging = Path(temporary)
        theme = staging/'theme'
        builder.build(theme, theme)
        config = staging/'config'
        config.mkdir()
        (config/'kdeglobals').write_text('[General]\nfont=Adwaita Sans,10,-1,5,50,0,0,0,0,0\n')
        main = theme/'qml/Main.qml'
        # Power buttons are present in the design preview, with actions stubbed.
        source = main.read_text().replace('readonly property bool canPowerOff: PlasmaLogin.SessionManagement.canShutdown', 'readonly property bool canPowerOff: true')
        source = source.replace('readonly property bool canSuspend: PlasmaLogin.SessionManagement.canSuspend', 'readonly property bool canSuspend: true')
        source = source.replace('PlasmaLogin.SessionManagement.suspend();', '')
        source = source.replace('readonly property bool canReboot: PlasmaLogin.SessionManagement.canReboot', 'readonly property bool canReboot: true')
        source = source.replace('PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip);', '')
        source = source.replace('PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip);', '')
        if demo_status:
            (theme/'qml/StatusFixture.qml').write_text((ROOT/'themes/login-screen/tests/StatusFixture.qml').read_text())
            source = source.replace('systemStatus: SystemStatus {}', 'systemStatus: StatusFixture {}')
        if backdrop:
            source = source.replace('    wallpaper: "../assets/sequoia-forest.jpg"',
                                    '    wallpaper: ' + json.dumps(backdrop.as_uri()))
        main.write_text(source.replace('    id: root', f'''    id: root
    Timer {{
        interval: 100; running: true
        onTriggered: {{
            root.Window.window.width = {width};
            root.Window.window.height = {height};
            PlasmaLogin.GreeterState.activateWindow(root.Window.window);
            root.focusInput();
            root.Controls.Overlay.overlay.parent = root;
            {'root.userMenu.open();' if user_switcher else ''}
            {('root.statusIsland.showSection(' + json.dumps(status_panel) + ');') if status_panel else ''}
        }}
    }}
    Timer {{
        interval: 2400; running: true
        onTriggered: root.grabToImage(function(result) {{
            if (result.saveToFile("{output}")) console.log("SEQUOIA_LOGIN_CAPTURE_OK");
            else console.error("CAPTURE_FAILED");
            Qt.quit();
        }})
    }}''', 1))
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software' if renderer == 'software' else 'rhi', QSG_RHI_BACKEND='opengl',
                   QT_FORCE_STDERR_LOGGING='1', QT_QPA_PLATFORMTHEME='generic',
                   LD_PRELOAD=str(theme/'bridge.so'), XDG_CONFIG_HOME=str(config),
                   XDG_CACHE_HOME=str(staging/'cache'))
        process = subprocess.Popen([str(builder.GREETER), '--test'], env=env,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        try:
            log, _ = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            process.terminate()
            log, _ = process.communicate(timeout=5)
        log += subprocess.run(['journalctl', '--user', '--no-pager', '-o', 'cat', f'_PID={process.pid}'],
                              capture_output=True, text=True).stdout
        output.with_suffix('.log').write_text(log)
        errors = [line for line in log.splitlines() if any(term in line for term in
                  ('ReferenceError:', 'TypeError:', 'Cannot assign', 'is not installed', 'unavailable',
                   'CAPTURE_FAILED', 'Sequoia login:', 'Binding loop'))]
        assert not errors, '\n'.join(errors)
        assert output.exists() and output.stat().st_size > 1000, log
        print('Real Plasma Login test greeter loaded without QML errors. Saved', output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('light', 'dark'), default='light')
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=800)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--renderer', choices=('software', 'opengl'), default='software')
    parser.add_argument('--wallpaper', type=Path, help='Alternate wallpaper for diagnostic blur previews.')
    parser.add_argument('--demo-status', action='store_true', help='Use simulated hardware status for a design preview; no system controls are called.')
    parser.add_argument('--status-panel', choices=('network', 'audio', 'battery', 'clock', 'session'), help='Open a status popover or the desktop-session menu in the capture.')
    parser.add_argument('--user-switcher', action='store_true', help='Show the inline account picker with the native greeter user model.')
    args = parser.parse_args()
    default_name = 'login-status-demo' if args.demo_status else f'login-{args.mode}'
    if args.status_panel:
        default_name += '-' + args.status_panel
    if args.user_switcher:
        default_name += '-users'
    capture(args.mode, args.width, args.height, (args.output or ROOT/'preview'/f'{default_name}.png').resolve(), args.renderer, args.wallpaper.resolve() if args.wallpaper else None, args.demo_status, args.status_panel, args.user_switcher)
