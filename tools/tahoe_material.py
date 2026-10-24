"""Build the Material Symbols Rounded layer without changing Tahoe app art."""
import hashlib
import json
from pathlib import Path
import shutil
import os
import xml.etree.ElementTree as ET
from functools import lru_cache
from tahoe_catalog import extend
from tahoe_states import register as register_states, geometry as state_geometry

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'themes/icons/material-rounded'
if not SOURCE.exists(): SOURCE = Path(__file__).resolve().parent / 'material-rounded'
REVISION = '10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7'
OPTICAL_SCALES = json.loads((SOURCE/'optical-sizes.json').read_text()) if (SOURCE/'optical-sizes.json').exists() else {}
# Preserve semantic mappings while giving box-shaped controls Sequoia corners.
SOFT_CONTROLS = {'content_paste':'clipboard','content_copy':'copy','file_copy':'filecopy',
                 'note_add':'newdocument','description':'document','save':'save','crop_square':'window'}
# Power management requests this explicit symbolic name. Supply real small-size
# controls so Qt does not fall through to an old Apps-context symbolic variant.
REQUIRED_SYMBOLIC_CONTROLS = {'speedometer'}
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
extend(GROUPS)


def symbols():
    return sorted({symbol for group in GROUPS.values() for symbol in group})


def source_variant(symbol):
    if symbol.endswith('_filled'): return symbol.removesuffix('_filled'), '_fill1'
    if symbol == 'flip_vertical': return 'flip', ''
    return symbol, ''


def outline_asset(path):
    """Identify small monochrome artwork, including semantic warning colors."""
    try: root = ET.parse(path).getroot()
    except (OSError, ET.ParseError): return False
    if any(e.tag.rsplit('}',1)[-1] in {'image','linearGradient','radialGradient','filter','mask'} for e in root.iter()): return False
    paints = set()
    def walk(element, inherited):
        tag = element.tag.rsplit('}',1)[-1]
        if tag in {'defs','metadata','namedview'}: return
        props = dict(inherited); props.update(element.attrib)
        props.update(x.strip().split(':',1) for x in element.get('style','').split(';') if ':' in x)
        if tag in {'path','rect','circle','ellipse','polygon','polyline','line'}:
            paints.update(props.get(k,'none') for k in ('fill','stroke'))
        for child in element: walk(child,props)
    walk(root,{'fill':'black','stroke':'none'})
    paints -= {'none','transparent'}
    neutrals = {'currentColor','black','white','#000','#fff','#000000','#ffffff','#363636','#f2f2f2','#232629','#eff0f1','#2e3436','#dedede','#dfdfdf'}
    return bool(paints) and (paints <= neutrals or len(paints) == 1)


def symbolic_name(name):
    if '-symbolic' in name: return name
    return name[:-4]+'-symbolic-rtl' if name.endswith('-rtl') else name+'-symbolic'


@lru_cache(maxsize=None)
def svg(symbol, size, foreground, role='Text'):
    optical = 20 if size <= 22 else 24
    authored = symbol.startswith('sequoia_') or symbol in SOFT_CONTROLS
    if authored:
        paths = state_geometry('sequoia_soft_'+SOFT_CONTROLS[symbol] if symbol in SOFT_CONTROLS else symbol)
        viewbox, cx, cy = '0 0 24 24', 12, 12
    else:
        root = ET.parse(SOURCE / f'{symbol}-{optical}.svg').getroot()
        paths = ''.join(f'<path d="{p.attrib["d"]}"/>' for p in root if p.tag.endswith('path'))
        viewbox = root.get('viewBox', f'0 0 {optical} {optical}')
        x,y,w,h = map(float, viewbox.split())
        cx,cy = x+w/2,y+h/2
        if symbol == 'flip_vertical': paths = f'<g transform="rotate(90 {cx} {cy})">'+paths+'</g>'
        if symbol == 'speed':
            # Retain the gauge silhouette, increasing contour weight by 25%.
            paths = f'<g stroke="currentColor" stroke-width="{w/48}" stroke-linejoin="round" stroke-linecap="round">'+paths+'</g>'
    # Some upstream glyphs reach the edge of a font's em square. Give these
    # standalone toolbar SVGs a pixel of breathing room at their actual size.
    padded = symbol in {'wifi', 'wifi_off', 'signal_wifi_0_bar', 'network_wifi_1_bar'}
    padded |= size == 16 and symbol in {'cloud_download', 'folder_open', 'light_mode', 'lock', 'visibility', 'visibility_off'}
    scale = OPTICAL_SCALES.get(symbol,{}).get(str(size), (size-2)/size if padded else 1)
    transform = f' transform="translate({cx} {cy}) scale({scale:.8f}) translate({-cx} {-cy})"' if scale != 1 else ''
    credit = 'Sequoia rounded UI and device state artwork, Apache-2.0.' if authored else 'Google Material Symbols Rounded, Apache-2.0. Sequoia adaptation: theme color and icon-name mapping.'
    if symbol == 'speed': credit += ' Contour weight increased for power management.'
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="{viewbox}">\n'
            f'<!-- {credit} -->\n'
            '<defs><style id="current-color-scheme" type="text/css">'
            f'.ColorScheme-Text {{ color:{foreground}; }}'
            '.ColorScheme-NegativeText { color:#da4453; }.ColorScheme-NeutralText { color:#f67400; }'
            '.ColorScheme-PositiveText { color:#27ae60; }</style></defs>\n'
            f'<g class="ColorScheme-{role}" fill="currentColor"{transform}>{paths}</g>\n</svg>\n')


