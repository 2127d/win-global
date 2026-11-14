#!/usr/bin/env python3
"""Exercise real tray menu types in an isolated session without device actions."""
import os
from pathlib import Path
import shlex
import subprocess
from preview_session import ensure_private_session
ensure_private_session()
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                  QT_STYLE_OVERRIDE='Fusion', QT_QPA_PLATFORMTHEME='generic')
root = Path(__file__).resolve().parents[1]
from control_center_menus import build_menu_helper
build_menu_helper(root / 'desktop/control-center/details')
build = root / '.staging/tray-menu-test-runner'
build.mkdir(parents=True, exist_ok=True)
source = root / 'tools/verify-tray-menu-lifetimes.cpp'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs',
    'Qt6QuickTest', 'Qt6Widgets', 'Qt6Qml'], text=True))
subprocess.run(['/usr/lib64/qt6/libexec/moc', str(source), '-o',
    str(build / 'verify-tray-menu-lifetimes.moc')], check=True)
binary = build / 'verify-tray-menu-lifetimes'
subprocess.run(['c++', '-std=c++17', '-fPIC', str(source), '-I' + str(build),
    '-o', str(binary), *flags], check=True)
result = subprocess.run([str(binary), '-input', str(root / 'desktop/control-center/details/tests'),
    '-o', '-,txt'], capture_output=True, text=True, timeout=60)
print(result.stdout, end='')
print(result.stderr, end='')
errors = ('ReferenceError', 'TypeError', 'Binding loop', 'Unable to assign',
          'Cannot assign', 'is not a type')
raise SystemExit(result.returncode or int(any(error in result.stdout + result.stderr for error in errors)))
