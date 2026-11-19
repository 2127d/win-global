"""Export the clock's pure-QML controls from the shared component library."""
from pathlib import Path
import argparse

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'desktop/components/library/qml'
TARGET = ROOT / 'widgets/plasmoids/org.sequoia.digitalclock/contents/ui/SequoiaControls'
NAMES = ('GlassPillSurface', 'GlassPillSwitcher', 'GlassPillTab')


def sync(check=False):
    files = {name + '.qml': (SOURCE / (name + '.qml')).read_bytes() for name in NAMES}
    files['qmldir'] = ('# Generated from the shared component library.\n'
                       + ''.join(f'{name} 1.0 {name}.qml\n' for name in NAMES)).encode()
    for name, content in files.items():
        target = TARGET / name
        if target.exists() and target.read_bytes() == content:
            continue
        if check:
            raise RuntimeError('Stale shared widget control; run tools/widget_library_components.py: ' + name)
        TARGET.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    sync(parser.parse_args().check)