def role_for(name):
    if name in {'state-error','firewall-applet-error','plasmavault_error'}: return 'NegativeText'
    if name in {'weather-severe-alert','state-warning','firewall-applet-panic'}: return 'NeutralText'
    if name == 'state-ok': return 'PositiveText'
    if name in {'dialog-error','data-error','am-dialog-error','script-error','printer-error','package-broken','network-vpn-error','app-safety-unsafe','security-low','emblem-urgent','vcs-conflicting','audio-volume-high-danger','critical-notif','alt-critical-notif','battery-critical','battery-empty'}:
        return 'NegativeText'
    if name in {'dialog-warning','data-warning','am-dialog-warning','printer-warning','warning-outline','messagebox_warning','emblem-warning','audio-volume-high-warning','battery-caution','battery-low','software-updates-important'}:
        return 'NeutralText'
    if name in {'data-success','emblem-ok','vcs-normal','security-high','app-verified','community-approved'}:
        return 'PositiveText'
    return 'Text'


def application_identities(theme, names):
    """Bare app/place identities keep Tahoe artwork; controls use -symbolic.

    Icon lookup does not know whether a caller is a launcher or toolbar. The
    presence of an Apps context alone is not sufficient: upstream also stores
    action aliases there. Keep explicit action namespaces as controls.
    """
    controls = ('edit-', 'document-', 'dialog-', 'help-', 'media-', 'view-')
    result = {}
    for name in names:
        if name.startswith(controls) and name != 'view-app-grid': continue
        source = theme/'apps/scalable'/f'{name}.svg'
        if name in {'user-trash','user-trash-full'}: source = theme/'places/scalable'/f'{name}.svg'
        if source.is_file(): result[name] = source
    return result


def preserve_application_identities(theme, identities):
    """Route pre-existing small bare-name aliases to the full-color identity."""
    for context in ('actions','status','devices','categories','places','emblems','preferences','apps'):
        for size in ('16','22','24','32','48','64'):
            directory=theme/context/size
            if not directory.is_dir() or directory.is_symlink(): continue
            for name,source in identities.items():
                path=directory/(name+'.svg')
                if not path.exists() or path.resolve()==source.resolve(): continue
                path.unlink()
                path.symlink_to(os.path.relpath(source.resolve(),directory))


