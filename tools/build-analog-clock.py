#!/usr/bin/env python3
"""Package the two-preset Sequoia Analog Clock with its upstream licenses."""
import hashlib
import json
from pathlib import Path
import zipfile
from widget_picker import catalog

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'widgets/plasmoids/org.sequoia.analogclock'


def build():
    catalog()
    output = ROOT / 'dist/widgets/sequoia-analog-clock.plasmoid'
    output.parent.mkdir(parents=True, exist_ok=True)
    files = {str(p.relative_to(SOURCE)): p.read_bytes() for p in sorted(SOURCE.rglob('*')) if p.is_file()}
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in files.items():
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        assert json.loads(archive.read('metadata.json'))['KPlugin']['Id'] == SOURCE.name
        assert {name: archive.read(name) for name in files} == files
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_name(output.name + '.sha256').write_text(f'{digest}  {output.name}\n')
    print(f'Built {output}')


if __name__ == '__main__':
    build()
