#!/usr/bin/env python3
"""Install/remove the desktop theme and its portable layout widgets."""
import argparse
import base64
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

from theme_files import Receipt, copy_file, exists, payload_files
from project_paths import asset_source
from installer_platform import (COMPONENTS, DISTROS, NATIVE, detected_distro,
                                default_components, login_available, resolve_components, resolve_removals,
                                install_dependencies, native_layout, packages_for, sudo_command)
import widget_payload
import shell_overrides
from login_payload import FILES as LOGIN_FILES, stage as stage_login

ROOT = Path(__file__).resolve().parents[1]
USER_STATE = 'sequoia-eleven/theme-installation'
SYSTEM_STATE = 'var/lib/sequoia-eleven/theme-installation'
# Compatibility alias for tools that inspect the complete Fedora dependency list.
PACKAGES = tuple(packages_for('fedora', COMPONENTS))
NATIVE_FILES = {
    'usr/lib64/qt6/plugins/styles/sequoiaeleven6.so',
    'usr/share/kstyle/themes/sequoiaeleven.themerc',
    'usr/lib64/qt6/plugins/kstyle_config/sequoiaelevenstyleconfig.so',
    'usr/share/applications/sequoiaelevenstyleconfig.desktop',
    'usr/bin/sequoiaeleven-settings6',
    'usr/share/icons/hicolor/scalable/apps/sequoiaeleven-settings.svgz',
    'usr/lib64/qt6/plugins/org.kde.kdecoration3/org.kde.sequoiaeleven.so',
    'usr/lib64/qt6/plugins/org.kde.kdecoration3/org.kde.sequoiaeleven.solid.so',
    'usr/lib64/qt6/plugins/org.kde.kdecoration3/org.kde.sequoiaeleven.transparent.so',
    'usr/share/kservices6/sequoiaelevendecorationconfig.desktop',
    'usr/lib64/qt6/plugins/org.kde.kdecoration3.kcm/kcm_sequoiaelevendecoration.so',
    'usr/share/applications/kcm_sequoiaelevendecoration.desktop',
    'usr/lib64/cmake/SequoiaEleven/SequoiaElevenConfig.cmake',
    'usr/lib64/cmake/SequoiaEleven/SequoiaElevenConfigVersion.cmake',
}
# Preserve the original Fedora paths in the allowlist for existing receipts.
_FEDORA_NATIVE_FILES = NATIVE_FILES.copy()
LIBDIR, PLUGINDIR = native_layout()
NATIVE_FILES = {name.replace('usr/lib64/qt6/plugins', 'usr/' + PLUGINDIR)
               .replace('usr/lib64/cmake', 'usr/' + LIBDIR + '/cmake') for name in NATIVE_FILES}
SYSTEM_FILES = NATIVE_FILES | LOGIN_FILES
# Accept earlier receipts so upgrades/removal can retire the old login wallpapers.
SYSTEM_ALLOWED_FILES = SYSTEM_FILES | _FEDORA_NATIVE_FILES | {
    f'usr/local/share/sequoia-eleven/login-screen/assets/wallpaper-{mode}.png'
    for mode in ('light', 'dark')
}
APP_FILES = {
    'systemsettings.desktop', 'org.kde.kinfocenter.desktop',
    'org.kde.plasma-systemmonitor.desktop', 'org.kde.discover.desktop',
    'org.kde.merkuro.calendar.desktop', 'org.kde.merkuro.desktop',
}
LAYOUT_WIDGETS = {
    'org.sequoia.startmenu', 'org.sequoia.downloads', 'org.sequoia.dockseparator',
    'org.kpple.kppleMenu', 'org.kde.windowtitle.Fork', 'org.sequoia.desktopmenu',
}
USER_TREES = {
    'icons/SequoiaTahoe', 'icons/SequoiaTahoe-light', 'icons/SequoiaTahoe-dark',
    'wallpapers/SequoiaEleven', 'fonts/sequoia-eleven',
    'plasma/desktoptheme/sequoia-eleven-light', 'plasma/desktoptheme/sequoia-eleven-dark',
    'plasma/shells/org.kde.plasma.desktop', 'sequoia-eleven/quick-islands',
    *(f'plasma/plasmoids/{name}' for name in LAYOUT_WIDGETS),
    *(f'plasma/look-and-feel/org.sequoia.eleven.{mode}{suffix}'
      for mode in ('light', 'dark') for suffix in ('', '.solid', '.transparent')),
}
COLOR_FILES = {f'color-schemes/SequoiaEleven{mode}{material}.colors'
               for mode in ('Light', 'Dark') for material in ('', 'Solid', 'Transparent')}


