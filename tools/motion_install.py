#!/usr/bin/env python3
"""Stage, apply, or restore the approved Sequoia animation palette for this user."""
import argparse
import configparser
from contextlib import contextmanager
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from motion_palette import prepare, EFFECTS, effect_sources, kwin_effect
from theme_files import copy_file, fingerprint


def data_home():
    return Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').absolute()


def config_home():
    return Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config').absolute()


def run(*args):
    if args[0] == 'qdbus-qt6':
        args = (dbus_command(), *args[1:])
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=40).strip()


def dbus_command():
    for command in ('qdbus-qt6', 'qdbus6', '/usr/lib/qt6/bin/qdbus', '/usr/lib64/qt6/bin/qdbus'):
        if shutil.which(command):
            return command
    raise RuntimeError('Animation kit requires the Qt 6 qdbus utility.')


def preflight(data):
    if data.resolve() != data_home().resolve():
        raise RuntimeError('Apply the Animation kit only to the current desktop account, not an alternate prefix.')
    source = effect_sources()
    for name in EFFECTS:
        kwin_effect(name, (source / name / 'contents/code/main.js').read_text())
    run('qdbus-qt6', 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.loadedEffects')


def active_path(data):
    return data / 'sequoia-eleven/motion-installation/active.json'


def active_backup(data):
    path = active_path(data)
    return Path(json.loads(path.read_text())['backup']) if path.exists() else None


def install(data):
    config = config_home()
    preflight(data)
    with tempfile.TemporaryDirectory(prefix='sequoia-animation-kit-') as temporary:
        stage = Path(temporary) / 'palette'
        prepare(stage, data, config)
        return apply(stage)


@contextmanager
def theme_installation(data, components):
    """Remove overlays before refreshing their base files, then reapply safely.

    Restoring before reapplication keeps the original settings as the baseline
    on every upgrade. A failed theme build also reinstates an existing kit.
    """
    affected = bool(set(components) & {'animation-kit', 'widgets', 'quick-style', 'lockscreen'})
    previous = active_backup(data) if affected else None
    requested = 'animation-kit' in components
    if previous or requested:
        preflight(data)
    if previous:
        restore(previous)
    try:
        yield
    except BaseException:
        if previous:
            install(data)
        raise
    else:
        if previous or requested:
            install(data)


def effect_call(method, name):
    return run('qdbus-qt6', 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.' + method, name)


def notify_factor(config):
    # KWin watches KConfig's change notification, not just reconfigure(). Force
    # a notification even when the same palette is reapplied. Restore the exact
    # original value (or absence), using an imperceptible intermediate delta.
    filename = str(config / 'kdeglobals')
    value = run('kreadconfig6', '--file', filename, '--group', 'KDE', '--key', 'AnimationDurationFactor')
    args = ('kwriteconfig6', '--file', filename, '--group', 'KDE', '--key', 'AnimationDurationFactor', '--notify')
    run(*args, str(float(value or '1') + .000000001))
    if value:
        run(*args, value)
    else:
        run(*args, '--delete')


def reload(effects, applying):
    run('systemctl', '--user', 'daemon-reload')
    run('qdbus-qt6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure')
    for effect in effects:
        effect_call('unloadEffect', effect if applying else 'sequoia-motion-' + effect)
        if applying:
            effect_call('unloadEffect', 'sequoia-motion-' + effect)
    if applying:
        effect_call('unloadEffect', 'slidingnotifications')
        if effect_call('isEffectLoaded', 'slidingpopups') != 'true':
            effect_call('loadEffect', 'slidingpopups')
        if effect_call('isEffectLoaded', 'slidingpopups') != 'true':
            raise RuntimeError('Could not enable the panel hide/show animation')
    if applying:
        for effect in effects:
            if effect_call('loadEffect', 'sequoia-motion-' + effect) != 'true':
                if effect_call('isEffectLoaded', 'sequoia-motion-' + effect) != 'true':
                    raise RuntimeError('Could not load motion effect: ' + effect)
    for effect in ('overview', 'windowview', 'slide', 'magiclamp', 'slidingpopups'):
        effect_call('reconfigureEffect', effect)
    notify_applications()
    run('systemctl', '--user', 'restart', 'plasma-plasmashell.service')
    if run('systemctl', '--user', 'is-active', 'plasma-plasmashell.service') != 'active':
        raise RuntimeError('Plasma failed to restart')


def notify_applications():
    # Broadcast the style's real reload signal so existing Qt Widgets repaint.
    from PySide6.QtCore import QCoreApplication
    from PySide6.QtDBus import QDBusConnection, QDBusMessage
    app = QCoreApplication.instance() or QCoreApplication([])
    message = QDBusMessage.createSignal('/SequoiaElevenStyle', 'org.kde.SequoiaEleven.Style', 'reparseConfiguration')
    QDBusConnection.sessionBus().send(message)
    # KGlobalSettings notifies Kirigami/platform themes of the workspace factor.
    message = QDBusMessage.createSignal('/KGlobalSettings', 'org.kde.KGlobalSettings', 'notifyChange')
    message.setArguments([3, 0])
    QDBusConnection.sessionBus().send(message)
    app.processEvents()


def prune_empty_directories(manifest):
    roots = {Path(manifest['plan']['data']), Path(manifest['plan']['config'])}
    parents = set()
    for entry in manifest['files']:
        for parent in Path(entry['target']).parents:
            if parent in roots:
                break
            parents.add(parent)
    for parent in sorted(parents, key=lambda p: len(p.parts), reverse=True):
        try:
            parent.rmdir()
        except OSError:
            pass


def configuration_changes(manifest, backup):
    """Restore only our settings, preserving runtime and unrelated user changes."""
    config = Path(manifest['plan']['config'])
    snapshots = {}
    for entry in manifest['files']:
        path = Path(entry['target'])
        if path.parent != config:
            continue
        parser = configparser.ConfigParser(interpolation=None, strict=False)
        parser.optionxform = str
        if entry['saved']:
            parser.read(backup / entry['saved'])
        snapshots[path.name] = parser
    settings = [(file, [group], key, value) for file, group, key, value in manifest['plan']['settings']]
    applet_file = 'plasma-org.kde.plasma.desktop-appletsrc'
    applets = snapshots[applet_file]
    for section in applets.sections():
        plugin = applets.get(section, 'plugin', fallback='')
        keys = {'org.sequoia.startmenu': {'animationDuration': '200', 'animationsEnabled': 'true'},
                'org.sequoia.icontasks.animated': {'dockSpeed': '100', 'dockAnimations': 'true'}}.get(plugin)
        if keys:
            for key, value in keys.items():
                settings.append((applet_file, section.split('][') + ['Configuration', 'General'], key, value))
    result = []
    for file, groups, key, value in settings:
        args = ['--file', str(config / file)]
        for group in groups:
            args += ['--group', group]
        args += ['--key', key]
        original = snapshots[file].get(']['.join(groups), key, fallback=None)
        result.append((args, original, value))
    return result


def is_theme_receipt(target, manifest):
    return target == Path(manifest['plan']['data']) / 'sequoia-eleven/theme-installation/manifest.json'


def update_theme_receipt(manifest):
    """Update just overlaid file hashes; retain unrelated theme installations."""
    data = Path(manifest['plan']['data'])
    receipt = data / 'sequoia-eleven/theme-installation/manifest.json'
    if not receipt.exists():
        return
    tracked = json.loads(receipt.read_text())
    for entry in manifest['files']:
        target = Path(entry['target'])
        if target.is_relative_to(data):
            name = str(target.relative_to(data))
            if name in tracked['files']:
                tracked['files'][name]['installed'] = fingerprint(target)
    receipt.write_text(json.dumps(tracked, indent=2) + '\n')


def check_restore(backup):
    manifest = json.loads((backup / 'manifest.json').read_text())
    config = Path(manifest['plan']['config'])
    changes = configuration_changes(manifest, backup)
    for args, original, installed in changes:
        current = run('kreadconfig6', *args)
        if current != installed:
            raise RuntimeError('A motion setting changed since installation; preserve it before restoring: ' + ' '.join(args))
    for entry in manifest['files']:
        target = Path(entry['target'])
        if target.parent == config or is_theme_receipt(target, manifest):
            continue
        if fingerprint(target) != entry['installed']:
            raise RuntimeError(f'File changed since motion installation; preserve it before restoring: {target}')
    return manifest


def restore(backup):
    manifest = check_restore(backup)
    config = Path(manifest['plan']['config'])
    changes = configuration_changes(manifest, backup)
    run('systemctl', '--user', 'stop', 'plasma-plasmashell.service')
    try:
        for entry in reversed(manifest['files']):
            target = Path(entry['target'])
            if target.parent == config or is_theme_receipt(target, manifest):
                continue
            target.unlink(missing_ok=True)
            if entry['saved']:
                copy_file(backup / entry['saved'], target)
        for args, original, installed in changes:
            run('kwriteconfig6', *args, '--notify', *(['--delete'] if original is None else [original]))
        update_theme_receipt(manifest)
        prune_empty_directories(manifest)
        for name in (*EFFECTS, *('sequoia-motion-' + e for e in EFFECTS), 'slidingpopups', 'slidingnotifications'):
            effect_call('unloadEffect', name)
        notify_factor(config)
        run('systemctl', '--user', 'daemon-reload')
        run('qdbus-qt6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure')
        for original in manifest['originalEffects']:
            effect_call('loadEffect', original)
        notify_applications()
    finally:
        run('systemctl', '--user', 'restart', 'plasma-plasmashell.service')
    data = Path(manifest['plan']['data'])
    if active_backup(data) == backup:
        active_path(data).unlink()
    print('Restored motion backup:', backup)


def apply(stage):
    plan = json.loads((stage / 'plan.json').read_text())
    data, config = Path(plan['data']), Path(plan['config'])
    if data != data_home():
        raise RuntimeError('Stage was prepared for a different data directory')
    if config != config_home():
        raise RuntimeError('Stage was prepared for a different config directory')
    if active_backup(data):
        raise RuntimeError('An Animation kit is already active. Use --install to refresh it while retaining the original settings.')
    import hashlib
    for source, expected in plan['sources'].items():
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != expected:
            raise RuntimeError('KWin changed since staging; rebuild the palette')
    originals = run('qdbus-qt6', 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.loadedEffects').splitlines()
    backup = data / 'sequoia-eleven/backups' / ('motion-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True, mode=0o700)
    mapping = {}
    for kind, root in [('data', data), ('config', config)]:
        for path in (stage / kind).rglob('*'):
            if path.is_file() or path.is_symlink():
                mapping[root / path.relative_to(stage / kind)] = path
    config_files = {config / row[0] for row in plan['settings']}
    config_files.add(config / 'plasma-org.kde.plasma.desktop-appletsrc')
    receipt = data / 'sequoia-eleven/theme-installation/manifest.json'
    targets = list(dict.fromkeys([*mapping, *config_files, *([receipt] if receipt.exists() else [])]))
    entries = []
    for index, target in enumerate(targets):
        saved = None
        if target.exists() or target.is_symlink():
            saved = 'files/' + str(index)
            dest = backup / saved
            dest.parent.mkdir(exist_ok=True)
            shutil.copy2(target, dest, follow_symlinks=False)
        entries.append({'target': str(target), 'saved': saved, 'installed': None})
    manifest = {'files': entries, 'loadedEffectsBefore': originals, 'originalEffects': [e for e in originals
        if e in (*EFFECTS, 'slidingpopups', 'slidingnotifications') or e.startswith('sequoia-motion-')], 'plan': plan}
    (backup / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Animation settings and files captured before applying:', backup, flush=True)
    try:
        for target, source in mapping.items():
            target.parent.mkdir(parents=True, exist_ok=True)
            copy_file(source, target)
        for file, group, key, value in plan['settings']:
            run('kwriteconfig6', '--file', str(config / file), '--group', group, '--key', key, '--notify', value)
            actual = run('kreadconfig6', '--file', str(config / file), '--group', group, '--key', key)
            if actual != value:
                raise RuntimeError(f'Could not verify {file}/{group}/{key}: {actual}')
        script = '''
var count = 0;
panels().forEach(p => p.widgets().forEach(w => {
    if (w.type === "org.sequoia.startmenu" || w.type === "org.sequoia.icontasks.animated") {
        w.currentConfigGroup = ["General"];
        if (w.type === "org.sequoia.startmenu") { w.writeConfig("animationDuration", 200); w.writeConfig("animationsEnabled", true); }
        else { w.writeConfig("dockSpeed", 100); w.writeConfig("dockAnimations", true); }
        w.reloadConfig(); count++;
    }
}));
print("Updated " + count + " motion widgets");
'''
        print(run('qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript', script))
        # Preserve the theme installer's ownership records for refreshed files.
        update_theme_receipt(manifest)
        # Allow Plasma's KConfig writes to reach disk before reloading it.
        import time
        time.sleep(2)
        notify_factor(config)
        reload(plan['effects'], True)
        if effect_call('isEffectLoaded', 'overview') == 'true':
            info = effect_call('supportInformation', 'overview')
            if 'animationDuration: 200' not in info:
                raise RuntimeError('KWin workspace timing did not update: ' + info)
    except BaseException:
        # Stop the shell before restoring its configuration so a pending save
        # cannot overwrite the snapshot during failure recovery.
        run('systemctl', '--user', 'stop', 'plasma-plasmashell.service')
        for entry in reversed(entries):
            target = Path(entry['target'])
            target.unlink(missing_ok=True)
            if entry['saved']:
                copy_file(backup / entry['saved'], target)
        for effect in EFFECTS:
            effect_call('unloadEffect', 'sequoia-motion-' + effect)
        prune_empty_directories(manifest)
        notify_factor(config)
        run('qdbus-qt6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure')
        for effect in manifest['originalEffects']:
            effect_call('loadEffect', effect)
        notify_applications()
        run('systemctl', '--user', 'daemon-reload')
        run('systemctl', '--user', 'restart', 'plasma-plasmashell.service')
        raise
    for entry in entries:
        entry['installed'] = fingerprint(Path(entry['target']))
    (backup / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    marker = active_path(data)
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(json.dumps({'backup': str(backup)}, indent=2) + '\n')
    print('Applied approved motion palette.')
    print('Backup:', backup)
    print('Restore: python3 tools/install-motion-palette.py --restore ' + str(backup))
    for change in plan['changes']:
        print('Applied:', change)
    for note in plan['limitations']:
        print('Note:', note)
    return backup


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--stage', type=Path)
    group.add_argument('--apply-stage', type=Path)
    group.add_argument('--restore', type=Path)
    group.add_argument('--install', action='store_true', help='Apply Your palette, preserving the original settings across upgrades.')
    group.add_argument('--remove', action='store_true', help='Restore the active kit’s pre-installation snapshot.')
    args = parser.parse_args()
    data = data_home()
    if args.install:
        with theme_installation(data, ['animation-kit']):
            pass
    elif args.remove:
        backup = active_backup(data)
        if backup:
            restore(backup)
        else:
            print('No managed Animation kit is installed.')
    elif args.stage:
        plan = prepare(args.stage.resolve(), data, config_home())
        print(json.dumps(plan, indent=2))
    elif args.apply_stage:
        apply(args.apply_stage.resolve())
    else:
        restore(args.restore.resolve())


if __name__ == '__main__':
    main()
