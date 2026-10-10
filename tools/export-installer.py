#!/usr/bin/env python3
"""Create one portable installer archive, excluding builds, SDKs and local state."""
import argparse
import hashlib
import io
from pathlib import Path
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NAME = 'Sequoia-Eleven'
# Include sources for every option exposed by the GUI. Native plugins are built
# on the recipient's machine; development SDKs and prior builds are not portable.
PAYLOAD = (
    'install-gui.py', 'Install Sequoia Eleven.desktop', 'install.sh',
    'uninstall-gui.py', 'Uninstall Sequoia Eleven.desktop',
    'README.md', 'LICENSE', 'application-style', 'color-schemes', 'desktop',
    'fonts', 'icons', 'lockscreen', 'login-screen', 'plasma', 'tools', 'wallpapers',
    'apps/settings', 'apps/gwenview',
)
EXCLUDED = {'.git', '.staging', '__pycache__', 'CMakeFiles', 'CMakeCache.txt'}
INTRO = '''SEQUOIA ELEVEN — INSTALLER

1. Extract this entire archive to a writable folder on a KDE Plasma 6 computer.
2. Open the extracted Sequoia-Eleven folder.
3. Run: python3 install-gui.py
   Or open "Install Sequoia Eleven.desktop" (KDE may ask you to trust the launcher).
4. Choose your distribution and components. The installer downloads missing
   dependencies and builds native theme components for the destination system.
   Choose Desktop widgets for widgets alone, or individual styles and artwork.
   Global Theme automatically includes the components referenced by its presets.
5. In System Settings > Colors & Themes > Global Theme, select Sequoia Eleven.
   Enable Desktop layout to recreate the included top bar and bottom dock.
   Leave it unchecked to preserve existing panels and desktop widgets.

Do not run the installer itself as root. It requests administrator permission
when needed. Keep the extracted folder to open Uninstall Sequoia Eleven.desktop later.
You can also run python3 uninstall-gui.py to choose components to remove.

The custom System Tray, Trash, animated task manager and six layout widgets are included. Native widgets are built for the destination machine. Pinned applications must be
installed separately. Downloads uses your own Downloads folder. The theme uses
its bundled wallpaper, not a personal wallpaper from the original machine.

This archive includes the theme, layout, installer and sources for the optional
Settings/Gwenview makeovers. It excludes build caches, SDKs, screenshots, backups,
Falkon/Firefox projects.

The installer supports Arch, Fedora, Ubuntu and Debian when their releases
provide the required Plasma 6 development packages. Tested with Fedora 44,
Plasma 6.7.5 and Qt 6.11.2. Optional Settings/Gwenview makeovers require the
matching application versions shown by the installer. The login theme requires
Plasma Login Manager; it does not support SDDM.
Both the GUI and default command-line installation omit the login screen when
Plasma Login Manager is unavailable. Other components can still be installed.

See README.md and plasma/layouts/README.md for details.
'''


def include(member):
    relative = Path(member.name).relative_to(NAME)
    if any(part in EXCLUDED for part in relative.parts) or relative.suffix in ('.pyc', '.pyo'):
        return None
    if member.issym():
        source = ROOT / relative
        target = source.resolve(strict=True)
        if not any(target == ROOT / entry or target.is_relative_to(ROOT / entry) for entry in PAYLOAD):
            raise RuntimeError(f'Link points outside the installer payload: {relative}')
        if Path(member.linkname).is_absolute():
            raise RuntimeError(f'Nonportable absolute symlink: {relative}')
    member.uid = member.gid = 0
    member.uname = member.gname = ''
    member.pax_headers = {}
    return member


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist/Sequoia-Eleven-Installer.tar.xz')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.name.endswith('.tar.xz'):
        parser.error('Output must end in .tar.xz')
    if any(output == ROOT / entry or output.is_relative_to(ROOT / entry) for entry in PAYLOAD):
        parser.error('Output must be outside the input files/directories')
    output.parent.mkdir(parents=True, exist_ok=True)
    for entry in PAYLOAD:
        if not (ROOT / entry).exists():
            raise RuntimeError(f'Missing installer source: {entry}')
    with tempfile.TemporaryDirectory(prefix='.installer-export-', dir=output.parent) as temporary:
        incoming = Path(temporary) / output.name
        with tarfile.open(incoming, 'w:xz', preset=6, dereference=False) as archive:
            info = tarfile.TarInfo(NAME + '/START-HERE.txt')
            data = INTRO.encode()
            info.size = len(data)
            info.mode = 0o644
            archive.addfile(info, io.BytesIO(data))
            for entry in PAYLOAD:
                archive.add(ROOT / entry, arcname=NAME + '/' + entry, filter=include)
        with tarfile.open(incoming, 'r:xz') as archive:
            members = archive.getmembers()
            count = sum(m.isfile() or m.issym() for m in members)
            unpacked = sum(m.size for m in members)
        incoming.replace(output)
    digest = hashlib.file_digest(output.open('rb'), 'sha256').hexdigest()
    output.with_name(output.name + '.sha256').write_text(digest + '  ' + output.name + '\n')
    print(f'Created {output}')
    print(f'{output.stat().st_size / 1024**2:.1f} MiB compressed; {unpacked / 1024**2:.1f} MiB file contents; '
          f'{count:,} files/links in one archive')


if __name__ == '__main__':
    main()
