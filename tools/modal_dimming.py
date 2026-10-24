"""Build and reversibly enable the independent Sequoia KWin modal effect."""
import configparser
import json
import os
from pathlib import Path
import shutil
import subprocess
from installer_platform import build_jobs
from motion_install import dbus_command
from theme_files import copy_file, Receipt

ROOT = Path(__file__).resolve().parents[1]
EFFECT = 'sequoia_modal_dimming'
KEY = EFFECT + 'Enabled'

def plugin_path(plugindir):
    return 'usr/' + plugindir + '/kwin/effects/plugins/' + EFFECT + '.so'

def stage(destination, plugindir):
    build = ROOT / '.staging/modal-build'
    command = ['cmake', '--fresh', '-S', str(ROOT / 'desktop/effects/modal-dimming'), '-B', str(build), '-G', 'Ninja',
               '-DCMAKE_BUILD_TYPE=Release', '-DSEQUOIA_BUILD_TESTS=OFF']
    sdk = ROOT / '.staging/modal-sdk/usr'
    if not Path('/usr/include/kwin').exists() and sdk.exists():
        command += ['-DKWin_DIR=' + str(sdk / 'lib64/cmake/KWin'),
                    '-DSEQUOIA_KWIN_INCLUDE_DIR=' + str(sdk / 'include/kwin'),
                    '-Depoxy_INCLUDE_DIRS=' + str(sdk / 'include'), '-Depoxy_LIBRARIES=/usr/lib64/libepoxy.so.0',
                    '-DLibdrm_INCLUDE_DIR=' + str(sdk / 'include'), '-DLibdrm_LIBRARY=/usr/lib64/libdrm.so.2']
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', str(build_jobs())], check=True)
    target = destination / plugin_path(plugindir)
    target.parent.mkdir(parents=True, exist_ok=True)
    copy_file(build / 'lib/kwin/effects/plugins' / (EFFECT + '.so'), target)

def state_path(prefix):
    return prefix / 'sequoia-eleven/modal-dimming/activation.json'

def config_path():
    return Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config') / 'kwinrc'

def read_key():
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    parser.read(config_path())
    return parser.get('Plugins', KEY, fallback=None)

def write_key(value):
    subprocess.run(['kwriteconfig6', '--file', str(config_path()), '--group', 'Plugins', '--key', KEY,
                    '--notify', *(['--delete'] if value is None else [value])], check=True)

def kwin(method):
    return subprocess.check_output([dbus_command(), 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.' + method,
                                    EFFECT], text=True, timeout=15).strip()

def activate(prefix):
    path = state_path(prefix)
    Receipt.check_parents(path)
    if path.is_symlink(): raise RuntimeError('Modal activation receipt must not be a symlink')
    fresh = not path.exists()
    previous = read_key()
    if fresh:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps({'version': 1, 'original': read_key(), 'config': str(config_path())}) + '\n')
    check_restore(prefix)
    try:
        write_key('true')
        kwin('unloadEffect')
        if kwin('loadEffect') != 'true':
            raise RuntimeError('KWin could not load Sequoia Modal Dimming.')
    except (RuntimeError, subprocess.SubprocessError, OSError):
        write_key(previous)
        if fresh: path.unlink()
        raise
    print('Sequoia Modal Dimming enabled; KWin was not restarted.')

def check_restore(prefix):
    path = state_path(prefix)
    if not path.exists(): return None
    Receipt.check_parents(path)
    if path.is_symlink(): raise RuntimeError('Modal activation receipt must not be a symlink')
    state = json.loads(path.read_text())
    if state.get('version') != 1 or state.get('config') != str(config_path()):
        raise RuntimeError('Modal activation receipt belongs to another configuration directory')
    return state

def restore(prefix):
    state = check_restore(prefix)
    if state is None: return
    # A user's later explicit disable wins over our saved original value.
    original = state['original'] if read_key() == 'true' else read_key()
    kwin('unloadEffect')
    write_key(original)
    state_path(prefix).unlink()
    print('Removed modal dimming activation; unrelated KWin settings were preserved.')
