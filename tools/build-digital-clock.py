#!/usr/bin/env python3
"""Build the standalone Sequoia Digital Clock review package."""
import json
from pathlib import Path
import zipfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'plasma/plasmoids/org.sequoia.digitalclock'


def build():
    metadata = json.loads((SOURCE / 'metadata.json').read_text())
    assert metadata['KPlugin']['Id'] == SOURCE.name
    ET.parse(SOURCE / 'contents/config/main.xml')
    assert (SOURCE / 'contents/ui/main.qml').is_file()
    destination = ROOT / 'dist/widgets/sequoia-digital-clock.plasmoid'
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(SOURCE.rglob('*')):
            if path.is_file():
                archive.write(path, path.relative_to(SOURCE))
    print(f'Packaged {destination}')


if __name__ == '__main__':
    build()
