#!/usr/bin/env python3
"""Narrow, journaled Sequoia surface trial. No layout, icon, font or wallpaper edits."""
import argparse
import configparser
import fcntl
import json
import os
from pathlib import Path
import selectors
import signal
import subprocess
import sys
import time

CONFIG = Path(os.environ.get('XDG_CONFIG_HOME', Path.home() / '.config'))
DATA = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
STATE = Path(os.environ.get('XDG_STATE_HOME', Path.home() / '.local/state')) / 'sequoia-eleven/appearance-hub'
QDBUS = '/usr/lib64/qt6/bin/qdbus'
EFFECT = 'sequoia_liquid_glass_v6'


def read(file, group, key):
    config = configparser.ConfigParser(interpolation=None, strict=False, allow_no_value=True)
    config.optionxform = str
    config.read(CONFIG / file)
    return config.get(group, key, fallback=None)


def write(entry, value):
    # Plasma Theme and KWin use KConfigWatcher. Writing a file alone leaves
    # their cached theme/color state unchanged (even after KWin.reconfigure).
    command = ['kwriteconfig6', '--notify', '--file', entry['file'], '--group', entry['group'], '--key', entry['key']]
    subprocess.run(command + (['--delete'] if value is None else [value]), check=True, timeout=5, stdout=subprocess.DEVNULL, env={**os.environ, 'XDG_CONFIG_HOME': str(CONFIG)})
    if value is None:
        path = CONFIG / entry['file']
        lines = path.read_text().splitlines(keepends=True)
        section, kept = None, []
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('[') and stripped.endswith(']'): section = stripped[1:-1]
            if section == entry['group'] and stripped in (entry['key'] + '[$d]', entry['key'] + '[$d]='): continue
            kept.append(line)
        if kept != lines:
            temp = path.with_suffix('.appearance-tmp'); temp.write_text(''.join(kept)); temp.replace(path)



def plan(material, color_mode='current'):
    if material not in ('solid', 'transparent', 'glass'):
        raise ValueError('Unknown material')
    color = read('kdeglobals', 'Colors:Window', 'BackgroundNormal') or '242,243,245'
    rgb = [int(v) for v in color.split(',')[:3]]
    dark = sum(rgb) < 384
    if color_mode not in ('current', 'light', 'dark'):
        raise ValueError('Unknown color mode')
    mode = ('dark' if dark else 'light') if color_mode == 'current' else color_mode
    dark = mode == 'dark'
    palette = {}
    if color_mode != 'current':
        scheme = 'SequoiaEleven' + mode.title() + ('Solid' if material == 'solid' else 'Transparent')
        candidates = [DATA / 'color-schemes' / (scheme + '.colors'), Path('/usr/share/color-schemes') / (scheme + '.colors')]
        source = next((p for p in candidates if p.is_file()), None)
        if source is None:
            raise RuntimeError('The matching Sequoia color scheme is not installed.')
        config = configparser.ConfigParser(interpolation=None, strict=False)
        config.optionxform = str
        config.read(source)
        for group in config.sections():
            if group.startswith('Colors:') or group in ('ColorEffects:Disabled', 'ColorEffects:Inactive', 'WM'):
                for key, value in config.items(group): palette[(group, key)] = value
        color = palette.get(('Colors:Window', 'BackgroundNormal'), color)
    style = 'SequoiaElevenGlass' if material == 'glass' else 'SequoiaEleven'
    decoration = 'org.kde.sequoiaeleven.' + ('glass-adjustable' if material == 'glass' else material)
    theme = 'sequoia-eleven-' + ('glass-' if material == 'glass' else '') + mode
    plugin = Path('/usr/lib64/qt6/plugins/org.kde.kdecoration3') / (decoration + '.so')
    if not plugin.is_file() or not (DATA / 'plasma/desktoptheme' / theme / 'metadata.json').is_file():
        raise RuntimeError('The matching Sequoia decoration or Plasma theme is not installed.')
    alpha = 255 if material == 'solid' else (192 if dark else 184) if material == 'transparent' else (168 if dark else 148)
    desired = [('kdeglobals', 'KDE', 'widgetStyle', style),
               ('kwinrc', 'org.kde.kdecoration2', 'library', decoration),
               ('kwinrc', 'Plugins', EFFECT + 'Enabled', str(material == 'glass').lower()),
               ('kwinrc', 'Plugins', 'blurEnabled', str(material == 'transparent').lower()),
               ('plasmarc', 'Theme', 'name', theme),
               ('sequoiaelevenrc', 'Style', 'MenuOpacity', str(100 if material == 'solid' else 72 if material == 'transparent' else 44))]
    if color_mode != 'current':
        desired.extend(('kdeglobals', group, key, value) for (group, key), value in palette.items())
        desired.extend([('kdeglobals', 'General', 'ColorScheme', scheme),
                        ('kdeglobals', 'KDE', 'AutomaticLookAndFeel', 'false')])
    for group in ('Colors:Window', 'Colors:Header'):
        for key in ('BackgroundNormal', 'BackgroundAlternate'):
            original = palette.get((group, key)) or read('kdeglobals', group, key) or color
            desired.append(('kdeglobals', group, key, ','.join(original.split(',')[:3] + [str(alpha)])))
    for key in ('activeBackground', 'inactiveBackground'):
        original = palette.get(('WM', key)) or read('kdeglobals', 'WM', key) or color
        desired.append(('kdeglobals', 'WM', key, ','.join(original.split(',')[:3] + [str(alpha)])))
    unique = {(f, g, k): v for f, g, k, v in desired}
    return [dict(file=f, group=g, key=k, before=read(f, g, k), after=v) for (f, g, k), v in unique.items()]