def system_allowed(name):
    return name in SYSTEM_ALLOWED_FILES or widget_payload.allowed(name, PLUGINDIR)


def validate_system_payload(payload, components):
    files = payload_files(payload)
    widget_files = {n: p for n, p in files.items() if widget_payload.allowed(n, PLUGINDIR)}
    if set(files) - widget_files.keys() != selected_system_files(components):
        raise RuntimeError('System payload does not match the selected components.')
    if 'widgets' in components:
        widget_payload.validate(widget_files, PLUGINDIR)
    elif widget_files:
        raise RuntimeError('Unselected native widgets in payload.')


def user_allowed(name):
    return (name in COLOR_FILES or name in {'applications/' + app for app in APP_FILES}
            or any(name.startswith(tree + '/') for tree in USER_TREES))


def run(*command, env=None):
    print('+', shlex.join(str(x) for x in command), flush=True)
    subprocess.run([str(x) for x in command], check=True, cwd=ROOT, env=env)


def dependencies(distro=None, components=None):
    distro = distro or detected_distro()
    if distro not in DISTROS:
        raise RuntimeError('Choose Arch, Fedora, Ubuntu or Debian, or use --skip-deps.')
    install_dependencies(distro, components if components is not None else COMPONENTS)


def component_for(name):
    if name in LOGIN_FILES or name.startswith('usr/local/share/sequoia-eleven/login-screen/'):
        return 'login'
    if widget_payload.allowed(name, PLUGINDIR):
        return 'widgets'
    if name.startswith('usr/'):
        return 'decorations' if 'decoration' in name else 'application-style'
    for prefix, component in (
        ('icons/', 'icons'), ('wallpapers/', 'wallpaper'), ('fonts/', 'fonts'),
        ('color-schemes/', 'colors'), ('plasma/desktoptheme/', 'plasma-style'),
        ('plasma/look-and-feel/', 'global-theme'), ('plasma/plasmoids/', 'widgets'),
        ('plasma/shells/', 'lockscreen'),
        ('sequoia-eleven/quick-islands/', 'quick-style'), ('applications/', 'quick-style'),
    ):
        if name.startswith(prefix):
            return component
    return None


def selected_system_files(components):
    return {name for name in SYSTEM_FILES if component_for(name) in components}


def user_scope(name, components):
    if shell_overrides.is_shell(name):
        return shell_overrides.scope(components)
    return component_for(name) in components


def system_action(action, args, payload=None):
    scope = lambda name: component_for(name) in args.components
    if args.system_root != Path('/'):
        receipt = Receipt(args.system_root, SYSTEM_STATE, system_allowed)
        if action == 'install':
            receipt.install(payload, force=args.force,
                            original_overrides=dict.fromkeys(NATIVE_FILES),
                            scope=lambda name: component_for(name) in args.components)
        elif action == 'check':
            receipt.validate(force=args.force, scope=scope)
        else:
            receipt.uninstall(force=args.force, scope=scope)
        return
    command = sudo_command(sys.executable, str(Path(__file__).resolve()), '_system', action)
    command += ['--components', *args.components]
    if payload:
        command += ['--payload', str(payload)]
    if args.force:
        command += ['--force']
    run(*command)


