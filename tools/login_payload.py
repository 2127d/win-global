"""Exact, reversible system payload for the custom login presentation."""
import importlib.util
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
THEME = 'usr/local/share/sequoia-eleven/login-screen'
UNIT = 'etc/systemd/user/plasma-login.service.d/60-sequoia-eleven.conf'
FONT = 'usr/local/share/fonts/sequoia-eleven-login/AdwaitaSans'
FILES = {
    *(f'{THEME}/{path.relative_to(ROOT / "login-screen")}'
      for folder in ('qml', 'assets') for path in (ROOT/'login-screen'/folder).rglob('*') if path.is_file()),
    f'{THEME}/bridge.so', f'{THEME}/compatibility.json',
    f'{THEME}/assets/sequoia-forest.jpg', UNIT,
    *(f'{FONT}/{path.relative_to(ROOT / "fonts/AdwaitaSans")}'
      for path in (ROOT/'fonts/AdwaitaSans').rglob('*') if path.is_file()),
}


def stage(destination):
    spec = importlib.util.spec_from_file_location('build_login', ROOT/'tools/build-login.py')
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    builder.build(destination/THEME)
    shutil.copytree(ROOT/'fonts/AdwaitaSans', destination/FONT)
    unit = destination/UNIT
    unit.parent.mkdir(parents=True, exist_ok=True)
    unit.write_text('[Service]\nEnvironment="LD_PRELOAD=/' + THEME + '/bridge.so"\n')
