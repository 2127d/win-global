#!/usr/bin/env python3
"""Install/restore Sequoia CSS without changing bookmarks, extensions, or prefs.js."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent
RECEIPT = '.sequoia-floating-install.json'
BEGIN = '// BEGIN Sequoia Eleven Floating\n'
END = '// END Sequoia Eleven Floating\n'
PREFS = BEGIN + 'user_pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);\n' + END


def discover():
    found = []
    for root in (Path.home() / '.config/mozilla/firefox', Path.home() / '.mozilla/firefox',
                 Path.home() / '.var/app/org.mozilla.firefox/.mozilla/firefox'):
        ini = configparser.ConfigParser()
        ini.read(root / 'profiles.ini')
        defaults = {ini[s].get('Default') for s in ini.sections() if s.startswith('Install')}
        for section in ini.sections():
            if not section.startswith('Profile'):
                continue
            value = ini[section]
            path = Path(value['Path'])
            if value.get('IsRelative', '1') == '1':
                path = root / path
            if path.is_dir() and path not in [p for p, _ in found]:
                found.append((path, value['Path'] in defaults))
    return found


def atomic_write(path, data):
    fd, name = tempfile.mkstemp(prefix='.sequoia-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as file:
            file.write(data)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def fingerprints(folder):
    return {str(p.relative_to(folder)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(folder.rglob('*')) if p.is_file()}


def without_block(data):
    text = data.decode('utf-8')
    if BEGIN in text:
        start = text.index(BEGIN)
        end = text.index(END, start) + len(END)
        text = text[:start] + text[end:]
    return text.encode('utf-8')


def install(profile, restore=False, dry_run=False):
    profile = profile.expanduser().resolve(strict=True)
    if not profile.is_dir():
        raise ValueError('Profile must be a directory')
    chrome, userjs, receipt = profile / 'chrome', profile / 'user.js', profile / RECEIPT
    state = json.loads(receipt.read_text()) if receipt.exists() else None
    if state and (chrome.is_symlink() or fingerprints(chrome) != state['files']):
        raise ValueError('Installed chrome files changed; save your edits before updating/restoring.')
    if restore and not state:
        raise ValueError('No Sequoia installation receipt in this profile.')
    print(('Restore' if restore else 'Install') + ': ' + str(profile))
    if dry_run:
        print('Dry run: no files changed. Existing chrome and user.js will be backed up.')
        return
    if restore:
        backup = profile / state['backup']
        if not backup.is_dir():
            raise ValueError('Backup folder is missing; refusing restoration.')
        shutil.rmtree(chrome)
        saved = backup / 'chrome'
        if saved.exists() or saved.is_symlink():
            saved.rename(chrome)
        current = userjs.read_bytes() if userjs.exists() else b''
        if hashlib.sha256(current).hexdigest() == state['userjs_hash']:
            if state['had_userjs']:
                atomic_write(userjs, (backup / 'user.js').read_bytes())
            else:
                userjs.unlink(missing_ok=True)
        else:
            atomic_write(userjs, without_block(current))
        receipt.unlink()
        print('Previous CSS restored. Restart Firefox. Backup retained at ' + str(backup))
        return
    previous = userjs.read_bytes() if userjs.exists() else b''
    # Validate before any mutation; never rewrite a malformed managed block.
    updated = without_block(previous)
    if updated and not updated.endswith(b'\n'):
        updated += b'\n'
    updated += PREFS.encode('utf-8')
    staging = Path(tempfile.mkdtemp(prefix='.sequoia-stage-', dir=profile))
    try:
        shutil.copytree(ROOT / 'chrome', staging / 'chrome')
        if not state:
            backup = profile / ('.sequoia-backup-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
            backup.mkdir(mode=0o700)
            if userjs.exists():
                shutil.copy2(userjs, backup / 'user.js')
            state = {'backup': backup.name, 'had_userjs': userjs.exists()}
            if chrome.exists() or chrome.is_symlink():
                chrome.rename(backup / 'chrome')  # Preserve a previous theme symlink, not its target.
        else:
            chrome.rename(staging / 'previous-chrome')
        try:
            (staging / 'chrome').rename(chrome)
            atomic_write(userjs, updated)
            state['files'] = fingerprints(chrome)
            state['userjs_hash'] = hashlib.sha256(updated).hexdigest()
            atomic_write(receipt, (json.dumps(state, indent=2) + '\n').encode())
        except Exception:
            if chrome.exists():
                shutil.rmtree(chrome)
            saved = staging / 'previous-chrome'
            if not saved.exists():
                saved = profile / state['backup'] / 'chrome'
            if saved.exists() or saved.is_symlink():
                saved.rename(chrome)
            if previous or state['had_userjs']:
                atomic_write(userjs, previous)
            else:
                userjs.unlink(missing_ok=True)
            raise
    finally:
        shutil.rmtree(staging)
    print('Installed. Restart Firefox to load all surfaces. Backup: ' + state['backup'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', type=Path, help='Firefox profile directory')
    parser.add_argument('--list', action='store_true', help='List discovered profiles')
    parser.add_argument('--restore', action='store_true', help='Restore the previous CSS theme')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    profiles = discover()
    if args.list:
        for path, default in profiles:
            print(str(path) + (' [install default]' if default else ''))
        return
    profile = args.profile
    if profile is None:
        candidates = [p for p, default in profiles if default] or [p for p, _ in profiles]
        if len(candidates) != 1:
            parser.error('Choose a profile with --profile PATH; use --list to see profiles.')
        profile = candidates[0]
    try:
        install(profile, args.restore, args.dry_run)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f'Error: {error}\n')


if __name__ == '__main__':
    main()