def stage_user(stage, prefix, components=None):
    components = set(COMPONENTS if components is None else components)
    stage.mkdir(parents=True, exist_ok=True)
    # Build everything before replacing any live theme files. Existing launcher
    # originals/receipts are seeded so refreshes preserve customized Exec lines.
    environment = dict(os.environ, XDG_CACHE_HOME=str(stage.parent / 'cache'))
    managed = Receipt(prefix, USER_STATE, user_allowed).data['files']
    # Copy only selected artwork; partial installs do not need native build tools.
    for tree in sorted(USER_TREES):
        if component_for(tree + '/') in components and tree not in (
                'fonts/sequoia-eleven', 'plasma/shells/org.kde.plasma.desktop', 'sequoia-eleven/quick-islands'):
            source = asset_source(tree, ROOT)
            if not source.is_dir():
                raise RuntimeError(f'Missing installer source: {source}. Extract the complete installer archive.')
            if tree.startswith('plasma/plasmoids/') and exists(prefix / tree) and not any(
                    name.startswith(tree + '/') for name in managed):
                # A user's separately installed widget must keep its version and
                # ownership. Only fresh installs and our own files are managed.
                print(f'Keeping separately installed layout widget: {prefix / tree}')
                continue
            shutil.copytree(source, stage / tree, symlinks=True)
    for name in COLOR_FILES:
        if 'colors' in components:
            (stage / name).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(asset_source(name, ROOT), stage / name)
    if 'plasma-style' in components:
        for mode in ('light', 'dark'):
            (stage / f'plasma/desktoptheme/sequoia-eleven-{mode}/colors').unlink(missing_ok=True)
    if 'icons' in components and shutil.which('gtk-update-icon-cache'):
        for theme in sorted((stage / 'icons').iterdir()):
            run('gtk-update-icon-cache', '--force', '--ignore-theme-index', theme, env=environment)
    if 'fonts' in components:
        run(sys.executable, ROOT / 'tools/install-fonts.py', '--prefix', stage, env=environment)
    if shell_overrides.scope(components):
        shell_overrides.stage(stage, prefix, components, managed)
    if 'quick-style' not in components:
        return
    quick = Path('sequoia-eleven/quick-islands')
    marker = prefix / quick / 'installation.json'
    if (prefix / quick).exists() and not marker.is_file():
        raise RuntimeError('Existing Quick application overlay is not managed by Sequoia.')
    if marker.exists():
        (stage / quick).mkdir(parents=True, exist_ok=True)
        shutil.copy2(marker, stage / quick / marker.name)
    for name in APP_FILES:
        source = prefix / 'applications' / name
        if exists(source):
            (stage / 'applications').mkdir(exist_ok=True)
            copy_file(source, stage / 'applications' / name)
    run(sys.executable, ROOT / 'tools/install-quick-islands.py', '--prefix', stage,
        '--launcher-prefix', prefix, env=environment)
    # Only modified/managed launchers belong to this installation.
    launchers = json.loads((stage / quick / 'installation.json').read_text())['launchers']
    for name in APP_FILES - launchers.keys():
        (stage / 'applications' / name).unlink(missing_ok=True)


def original_user_files(stage, temporary):
    # Theme-specific paths also adopt old manual installations. Desktop launchers
    # are shared with KDE applications: restore the originals kept by the Quick
    # installer, including originals saved before this unified installer existed.
    originals = dict.fromkeys(payload_files(stage))
    marker = stage / 'sequoia-eleven/quick-islands/installation.json'
    launchers = json.loads(marker.read_text())['launchers'] if marker.exists() else {}
    for name, entry in launchers.items():
        if name not in APP_FILES:
            raise RuntimeError(f'Unexpected launcher in Quick installation: {name}')
        if entry['original'] is not None:
            saved = temporary / name
            saved.write_bytes(base64.b64decode(entry['original'], validate=True))
            originals['applications/' + name] = saved
    return originals


def legacy_files(prefix, components=None):
    """Adopt stale files inside explicitly named theme trees on first installation."""
    result = []
    for name in USER_TREES:
        if components is not None and component_for(name + "/") not in components:
            continue
        # Untracked widgets belong to their separate installer. stage_user keeps
        # them; first-install cleanup must also leave them alone.
        if name.startswith('plasma/plasmoids/'):
            continue
        path = prefix / name
        if path.is_symlink():
            raise RuntimeError(f'Theme directory must not be a symlink: {path}')
        if path.is_dir():
            result.extend(name + '/' + child for child in payload_files(path))
    return result


def refresh(prefix):
    if shutil.which('fc-cache'):
        run('fc-cache', '-f')
    if prefix == Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').absolute():
        if shutil.which('kbuildsycoca6'):
            run('kbuildsycoca6', '--noincremental')


