#!/usr/bin/env python3
"""Compile and export the shared untinted glass surface into Activity Bar."""
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'desktop/glass-widget'
TARGET = ROOT / 'widgets/plasmoids/org.sequoia.activitybar/contents/ui'


def build():
    qt_bins = subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_BINS'], text=True).strip()
    subprocess.run([str(Path(qt_bins) / 'qsb'), '--glsl', '100 es,120,150',
                    '--hlsl', '50', '--msl', '12', '-o', str(SOURCE / 'pure-glass.frag.qsb'),
                    str(SOURCE / 'pure-glass.frag')], check=True)
    for name in ('PureGlassSurface.qml', 'pure-glass.frag', 'pure-glass.frag.qsb'):
        shutil.copy2(SOURCE / name, TARGET / name)


if __name__ == '__main__':
    build()
