"""Build the Material Symbols Rounded trial layer without changing Tahoe app art."""
import hashlib
import json
from pathlib import Path
import shutil
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'themes/icons/material-rounded'
if not SOURCE.exists(): SOURCE = Path(__file__).resolve().parent / 'material-rounded'
REVISION = '10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7'
# Explicit semantic mappings; other icons and their aliases remain upstream.
GROUPS = {
    'actions': {
        'search': 'edit-find search system-search',
        'close': 'window-close dialog-close edit-clear-locationbar-ltr edit-clear-locationbar-rtl',
        'add': 'list-add edit-add',
        'remove': 'list-remove',
        'zoom_in': 'zoom-in',
        'zoom_out': 'zoom-out',
        'check': 'dialog-ok dialog-ok-apply dialog-apply',
        'cancel': 'dialog-cancel process-stop',
        'arrow_back': 'go-previous go-left',
        'arrow_forward': 'go-next go-right',
        'keyboard_arrow_down': 'go-down pan-down',
        'keyboard_arrow_up': 'go-up pan-up',
        'first_page': 'go-first',
        'last_page': 'go-last',
        'refresh': 'view-refresh reload',
        'settings': 'configure settings-configure',
        'tune': 'view-filter configure-toolbars',
        'menu': 'application-menu open-menu',
        'more_horiz': 'overflow-menu',
        'content_copy': 'edit-copy',
        'content_cut': 'edit-cut',
        'content_paste': 'edit-paste',
        'undo': 'edit-undo',
        'redo': 'edit-redo',
        'delete': 'edit-delete user-trash',
        'edit': 'document-edit edit-rename',
        'note_add': 'document-new',
        'folder_open': 'document-open document-open-folder',
        'save': 'document-save',
        'save_as': 'document-save-as',
        'print': 'document-print',
        'download': 'edit-download download',
        'upload': 'upload',
        'share': 'document-share',
        'info': 'help-about help-hint',
        'help': 'help-contextual help-contents',
        'home': 'go-home',
        'fullscreen': 'view-fullscreen',
        'fullscreen_exit': 'view-restore',
        'play_arrow': 'media-playback-start',
        'pause': 'media-playback-pause',
        'stop': 'media-playback-stop',
        'skip_next': 'media-skip-forward',
        'skip_previous': 'media-skip-backward',
        'fast_forward': 'media-seek-forward',
        'fast_rewind': 'media-seek-backward',
        'shuffle': 'media-playlist-shuffle',
        'repeat': 'media-playlist-repeat media-repeat-all',
        'repeat_one': 'media-repeat-single media-repeat-track',
        'visibility': 'view-visible password-show-on',
        'visibility_off': 'view-hidden password-show-off',
        'calendar_month': 'view-calendar',
    },
    'status': {
        'notifications': 'notifications notification',
        'notifications_off': 'notifications-disabled notification-disabled',
        'light_mode': 'brightness-high display-brightness',
        'brightness_4': 'brightness-low',
        'content_paste': 'klipper clipboard',
        'volume_up': 'audio-volume-high audio-volume-high-panel volume-level-high volume-level-high-panel',
        'volume_down': 'audio-volume-medium audio-volume-medium-panel volume-level-medium volume-level-medium-panel',
        'volume_mute': 'audio-volume-low audio-volume-low-panel audio-volume-zero audio-volume-zero-panel volume-level-low volume-level-low-panel volume-level-none volume-level-none-panel',
        'volume_off': 'audio-volume-muted audio-volume-muted-panel volume-level-muted volume-level-muted-panel',
        'mic_off': 'microphone-sensitivity-muted',
        'bluetooth': 'bluetooth-active',
        'bluetooth_connected': 'bluetooth-connected',
        'bluetooth_disabled': 'bluetooth-disabled',
        'wifi': 'network-wireless-100 network-wireless-connected-100',
        'wifi_2_bar': 'network-wireless-75 network-wireless-connected-75',
        'network_wifi_1_bar': 'network-wireless-25 network-wireless-connected-25',
        'wifi_1_bar': 'network-wireless-50 network-wireless-connected-50',
        'signal_wifi_0_bar': 'network-wireless-00 network-wireless-connected-00',
        'wifi_off': 'network-wireless-disconnected network-wireless-off',
        'cloud_download': 'update-low',
        'lock': 'network-vpn network-vpn-active',
    },
    'devices': {
        'bluetooth': 'bluetooth network-bluetooth',
        'mic': 'audio-input-microphone microphone',
        'headphones': 'audio-headphones',
        'speaker': 'audio-speakers',
        'desktop_windows': 'video-display',
        'keyboard': 'input-keyboard',
        'mouse': 'input-mouse',
        'wifi': 'network-wireless',
        'lan': 'network-wired',
        'usb': 'drive-removable-media-usb',
    },
}


def symbols():
    return sorted({symbol for group in GROUPS.values() for symbol in group})


