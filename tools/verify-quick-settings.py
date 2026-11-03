#!/usr/bin/env python3
"""Exercise the real tile editor and layout engine with private, simulated services."""
from pathlib import Path
import os, subprocess, sys, tempfile
from preview_session import ensure_private_session
ensure_private_session()
root = Path(__file__).resolve().parents[1]
env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_STYLE_OVERRIDE='Fusion', QT_QPA_PLATFORMTHEME='generic', QT_QUICK_CONTROLS_STYLE='org.kde.desktop')
command = ['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(root / 'desktop/control-center/tests'), '-o', '-,txt']
result = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
print(result.stdout)
errors = [line for line in result.stdout.splitlines() if any(x in line for x in ('ReferenceError', 'TypeError', 'Binding loop', 'Unable to assign', 'Cannot assign', 'is not a type', 'polish() loop'))]
raise SystemExit(result.returncode or bool(errors))
