"""Keep GTK's shared native/portal icon setting independent of the KDE theme."""
import configparser
import io
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess

from theme_files import Receipt

SCHEMA = 'org.gnome.desktop.interface'
KEY = 'icon-theme'
SETTINGS = {
    'kded5rc': ('Module-gtkconfig', 'autoload', 'false'),
    'gtk-3.0/settings.ini': ('Settings', 'gtk-icon-theme-name', 'Adwaita'),
    'gtk-4.0/settings.ini': ('Settings', 'gtk-icon-theme-name', 'Adwaita'),
}
XSETTINGS = 'xsettingsd/xsettingsd.conf'
XICON = re.compile(r'^Net/IconThemeName\s+.*$', re.M)


def config_home():
    return Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config').absolute()


def live_install(prefix, system_root):
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').absolute()
    return Path(system_root) == Path('/') and Path(prefix).absolute() == data


def state_path(prefix):
    return Path(prefix) / 'sequoia-eleven/gtk-icons/activation.json'


def command(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=15).strip()


def kded(method, *args):
    return command('gdbus', 'call', '--session', '--dest', 'org.kde.kded6',
                   '--object-path', '/kded', '--method', 'org.kde.kded6.' + method, *args)


def session_available():
    try:
        kded('loadedModules')
        return True
    except (OSError, subprocess.SubprocessError):
        return False


def parser_for(path):
    Receipt.check_parents(path)
    if path.is_symlink():
        raise RuntimeError(f'GTK settings must not be a symlink: {path}')
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    parser.read(path)
    return parser


def read_values():
    values = {}
    for name, (section, key, _) in SETTINGS.items():
        values[name] = parser_for(config_home() / name).get(section, key, fallback=None)
    path = config_home() / XSETTINGS
    Receipt.check_parents(path)
    if path.is_symlink():
        raise RuntimeError(f'GTK settings must not be a symlink: {path}')
    matches = XICON.findall(path.read_text()) if path.exists() else []
    values[XSETTINGS] = matches[-1] if matches else None
    return values


def write_values(values):
    for name, (section, key, _) in SETTINGS.items():
        path = config_home() / name
        parser = parser_for(path)
        if values[name] is None:
            if not parser.has_option(section, key):
                continue
            parser.remove_option(section, key)
        else:
            if not parser.has_section(section):
                parser.add_section(section)
            parser.set(section, key, values[name])
        output = io.StringIO()
        parser.write(output, space_around_delimiters=False)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(output.getvalue())
    path = config_home() / XSETTINGS
    Receipt.check_parents(path)
    if path.is_symlink():
        raise RuntimeError(f'GTK settings must not be a symlink: {path}')
    text = path.read_text() if path.exists() else ''
    replacement = values[XSETTINGS]
    if XICON.search(text):
        text = XICON.sub(lambda match: replacement or '', text)
    elif replacement:
        text = text.rstrip('\n') + ('\n' if text else '') + replacement + '\n'
    else:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def desired_values():
    return {**{name: setting[2] for name, setting in SETTINGS.items()},
            XSETTINGS: 'Net/IconThemeName "Adwaita"'}


def set_gsettings(value):
    command('gsettings', 'set', SCHEMA, KEY, value)
    # gsettings can return success even if the dconf writer could not reach D-Bus.
    if command('gsettings', 'get', SCHEMA, KEY) != value:
        raise RuntimeError('Could not save GTK icon settings. Run the installer in your desktop session.')


def reload_xsettings():
    if not shutil.which('pgrep'):
        print('Log out and back in to refresh X11 GTK icons.')
        return
    result = subprocess.run(['pgrep', '-u', str(os.getuid()), '-x', 'xsettingsd'],
                            text=True, capture_output=True, timeout=10)
    for pid in result.stdout.split():
        try:
            os.kill(int(pid), signal.SIGHUP)
        except ProcessLookupError:
            pass
        except PermissionError:
            print('Log out and back in to refresh X11 GTK icons.')


def check_restore(prefix):
    path = state_path(prefix)
    Receipt.check_parents(path)
    if path.is_symlink():
        raise RuntimeError('GTK icon activation receipt must not be a symlink')
    if not path.exists():
        return None
    state = json.loads(path.read_text())
    if (state.get('version') != 1 or state.get('config') != str(config_home())
            or set(state.get('values', {})) != set(desired_values())
            or not isinstance(state.get('gsettings'), str)
            or not all(value is None or isinstance(value, str) for value in state['values'].values())
            or state.get('autoload') not in (None, True, False)
            or type(state.get('loaded')) is not bool):
        raise RuntimeError('Invalid GTK icon receipt or a different configuration directory')
    return state


def snapshot():
    live = session_available()
    return {'version': 1, 'config': str(config_home()), 'values': read_values(),
            'gsettings': command('gsettings', 'get', SCHEMA, KEY),
            'autoload': kded('isModuleAutoloaded', 'gtkconfig') == '(true,)' if live else None,
            'loaded': 'gtkconfig' in kded('loadedModules') if live else False}


def restore_snapshot(state, preserve_edits=True):
    current = read_values()
    values = {name: original if not preserve_edits or current[name] == desired_values()[name]
              else current[name] for name, original in state['values'].items()}
    current_icon = command('gsettings', 'get', SCHEMA, KEY)
    edited_icons = current_icon != "'Adwaita'" or any(
        current[name] != desired_values()[name] for name in current if name != 'kded5rc')
    restore_module = not preserve_edits or (current['kded5rc'] == 'false' and not edited_icons)
    # Re-enabling the sync writer would immediately clobber later GTK choices.
    if preserve_edits and edited_icons:
        values['kded5rc'] = current['kded5rc']
    live = session_available()
    if live and restore_module and state['autoload'] is not None:
        kded('setModuleAutoloading', 'gtkconfig', str(state['autoload']).lower())
    write_values(values)
    if not preserve_edits or current_icon == "'Adwaita'":
        set_gsettings(state['gsettings'])
    if live and restore_module and state['loaded']:
        kded('loadModule', 'gtkconfig')
    reload_xsettings()


def activate(prefix):
    path = state_path(prefix)
    existing = check_restore(prefix)
    before = snapshot()
    if existing is None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(before, indent=2) + '\n')
    try:
        # Stop the writer before changing settings. This prevents login and
        # later KDE theme changes from restoring Sequoia icons in GTK/portals.
        if session_available():
            kded('setModuleAutoloading', 'gtkconfig', 'false')
            kded('unloadModule', 'gtkconfig')
            if 'gtkconfig' in kded('loadedModules'):
                raise RuntimeError('KDE GTK synchronization could not be stopped')
        write_values(desired_values())
        set_gsettings("'Adwaita'")
        reload_xsettings()
    except (RuntimeError, OSError, subprocess.SubprocessError):
        restore_snapshot(before, preserve_edits=False)
        if existing is None:
            path.unlink()
        raise
    print('GTK icons: Adwaita for native and Flatpak GTK apps (including GTK 3). '
          'KDE icons are unchanged; automatic KDE-to-GTK appearance syncing is disabled. '
          'Restart GTK apps; log out and back in if X11 icons remain cached.')


def restore(prefix):
    state = check_restore(prefix)
    if state is None:
        return
    restore_snapshot(state)
    state_path(prefix).unlink()
    print('Restored GTK icon preferences and KDE GTK synchronization; later user edits were preserved.')
