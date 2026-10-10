#!/usr/bin/env python3
"""Package the editable Start menu as dist/widgets/start-menu.tar.gz."""
import json
from pathlib import Path
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'plasma/plasmoids/org.sequoia.startmenu'


def build(destination=None):
    destination = destination or ROOT / 'dist/widgets/start-menu.tar.gz'
    metadata = json.loads((SOURCE / 'metadata.json').read_text())
    assert metadata['KPlugin']['Id'] == SOURCE.name
    assert (SOURCE / 'contents/ui/main.qml').is_file()
    ET.parse(SOURCE / 'contents/config/main.xml')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(destination, 'w:gz') as archive:
        archive.add(SOURCE, arcname=SOURCE.name)
    print(f'Packaged {destination}')
    return destination


if __name__ == '__main__':
    build()
