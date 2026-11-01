#!/usr/bin/env python3
"""Test the generated lock-screen layout with mock authentication and session actions."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sequoia-lock-test-') as temporary:
    subprocess.run(['python3', str(ROOT / 'tools/install-lockscreen.py'), '--prefix', temporary], check=True)
    qml = Path(temporary) / 'plasma/shells/org.kde.plasma.desktop/contents/lockscreen'
    # Load the entire greeter before substituting the unit-test authenticator.
    # MainBlock-only tests cannot catch a new property assigned by LockScreenUi.
    env = dict(os.environ, XDG_DATA_HOME=temporary, QT_QPA_PLATFORM='offscreen',
               QT_QUICK_BACKEND='software', QT_FORCE_STDERR_LOGGING='1')
    greeter = subprocess.Popen(['/usr/libexec/kscreenlocker_greet', '--testing'],
                               env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    try:
        output, _ = greeter.communicate(timeout=5)
        raise RuntimeError('Test greeter exited unexpectedly: ' + output)
    except subprocess.TimeoutExpired:
        greeter.terminate()
        output, _ = greeter.communicate(timeout=5)
    if any(marker in output for marker in ('Failed to load lockscreen', 'unavailable', 'Cannot assign', 'ReferenceError:', 'TypeError:')):
        raise RuntimeError(output)
    print('PASS: complete lock-screen greeter loads without fallback')
    # The enum is registered by the greeter executable, not an importable plugin.
    main = qml / 'MainBlock.qml'
    main.write_text(main.read_text().replace('import org.kde.kscreenlocker as ScreenLocker', '')
                    .replace('ScreenLocker.Authenticator.Fingerprint', '2')
                    .replace('ScreenLocker.Authenticator.Smartcard', '4'))
    shutil.copy2(ROOT / 'themes/lockscreen/tests/tst_unlock.qml', qml)
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_FORCE_STDERR_LOGGING='1')
    subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(qml / 'tst_unlock.qml')], env=env, check=True, timeout=45)
