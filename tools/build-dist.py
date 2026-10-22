#!/usr/bin/env python3
"""Refresh all supported distribution artifacts from current source."""
import subprocess
import sys
from dist_packages import ROOT, DIST, assets, widget_bundle, contents


def main():
    DIST.mkdir(exist_ok=True)
    for script in ('build-analog-clock.py', 'build-digital-clock.py', 'build-grid-test.py'):
        subprocess.run([sys.executable, str(ROOT / 'tools' / script)], check=True)
    assets()
    widget_bundle()
    contents()
    subprocess.run([sys.executable, str(ROOT / 'tools/export-installer.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'tools/verify-dist.py')], check=True)


if __name__ == '__main__':
    main()
