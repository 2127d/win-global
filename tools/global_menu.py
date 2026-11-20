"""Build the global-menu applet with atomic category switching."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tempfile
from installer_platform import build_jobs

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.kde.plasma.appmenu.so'
USER_PLUGIN = Path('sequoia-eleven/widget-forks/plugins/plasma/applets') / PLUGIN
USER_RECEIPT = Path('sequoia-eleven/global-menu-installation.json')


def build():
    source = ROOT / 'desktop/global-menu'
    directory = ROOT / 'build/global-menu'
    cache = directory / 'CMakeCache.txt'
    if cache.exists():
        entries = dict(line.split('=', 1) for line in cache.read_text().splitlines()
                       if line.startswith(('CMAKE_HOME_DIRECTORY:INTERNAL=', 'CMAKE_CACHEFILE_DIR:INTERNAL=')))
        if (entries.get('CMAKE_HOME_DIRECTORY:INTERNAL') != str(source)
                or entries.get('CMAKE_CACHEFILE_DIR:INTERNAL') != str(directory)):
            saved = Path(tempfile.mkdtemp(prefix='global-menu-relocated-', dir=directory.parent)) / 'build'
            directory.rename(saved)
    subprocess.run(['cmake', '-S', str(source), '-B', str(directory), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=Release'], check=True)
    subprocess.run(['cmake', '--build', str(directory), '--parallel', str(build_jobs())], check=True)
    return directory / 'bin/plasma/applets' / PLUGIN


def stage(destination, plugin_dir):
    source = build()
    target = destination / 'usr' / plugin_dir / 'plasma/applets' / PLUGIN
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def retire_user_override(data):
    """Retire our exact user override after the system receipt installs a build."""
    receipt, target = data / USER_RECEIPT, data / USER_PLUGIN
    if not receipt.is_file() or not target.is_file():
        return
    owned = json.loads(receipt.read_text())
    if hashlib.sha256(target.read_bytes()).hexdigest() != owned.get('sha256'):
        raise RuntimeError('The local global-menu override was modified; preserve it before updating the system plugin.')
    backup = Path(tempfile.mkdtemp(prefix='global-menu-upgraded-', dir=receipt.parent))
    shutil.copy2(target, backup / PLUGIN)
    shutil.copy2(receipt, backup / receipt.name)
    target.unlink()
    receipt.unlink()


if __name__ == '__main__':
    print(build())
