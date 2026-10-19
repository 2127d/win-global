#!/usr/bin/env python3
"""Test login interactions, native fallback, and reversible installation in /tmp."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch

from login_payload import FILES, UNIT, stage
from theme_files import Receipt, payload_files

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('build_login', ROOT/'tools/build-login.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
           QT_QPA_PLATFORMTHEME='generic', QT_FORCE_STDERR_LOGGING='1')

with tempfile.TemporaryDirectory(prefix='sequoia-login-verify-') as temporary:
    base = Path(temporary)
    payload = base/'payload'
    stage(payload)
    assert set(payload_files(payload)) == FILES
    live = base/'system'
    unit = live/UNIT
    unit.parent.mkdir(parents=True)
    unit.write_text('# pre-existing local customization\n')
    def receipt():
        return Receipt(live, 'var/lib/sequoia-eleven/test-login', FILES.__contains__)
    receipt().install(payload)
    receipt().install(payload)
    assert 'LD_PRELOAD=' in unit.read_text()
    receipt().uninstall()
    assert unit.read_text() == '# pre-existing local customization\n'
    assert all(not (live/name).exists() for name in FILES - {UNIT})
    print('PASS: exact login payload, repeat installation, original drop-in restored, all added files removed')

    theme = base/'theme'
    builder.build(theme, theme)
    subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(ROOT/'themes/login-screen/tests')],
                   env=dict(env, LD_PRELOAD=str(theme/'bridge.so')), check=True)
    def greeter_log(timeout=2):
        process = subprocess.Popen([str(builder.GREETER), '--test'],
            env=dict(env, LD_PRELOAD=str(theme/'bridge.so'), XDG_CONFIG_HOME=str(base/'config'),
                     XDG_CACHE_HOME=str(base/'cache')),
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        try:
            output, _ = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.terminate()
            output, _ = process.communicate(timeout=5)
        output += subprocess.run(['journalctl', '--user', '--no-pager', '-o', 'cat', f'_PID={process.pid}'],
                                 text=True, capture_output=True).stdout
        return output
    main = theme/'qml/Main.qml'
    source = main.read_text()
    main.write_text(source.replace('    id: root', '''    id: root
    Timer {
        interval: 200; running: true
        onTriggered: {
            const model = PlasmaLogin.SessionModel;
            const count = model.rowCount();
            if (count < 1) { console.error("SESSION_SELECTION_FAILED: no sessions"); return; }
            const original = session.currentIndex;
            const next = (original + 1) % count;
            root.sessionSelected(next);
            const row = model.index(next, 0);
            const valid = session.currentIndex === next && root.sessionIndex === next
                && PlasmaLogin.GreeterState.sessionIndex === next
                && session.currentSessionFileName === model.data(row, PlasmaLogin.SessionModel.FileNameRole)
                && session.currentSessionType === model.data(row, PlasmaLogin.SessionModel.TypeRole);
            root.sessionSelected(original);
            if (valid) console.warn("SEQUOIA_SESSION_SELECTION_OK");
            else console.error("SESSION_SELECTION_FAILED");
        }
    }
    Timer {
        interval: 600; running: true
        onTriggered: {
            if (!native.users.length) { console.error("USER_SELECTION_FAILED: no users"); return; }
            PlasmaLogin.GreeterState.activateWindow(root.Window.window);
            const next = (native.currentIndex + 1) % native.users.length;
            native.selectManual();
            root.userMenu.open();
            root.userMenu.choose(next);
            Qt.callLater(function() {
                const valid = !root.userMenu.opened && root.loginCard.visible
                    && !native.manual && native.currentIndex === next
                    && root.user.name === native.users[next].name
                    && (!root.passwordRequired || root.passwordField.activeFocus);
                if (valid) console.warn("SEQUOIA_USER_SELECTION_OK");
                else console.error("USER_SELECTION_FAILED");
            });
        }
    }''', 1))
    log = greeter_log(timeout=5)
    assert 'SEQUOIA_SESSION_SELECTION_OK' in log, log
    assert 'SESSION_SELECTION_FAILED' not in log, log
    print('PASS: tray session selection updates the native greeter selection and login session payload')
    assert 'SEQUOIA_USER_SELECTION_OK' in log, log
    assert 'USER_SELECTION_FAILED' not in log, log
    print('PASS: inline account picker updates the native selected user and restores password focus')
    main.unlink()
    assert 'Sequoia login: using stock greeter' in greeter_log()
    (theme/'qml/Main.qml').write_text('Deliberately invalid QML for the fallback test\n')
    assert 'Sequoia login: theme failed to load; restoring stock greeter.' in greeter_log()
    changed_binary = base/'updated-greeter'
    changed_binary.write_bytes(builder.GREETER.read_bytes() + b'changed version')
    with patch.object(builder, 'GREETER', changed_binary):
        builder.build(theme, theme)
    log = greeter_log()
    assert 'Sequoia login: using stock greeter' in log
    assert 'Error loading QML file' not in log
    print('PASS: stock greeter fallback for missing theme, invalid QML, and changed greeter binary')
print('No login manager services, user sessions, authentication settings, or power actions were changed.')
