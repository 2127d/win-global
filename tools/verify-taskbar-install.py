#!/usr/bin/env python3
"""Check taskbar installation, updates, tamper rejection and removal in a temporary prefix."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    stage = parser.parse_args().stage.resolve()
    with tempfile.TemporaryDirectory(prefix='sequoia-taskbar-install-') as temporary:
        base = Path(temporary)
        prefix = base / 'usr'
        original = prefix / 'share/plasma/plasmoids/org.sequoia.taskbar/contents/ui/main.qml'
        original.parent.mkdir(parents=True)
        original.write_text('previous taskbar installation\n')
        installer = [sys.executable, str(ROOT / 'tools/install-taskbar.py'), '--prefix', str(prefix)]
        subprocess.run([*installer, '--stage', str(stage)], check=True, capture_output=True)
        subprocess.run([*installer, '--stage', str(stage)], check=True, capture_output=True)
        installed = original.read_text()
        original.write_text('local edit\n')
        assert subprocess.run([*installer, '--remove'], capture_output=True).returncode != 0
        assert original.read_text() == 'local edit\n'
        original.write_text(installed)
        subprocess.run([*installer, '--remove'], check=True, capture_output=True)
        assert original.read_text() == 'previous taskbar installation\n'
        assert not (prefix / 'share/plasma/layout-templates/org.sequoia.taskbar.panel/metadata.json').exists()
        altered = base / 'altered'
        shutil.copytree(stage, altered)
        (altered / 'usr/share/plasma/plasmoids/org.sequoia.taskbar/metadata.json').write_text('{}\n')
        assert subprocess.run([*installer, '--stage', str(altered)], capture_output=True).returncode != 0
        assert original.read_text() == 'previous taskbar installation\n'
    print('PASS: install/update, edited-file protection, backup restoration, and tampered-stage rejection')

if __name__ == '__main__':
    main()