def apply(output):
    register_states(GROUPS, output)
    source_manifest = json.loads((SOURCE / 'sources.json').read_text())
    for name, digest in source_manifest['sha256'].items():
        if hashlib.sha256((SOURCE / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError(f'Material source changed: {name}')
    generated = {}
    protected = {}
    all_names = {name: symbol for group in GROUPS.values() for symbol, names in group.items() for name in names.split()}
    for suffix in ('', '-light', '-dark'):
        theme = output / ('SequoiaTahoe' + suffix)
        identities = application_identities(theme, all_names)
        preserve_application_identities(theme, identities)
        protected[theme.name] = {name:str(source.resolve().relative_to(output.resolve())) for name,source in identities.items()}
        for context, group in GROUPS.items():
            if context == 'apps': continue  # App artwork is never a control overlay target.
            for folder in ('16', '22', '24', '32', 'symbolic'):
                directory = theme / context / folder
                # Shared family directories already resolve to the correct base
                # palette; never break those directories or follow file aliases.
                if not directory.is_dir() or directory.is_symlink():
                    continue
                if context in {'apps','mimes'} and folder != 'symbolic': continue
                size = 16 if folder == 'symbolic' else int(folder)
                light = suffix == '-light' or (suffix == '' and (context != 'status' or folder == 'symbolic'))
                color = '#363636' if light else '#f2f2f2'
                own_names = {name for names in group.values() for name in names.split()}
                if context == 'actions': own_names |= REQUIRED_SYMBOLIC_CONTROLS
                if context in {'categories','places','emblems','preferences','apps','mimes'}: own_names = set()
                for name, symbol in all_names.items():
                    # Qt searches multiple contexts; update existing duplicate
                    # names too, so an older actions icon cannot shadow status.
                    if name in own_names or (directory / (name + '.svg')).exists() or (directory / (name + '-symbolic.svg')).exists():
                        targets = [symbolic_name(name)] if folder == 'symbolic' or name in identities else list(dict.fromkeys([name, symbolic_name(name)]))
                        for target in targets:
                            path = directory / (target + '.svg')
                            # Also supply standard names absent from the upstream
                            # size directory, so Qt won't fall back to old artwork.
                            if target.endswith('-symbolic') and folder != 'symbolic' and not path.exists() and name not in identities and name not in REQUIRED_SYMBOLIC_CONTROLS:
                                continue
                            if context not in {'actions','status','devices'} and (not path.exists() or not outline_asset(path)):
                                continue
                            if path.is_symlink(): path.unlink()
                            role = role_for(name.replace('-symbolic',''))
                            data = svg(symbol, size, color, role)
                            path.write_text(data)
                            generated[str(path.relative_to(output))] = {'symbol': symbol, 'role':role, 'sha256': hashlib.sha256(data.encode()).hexdigest()}
        shutil.copyfile(SOURCE / 'LICENSE', theme / 'COPYING.MaterialSymbols')
        (theme / 'MATERIAL-SYMBOLS.txt').write_text(
            'Material Symbols Rounded by Google, Apache License 2.0.\n'
            f'Upstream: https://github.com/google/material-design-icons/tree/{REVISION}\n'
            'Modified for Sequoia Tahoe: palette-aware SVGs and KDE/freedesktop aliases.\n'
            'Sequoia authors: original rounded controls, peripheral and connection state artwork, Apache-2.0.\n'
            'Only the files listed in material-icons.json use this license; existing Tahoe art retains its own license.\n'
            'Sources and reproducible mapping: themes/icons/material-rounded and tools/tahoe_material.py.\n')
        own = {str(Path(path).relative_to(theme.name)): info for path, info in generated.items() if Path(path).parts[0] == theme.name}
        (theme / 'material-icons.json').write_text(json.dumps(own, indent=2) + '\n')
        notes = theme / 'MODIFICATIONS.md'
        original = notes.read_text().split('\n## Rounded actions trial')[0].split('\n## Rounded controls')[0]
        notes.write_text(original + '\n## Rounded controls\n\n'
            '2026-10-24: UI controls and tray indicators use Google Material\n'
            'Symbols Rounded, weight 400, with 20/24 optical sizes. Selected states use filled variants.\n'
            'Adaptations add semantic icon names, optical insets and KDE palette recoloring.\n'
            'Original Sequoia battery, Wi-Fi and microphone families preserve device states.\n'
            'Peripheral batteries and cellular/connection/trust indicators preserve their states.\n'
            'Bare app identities retain Tahoe art; colliding controls use symbolic names.\n'
            'See material-icons.json, MATERIAL-SYMBOLS.txt and COPYING.MaterialSymbols.\n')
    manifest = {'revision': REVISION, 'style': 'Material Symbols Rounded / weight 400 / optical 20 and 24 / selected filled states and original Sequoia device states',
                'glyphs': len(symbols()), 'protected_identities':protected, 'generated': generated}
    (output / 'material-icons.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Material Rounded: {len(symbols())} glyphs, {len(generated)} palette-aware SVGs')


if __name__ == '__main__':
    apply(SOURCE.parent)