def effect_call(method, *args):
    return subprocess.check_output([QDBUS, 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.' + method, *args], text=True, timeout=8).strip()


def sync_effects():
    # These are mutually exclusive providers. Disable the outgoing provider
    # before attempting the incoming one; Glass::supported() checks stock Blur.
    loaded = set(effect_call('loadedEffects').splitlines())
    glass = read('kwinrc', 'Plugins', EFFECT + 'Enabled') == 'true'
    blur_value = read('kwinrc', 'Plugins', 'blurEnabled')
    blur = blur_value != 'false' and not glass  # Stock Blur's inherited default.
    desired = {EFFECT} if glass else {'blur'} if blur else set()
    providers = {EFFECT, 'blur'}
    for name in sorted((loaded & providers) - desired):
        effect_call('unloadEffect', name)
    loaded = set(effect_call('loadedEffects').splitlines())
    for name in sorted(desired - loaded):
        if effect_call('loadEffect', name) != 'true' and name not in effect_call('loadedEffects').splitlines():
            raise RuntimeError('KWin could not load ' + name)
    actual = set(effect_call('loadedEffects').splitlines()) & providers
    if actual != desired:
        raise RuntimeError('KWin material providers did not reach the requested state.')
    if glass:
        effect_call('reconfigureEffect', EFFECT)


def refresh():
    sync_effects()
    subprocess.run([QDBUS, 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure'], check=True, timeout=8, stdout=subprocess.DEVNULL)
    # Recheck after configuration reload rather than trusting a load request.
    sync_effects()
    subprocess.run(['dbus-send', '--session', '--type=signal', '/KGlobalSettings', 'org.kde.KGlobalSettings.notifyChange', 'int32:0', 'int32:0'], check=True, timeout=5)
    subprocess.run(['dbus-send', '--session', '--type=signal', '/KGlobalSettings', 'org.kde.KGlobalSettings.notifyChange', 'int32:2', 'int32:0'], check=True, timeout=5)
    subprocess.run(['dbus-send', '--session', '--type=signal', '/SequoiaElevenStyle', 'org.kde.SequoiaEleven.Style.reparseConfiguration'], check=True, timeout=5)
    theme = read('plasmarc', 'Theme', 'name')
    if theme:
        subprocess.run(['plasma-apply-desktoptheme', theme], check=True, timeout=10, stdout=subprocess.DEVNULL)


def save(state):
    temp = STATE / 'receipt.tmp'
    temp.write_text(json.dumps(state, indent=2) + '\n')
    temp.chmod(0o600)
    temp.replace(STATE / 'receipt.json')


def restore(state):
    if state['config'] != str(CONFIG):
        raise RuntimeError('This receipt belongs to another configuration directory.')
    for entry in reversed(state['changes']):
        # A manual edit made during the trial wins over the saved baseline.
        if read(entry['file'], entry['group'], entry['key']) == entry['after']:
            write(entry, entry['before'])
    refresh()
    state['status'] = 'restored'
    save(state)


def emit(message, seconds=0):
    try:
        print(json.dumps(dict(message=message, seconds=seconds)), flush=True)
    except BrokenPipeError:
        pass  # Parent died; still complete restoration.


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('material', choices=('solid', 'transparent', 'glass', 'restore', 'recover'))
    parser.add_argument('--mode', choices=('current', 'light', 'dark'), default='current')
    args = parser.parse_args()
    STATE.mkdir(parents=True, exist_ok=True, mode=0o700)
    with (STATE / 'lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        receipt = STATE / 'receipt.json'
        previous = json.loads(receipt.read_text()) if receipt.exists() else None
        if previous and (previous['status'] == 'pending' or (args.material == 'restore' and previous['status'] == 'kept')):
            restore(previous)
            emit('Previous material restored. Some apps may need reopening.')
        if args.material in ('restore', 'recover'):
            if not previous or previous['status'] == 'restored': emit('No pending material test to restore.')
            return
        state = dict(version=1, config=str(CONFIG), status='pending', changes=plan(args.material, args.mode))
        save(state)  # Journal before the first mutation, including interrupted application.
        canceled = False
        def stop(signum, frame):
            nonlocal canceled
            canceled = True
        signal.signal(signal.SIGTERM, stop)
        signal.signal(signal.SIGINT, stop)
        try:
            for entry in state['changes']:
                write(entry, entry['after'])
            refresh()
            selector = selectors.DefaultSelector()
            selector.register(sys.stdin, selectors.EVENT_READ)
            deadline = time.monotonic() + 15
            last = -1
            while not canceled:
                remaining = max(0, int(deadline - time.monotonic() + 0.999))
                if remaining != last:
                    emit('Keep this material, or the previous look returns automatically.', remaining)
                    last = remaining
                if not remaining:
                    break
                if selector.select(min(1, deadline - time.monotonic())):
                    command = sys.stdin.readline().strip()
                    if command == 'keep':
                        state['status'] = 'kept'
                        save(state)
                        emit('Material kept. Undo last test can restore the previous look.')
                        return
                    break  # Revert, EOF (parent crash), or unexpected input.
        finally:
            if state['status'] == 'pending':
                restore(state)
                emit('Previous material restored. Some apps may need reopening.')


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
