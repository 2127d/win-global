"""Shared, narrowly scoped Plasma Login appearance configuration."""
import configparser
import io
import os
from pathlib import Path
import tempfile


def read_config(*paths):
    config = configparser.ConfigParser(interpolation=None, strict=False)
    config.optionxform = str
    for path in paths:
        if path.is_file():
            # Only read small settings files, even when invoked by the system service.
            with path.open() as stream:
                text = stream.read(1024 * 1024 + 1)
            if len(text) > 1024 * 1024:
                raise ValueError(f'Configuration is too large: {path}')
            config.read_string(text)
    return config


def set_values(config, group, values):
    if not config.has_section(group):
        config.add_section(group)
    for key, value in values.items():
        config.set(group, key, str(value))


def save_config(config, path):
    stream = io.StringIO()
    config.write(stream, space_around_delimiters=False)
    text = stream.getvalue()
    if path.exists() and path.read_text() == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=path.parent, prefix='.' + path.name)
    try:
        with os.fdopen(fd, 'w') as output:
            output.write(text)
        os.chmod(temporary, 0o644)
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def desktop_mode(config_dir):
    config = read_config(config_dir/'kdedefaults/kdeglobals', config_dir/'kdeglobals')
    scheme = config.get('General', 'ColorScheme', fallback='')
    for mode in ('light', 'dark'):
        if scheme in (f'SequoiaEleven{mode.title()}{suffix}' for suffix in ('', 'Solid', 'Transparent')):
            return mode
    # A different color scheme is an explicit choice; leave the greeter alone.
    if scheme:
        return None
    package = config.get('KDE', 'LookAndFeelPackage', fallback='')
    return next((mode for mode in ('light', 'dark')
                 if package in (f'org.sequoia.eleven.{mode}{suffix}' for suffix in ('', '.solid', '.transparent'))), None)


def apply_appearance(config_dir, assets, mode):
    globals_path = config_dir/'kdeglobals'
    config = read_config(globals_path)
    palette = read_config(assets/'color-schemes'/f'SequoiaEleven{mode.title()}.colors')
    for group in palette.sections():
        set_values(config, group, dict(palette.items(group)))
    for group, values in {
        'KDE': {'LookAndFeelPackage': f'org.sequoia.eleven.{mode}',
                'widgetStyle': 'SequoiaEleven', 'AutomaticLookAndFeel': 'false'},
        'General': {'font': 'Adwaita Sans,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1,,0,0',
                    'XftAntialias': 'true', 'XftHintStyle': 'hintslight', 'XftSubPixel': 'none'},
        'Icons': {'Theme': 'breeze'},
    }.items():
        set_values(config, group, values)
    config.remove_option('General', 'ColorSchemeHash')
    save_config(config, globals_path)
    plasma = read_config(config_dir/'plasmarc')
    set_values(plasma, 'Theme', {'name': f'sequoia-eleven-{mode}'})
    save_config(plasma, config_dir/'plasmarc')


def sync_installed():
    import json
    import pwd
    settings = json.loads(Path('/etc/sequoia-eleven-login.json').read_text())
    mode = desktop_mode(Path(settings['source_config']))
    if mode is None:
        print('Desktop is not using Sequoia Eleven; retaining the last login appearance.')
        return
    account = pwd.getpwnam('plasmalogin')
    # The only privileged read is theme selection. All writes run as the greeter,
    # and all palettes/assets are installed and owned by root.
    os.setgroups([])
    os.setgid(account.pw_gid)
    os.setuid(account.pw_uid)
    apply_appearance(Path(account.pw_dir)/'.config', Path('/usr/local/share'), mode)
    print(f'Login appearance synchronized: Sequoia Eleven {mode.title()}')


if __name__ == '__main__':
    sync_installed()