def svg(symbol, size, foreground):
    optical = 20 if size <= 22 else 24
    root = ET.parse(SOURCE / f'{symbol}-{optical}.svg').getroot()
    paths = ''.join(f'<path d="{p.attrib["d"]}"/>' for p in root if p.tag.endswith('path'))
    # Some upstream glyphs reach the edge of a font's em square. Give these
    # standalone toolbar SVGs a pixel of breathing room at their actual size.
    padded = symbol in {'wifi', 'wifi_off', 'signal_wifi_0_bar', 'network_wifi_1_bar'}
    padded |= size == 16 and symbol in {'cloud_download', 'folder_open', 'light_mode', 'lock', 'visibility', 'visibility_off'}
    transform = f' transform="translate(480 -480) scale({(size-2)/size:.8f}) translate(-480 480)"' if padded else ''
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="{root.attrib["viewBox"]}">\n'
            '<!-- Google Material Symbols Rounded, Apache-2.0. Sequoia adaptation: theme color and icon-name mapping. -->\n'
            '<defs><style id="current-color-scheme" type="text/css">'
            f'.ColorScheme-Text {{ color:{foreground}; }}</style></defs>\n'
            f'<g class="ColorScheme-Text" fill="currentColor"{transform}>{paths}</g>\n</svg>\n')


def apply(output):
    source_manifest = json.loads((SOURCE / 'sources.json').read_text())
    for name, digest in source_manifest['sha256'].items():
        if hashlib.sha256((SOURCE / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError(f'Material source changed: {name}')
    generated = {}
    all_names = {name: symbol for group in GROUPS.values() for symbol, names in group.items() for name in names.split()}
    for suffix in ('', '-light', '-dark'):
        theme = output / ('SequoiaTahoe' + suffix)
        for context, group in GROUPS.items():
            for folder in ('16', '22', '24', '32', 'symbolic'):
                directory = theme / context / folder
                # Shared family directories already resolve to the correct base
                # palette; never break those directories or follow file aliases.
                if not directory.is_dir() or directory.is_symlink():
                    continue
                size = 16 if folder == 'symbolic' else int(folder)
                light = suffix == '-light' or (suffix == '' and (context != 'status' or folder == 'symbolic'))
                color = '#363636' if light else '#f2f2f2'
                own_names = {name for names in group.values() for name in names.split()}
                if context == 'actions':
                    # A scalable application icon otherwise wins before Devices
                    # in Qt's context order (e.g. Bluetooth's blue app badge).
                    # Supply a small control alias; keep the app SVG untouched.
                    own_names |= {name for name in all_names if (theme / 'apps/scalable' / (name + '.svg')).exists()}
                for name, symbol in all_names.items():
                    # Qt searches multiple contexts; update existing duplicate
                    # names too, so an older actions icon cannot shadow status.
                    if name in own_names or (directory / (name + '.svg')).exists() or (directory / (name + '-symbolic.svg')).exists():
                        targets = [name + '-symbolic'] if folder == 'symbolic' else [name, name + '-symbolic']
                        for target in targets:
                            path = directory / (target + '.svg')
                            # Also supply standard names absent from the upstream
                            # size directory, so Qt won't fall back to old artwork.
                            if target.endswith('-symbolic') and folder != 'symbolic' and not path.exists():
                                continue
                            if path.is_symlink(): path.unlink()
                            data = svg(symbol, size, color)
                            path.write_text(data)
                            generated[str(path.relative_to(output))] = {'symbol': symbol, 'sha256': hashlib.sha256(data.encode()).hexdigest()}
        shutil.copyfile(SOURCE / 'LICENSE', theme / 'COPYING.MaterialSymbols')
        (theme / 'MATERIAL-SYMBOLS.txt').write_text(
            'Material Symbols Rounded by Google, Apache License 2.0.\n'
            f'Upstream: https://github.com/google/material-design-icons/tree/{REVISION}\n'
            'Modified for Sequoia Tahoe: palette-aware SVGs and KDE/freedesktop aliases.\n'
            'Only the files listed in material-icons.json use this license; existing Tahoe art retains its own license.\n'
            'Sources and reproducible mapping: themes/icons/material-rounded and tools/tahoe_material.py.\n')
        own = {str(Path(path).relative_to(theme.name)): info for path, info in generated.items() if Path(path).parts[0] == theme.name}
        (theme / 'material-icons.json').write_text(json.dumps(own, indent=2) + '\n')
        notes = theme / 'MODIFICATIONS.md'
        original = notes.read_text().split('\n## Rounded actions trial')[0]
        notes.write_text(original + '\n## Rounded actions trial\n\n'
            '2026-10-24: selected UI controls and tray indicators use Google Material\n'
            'Symbols Rounded, weight 400, unfilled, with 20/24 optical sizes.\n'
            'Adaptations add semantic icon names and KDE palette recoloring.\n'
            'See material-icons.json, MATERIAL-SYMBOLS.txt and COPYING.MaterialSymbols.\n')
    manifest = {'revision': REVISION, 'style': 'Material Symbols Rounded / FILL 0 / weight 400 / optical 20 and 24',
                'glyphs': len(symbols()), 'generated': generated}
    (output / 'material-icons.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Material Rounded: {len(symbols())} glyphs, {len(generated)} palette-aware SVGs')


if __name__ == '__main__':
    apply(SOURCE.parent)
