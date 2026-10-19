#!/usr/bin/env python3
"""Verify KCalc mode sizing and installation with isolated configuration."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time
import re

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT / '.staging/kcalc-build/bin/kcalc')
parser.add_argument('--dark', action='store_true')
args = parser.parse_args()
helper = ROOT / '.staging/verify-kcalc.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test'], text=True))
subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/verify-kcalc.cpp'), '-o', str(helper), *flags], check=True)
output = ROOT / ('preview/kcalc-dark' if args.dark else 'preview/kcalc-light')
output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='kcalc-test-') as temp:
    base = Path(temp)
    env = dict(os.environ)
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
        path = base / kind.lower(); path.mkdir(mode=0o700)
        env[f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'] = str(path)
    variant = 'Dark' if args.dark else 'Light'
    (base / 'config/kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text())
    env.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE='Fusion',
               LD_PRELOAD=str(helper), SEQUOIA_KCALC_VERIFY=str(output), LC_ALL='C.UTF-8')
    result = subprocess.run(['dbus-run-session', '--', str(args.binary.resolve())], env=env, capture_output=True, text=True, timeout=45)
    finished_ms = time.time() * 1000
    closing = re.search(r'CLOSING: (\d+)', result.stderr)
    assert closing, 'Missing normal-close marker'
    exit_ms = finished_ms - int(closing.group(1))
    assert 0 <= exit_ms < 2000, f'Slow normal exit: {exit_ms:.0f} ms'
    print(f'PASS: normal exit and process teardown in {exit_ms:.0f} ms')
    (output / 'verification.log').write_text(result.stdout + result.stderr + f'EXIT: {exit_ms:.0f} ms\n')
    print(result.stderr[-3000:])
    assert result.returncode == 0 and 'PASS:' in result.stderr

with tempfile.TemporaryDirectory(prefix='kcalc-install-test-') as temp:
    base = Path(temp); data = base / 'user data'; bins = base / 'user bin'; bins.mkdir()
    launcher = bins / 'kcalc'
    launcher.write_text('#!/bin/sh\nexit 19\n'); launcher.chmod(0o755)
    command = ['python3', str(ROOT / 'tools/install-kcalc.py'), '--prefix', str(data), '--bin-prefix', str(bins),
               '--build-dir', str(args.binary.resolve().parent.parent), '--skip-build']
    subprocess.run(command, check=True)
    # Installed launcher has no extension.
    from importlib.machinery import SourceFileLoader
    import importlib.util
    spec = importlib.util.spec_from_loader('kcalc_launcher', SourceFileLoader('kcalc_launcher', str(launcher)))
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    assert module.INSTALL_DATA == data
    arguments = ['--version', 'argument with spaces']
    assert module.command(data, Path('/usr/bin/kcalc'), arguments) == [str(data / 'sequoia-eleven/kcalc/kcalc'), *arguments]
    changed = base / 'changed-stock'; changed.write_text('package upgrade')
    assert module.command(data, changed, arguments) == [str(changed), *arguments]
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic')
    assert '26.08.1' in subprocess.check_output([str(launcher), '--version'], env=env, text=True)
    subprocess.run([*command, '--remove', '--check'], check=True)
    subprocess.run([*command, '--remove'], check=True)
    assert launcher.read_text() == '#!/bin/sh\nexit 19\n'
print('PASS: launcher forwarding, package upgrade fallback and reversible installation')