def main():
    # Elevated code handles only explicitly listed theme files and the greeter
    # presentation drop-in. Authentication and active services are untouched.
    if sys.argv[1:2] == ['_system']:
        parser = argparse.ArgumentParser()
        parser.add_argument('mode')
        parser.add_argument('action', choices=['install', 'uninstall', 'check'])
        parser.add_argument('--payload', type=Path)
        parser.add_argument('--components', nargs='+', choices=COMPONENTS, default=list(COMPONENTS))
        parser.add_argument('--force', action='store_true')
        args = parser.parse_args()
        if os.geteuid() != 0:
            raise RuntimeError('Native plugin installation requires administrator privileges.')
        receipt = Receipt(Path('/'), SYSTEM_STATE, system_allowed)
        scope = lambda name: component_for(name) in args.components
        if args.action == 'install':
            if not args.payload:
                raise RuntimeError('Missing native payload.')
            validate_system_payload(args.payload, args.components)
            receipt.install(args.payload, force=args.force,
                            original_overrides=dict.fromkeys(NATIVE_FILES),
                            scope=lambda name: component_for(name) in args.components)
            if shutil.which('restorecon'):
                run('restorecon', '-F', *(str(Path('/') / name) for name in sorted(payload_files(args.payload))))
        elif args.action == 'check':
            receipt.validate(force=args.force, scope=scope)
        else:
            receipt.uninstall(force=args.force, scope=scope)
        return
    action = sys.argv[1] if len(sys.argv) > 1 else ''
    if action not in ('install', 'uninstall'):
        raise RuntimeError('Use ./install.sh or ./uninstall-gui.py.')
    parser = argparse.ArgumentParser(prog=f'install-theme.py {action}', description=__doc__)
    parser.add_argument('--prefix', type=Path,
                        default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'),
                        help='User data directory (default: XDG_DATA_HOME or ~/.local/share).')
    parser.add_argument('--system-root', type=Path, default=Path('/'),
                        help='Alternate system root for staging/testing; default / needs sudo.')
    parser.add_argument('--distro', choices=DISTROS, default=detected_distro() or None)
    parser.add_argument('--components', nargs='+', choices=COMPONENTS,
                        help='Select components; installation includes required assets, removal includes dependent presets.')
    parser.add_argument('--skip-deps', action='store_true', help='Use already-installed build dependencies.')
    parser.add_argument('--keep-system', action='store_true',
                        help='Uninstall only this account; retain shared native plugins and login theme.')
    parser.add_argument('--force', action='store_true', help='Replace/remove locally edited managed files.')
    parser.add_argument('--plan', action='store_true', help='Print scope and destinations without changing anything.')
    parser.add_argument('--check', action='store_true', help='Check removal receipts without removing files (uninstall only).')
    args = parser.parse_args(sys.argv[2:])
    args.action = action
    requested = args.components
    if action == 'install':
        args.components = resolve_components(requested if requested is not None else default_components())
    else:
        args.components = resolve_removals(requested if requested is not None else COMPONENTS)
    if requested is None and not login_available() and action == 'install':
        print('Login screen omitted: Plasma Login Manager is not installed.')
    added = set(args.components) - set(requested or args.components)
    if added:
        print(('Required by the selected presets: ' if action == 'install' else 'Also removing dependent presets: ')
              + ', '.join(COMPONENTS[c][0] for c in args.components if c in added))
    args.prefix = args.prefix.absolute()
    args.system_root = args.system_root.absolute()
    if args.keep_system and args.action != 'uninstall':
        parser.error('--keep-system is only for uninstall')
    if args.check and action != 'uninstall':
        parser.error('--check is only for uninstall')
    if args.system_root != Path('/') and not args.skip_deps and args.action == 'install':
        parser.error('Use --skip-deps with --system-root; staging must not install host packages')
    print(f'{args.action.title()} theme: ' + ', '.join(COMPONENTS[c][0] for c in args.components))
    print(f'User data: {args.prefix}\nNative plugin root: {args.system_root}')
    print('Desktop widgets include the custom tray, trash, animated task manager and layout widgets. '
          'Desktop settings change only when you apply a theme/layout. The login theme takes effect after reboot.')
    if args.plan:
        if args.action == 'install' and not args.skip_deps:
            if args.distro not in DISTROS:
                parser.error('Select --distro arch, fedora, ubuntu or debian.')
            print(f'{DISTROS[args.distro]} dependencies:', ' '.join(packages_for(args.distro, args.components)))
        return
    if os.geteuid() == 0:
        raise RuntimeError('Run this script as the desktop user, without sudo. '
                           'It requests sudo only for dependencies and shared theme/login files.')
    receipt = Receipt(args.prefix, USER_STATE, user_allowed)
    if args.action == 'uninstall':
        current_shell = shell_overrides.installed(receipt.data['files'])
        change_shell = bool(current_shell & set(args.components))
        scope = lambda name: change_shell if shell_overrides.is_shell(name) else component_for(name) in args.components
        receipt.validate(force=args.force, scope=scope)
        retained_shell = current_shell - set(args.components)
        shared = not args.keep_system and bool(set(args.components) & (NATIVE | {'widgets', 'login'}))
        shared = shared and (args.system_root / SYSTEM_STATE).exists()
        # Check both receipts before starting removal so local edits fail early.
        if shared:
            system_action('check', args)
        if args.check:
            print('Removal checks passed.')
            return
        if change_shell and retained_shell:
            # Both independent options share a complete shell package. Retain
            # the other layer when removing just the style or the lock screen.
            with tempfile.TemporaryDirectory(prefix='sequoia-shell-remove-') as temporary:
                shell_overrides.stage(Path(temporary), args.prefix, (), receipt.data['files'],
                                      removing=args.components)
                receipt.install(temporary, force=args.force, scope=shell_overrides.is_shell)
            scope = lambda name: not shell_overrides.is_shell(name) and component_for(name) in args.components
        receipt.uninstall(force=args.force, scope=scope)
        if shared:
            system_action('uninstall', args)
        refresh(args.prefix)
        print('Removed tracked theme files and restored replaced files. '
              'Shared distribution packages and separately installed widgets were retained. Reboot to reload the login screen.')
        return
    components = set(args.components)
    scope = lambda name: user_scope(name, components)
    receipt.validate(force=args.force, scope=scope)
    if 'login' in components and not login_available():
        raise RuntimeError('The login theme requires Plasma Login Manager. Deselect the login screen on systems using SDDM.')
    if not args.skip_deps:
        dependencies(args.distro, components)
    with tempfile.TemporaryDirectory(prefix='sequoia-theme-install-') as temporary:
        stage = Path(temporary)
        native = stage / 'native'
        native.mkdir()
        if components & NATIVE:
            run('bash', ROOT / 'tools/build-desktop.sh',
                env=dict(os.environ, SEQUOIA_STAGE_DIR=str(native),
                         SEQUOIA_WITH_DECORATIONS='ON' if 'decorations' in components else 'OFF'))
            built_files = NATIVE_FILES if 'decorations' in components else selected_system_files({'application-style'})
            if set(payload_files(native)) != built_files:
                raise RuntimeError('CMake native install list changed; review the theme file allowlist.')
            for name, path in payload_files(native).items():
                if component_for(name) not in components:
                    path.unlink()
        if 'login' in components:
            stage_login(native)
        if 'widgets' in components:
            widget_payload.stage(native, PLUGINDIR)
        validate_system_payload(native, components)
        user = stage / 'user'
        stage_user(user, args.prefix, components)
        incoming = payload_files(user)
        receipt.validate(incoming, args.force, scope=lambda name: name in incoming or scope(name))
        obsolete = legacy_files(args.prefix, components) if not receipt.manifest.exists() else ()
        for name in obsolete:
            receipt.target(name)
        if payload_files(native):
            system_action('install', args, native)
        if payload_files(user):
            receipt.install(user, force=args.force,
                            original_overrides=original_user_files(user, stage), obsolete=obsolete,
                            scope=lambda name: user_scope(name, components))
    refresh(args.prefix)
    print('Installed. Select the installed theme components in System Settings. '
          'Close and reopen applications to load their new style. '
          + ('Reboot for the login theme. ' if 'login' in components else '')
          + 'Remove with ./uninstall-gui.py.')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        sys.exit(f'Theme installer: {error}')
