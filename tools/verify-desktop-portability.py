#!/usr/bin/env python3
"""Exercise build-desktop.sh with real CMake after copying/moving a checkout."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def build(project):
    result = subprocess.run(['bash', str(project / 'tools/build-desktop.sh')],
                            cwd='/', capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    assert (project / '.staging/desktop/usr/share/sequoia-fixture/fixture.txt').read_text() == 'portable\n'
    cache = (project / 'build/desktop/CMakeCache.txt').read_text()
    assert f'CMAKE_HOME_DIRECTORY:INTERNAL={project}/application-style/SequoiaEleven\n' in cache
    assert f'CMAKE_CACHEFILE_DIR:INTERNAL={project}/build/desktop\n' in cache
    return result.stdout


with tempfile.TemporaryDirectory(prefix='sequoia-build-portability-') as temporary:
    root = Path(temporary)
    original = root / 'original account/checkout'
    source = original / 'application-style/SequoiaEleven'
    source.mkdir(parents=True)
    (original / 'tools').mkdir()
    shutil.copy2(ROOT / 'tools/build-desktop.sh', original / 'tools')
    (source / 'fixture.txt').write_text('portable\n')
    (source / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(PortabilityFixture NONE)
install(FILES fixture.txt DESTINATION share/sequoia-fixture)
''')
    build(original)
    marker = original / 'build/desktop/preserve-me'
    marker.write_text('previous build\n')
    assert 'Checkout moved' not in build(original)
    assert marker.is_file()
    copied = root / 'guest account/checkout'
    shutil.copytree(original, copied)
    assert 'Checkout moved' in build(copied)
    saved = list((copied / 'build').glob('desktop-relocated-*/desktop/preserve-me'))
    assert len(saved) == 1 and saved[0].read_text() == marker.read_text()
    assert marker.is_file(), 'Copy must not alter the original checkout'
    moved = root / 'renamed checkout'
    copied.rename(moved)
    assert 'Checkout moved' in build(moved)
    assert 'Checkout moved' not in build(moved)
print('PASS: fresh, incremental, copied and moved CMake builds, paths with spaces, preserved old build')
