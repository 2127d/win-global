"""Prepare version-checked, per-user bindings for the approved motion palette."""
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
EFFECTS = ('scale', 'fadingpopups', 'maximize', 'fullscreen', 'login', 'logout')
FACTOR = 2 / 3  # KWin Overview's native 300 ms becomes the approved 200 ms.


def replace(text, old, new, count=1):
    if text.count(old) != count:
        raise RuntimeError(f'Upstream animation changed; expected {count} occurrence(s) of {old!r}')
    return text.replace(old, new)


def duration(ms):
    # Compensate for the workspace baseline; later user speed/reduced-motion
    # changes still scale every compositor effect normally.
    return f'animationTime({round(ms / FACTOR)})'


def kwin_effect(name, text):
    if name == 'scale':
        text = replace(text, 'this.duration = animationTime(duration);', 'this.duration = ' + duration(240) + ';')
        text = replace(text, 'this.inScale = effect.readConfig("InScale", 0.8);', 'this.inScale = 0.93;')
        text = replace(text, 'this.outScale = effect.readConfig("OutScale", 0.8);', 'this.outScale = 0.93;')
        text = replace(text, 'curve: QEasingCurve.InCubic,\n            duration: this.duration,',
                       'curve: QEasingCurve.InCubic,\n            duration: ' + duration(168) + ',')
    elif name == 'fadingpopups':
        text = replace(text, 'animationTime(150) * 4', duration(112))
        text = replace(text, 'animationTime(150)', duration(160))
        text = replace(text, 'QEasingCurve.Linear', 'QEasingCurve.OutCubic')
        text = replace(text, 'QEasingCurve.OutQuart', 'QEasingCurve.InCubic')
        text = replace(text, 'window.splash || window.toolbar || window.onScreenDisplay || window.appletPopup',
            'window.splash || window.toolbar || window.onScreenDisplay || window.appletPopup\n'
            '            || window.notification || window.criticalNotification\n'
            '            || (window.dialog && !window.hasDecoration && window.windowClass.includes("plasmashell"))')
        text = replace(text, 'type: Effect.Opacity,\n            from: 0.0,\n            to: 1.0',
            'animations: [\n'
            '                { type: Effect.Opacity, from: 0.0, to: 1.0 },\n'
            '                { type: Effect.Translation, from: {value1: 0, value2: -8}, to: {value1: 0, value2: 0} }\n'
            '            ]')
        text = replace(text, 'type: Effect.Opacity,\n            from: 1.0,\n            to: 0.0',
            'animations: [\n'
            '                { type: Effect.Opacity, from: 1.0, to: 0.0 },\n'
            '                { type: Effect.Translation, from: {value1: 0, value2: 0}, to: {value1: 0, value2: -8} }\n'
            '            ]')
    elif name in ('maximize', 'fullscreen'):
        text = replace(text, 'animationTime(250)', duration(240))
    elif name == 'login':
        text = replace(text, 'animationTime(500)', duration(200))
    elif name == 'logout':
        text = replace(text, 'animationTime(800)', duration(200))
        text = replace(text, 'animationTime(400)', duration(200))
    return text + f'\nprint("Sequoia motion loaded: {name}");\n'


def dock_task(text):
    text = replace(text, 'Math.round(Kirigami.Units.longDuration * 100 / Math.max(50, Plasmoid.configuration.dockSpeed))',
        'Math.round(370 * 100 / Math.max(50, Plasmoid.configuration.dockSpeed))')
    text = replace(text, 'loops: Animation.Infinite', 'loops: 1')
    text = replace(text, 'duration: task.sequoiaDuration * 2', 'duration: task.sequoiaMotion ? 225 : 0', 2)
    return text


def qml_timings(text, name):
    if name == 'Menu.qml':
        # Preserve the native menu's input/positioning; only its transition changes.
        for marker, milliseconds, curve in [('enter: Transition', 160, 'OutCubic'), ('exit: Transition', 112, 'InCubic')]:
            start = text.index(marker)
            end = text.index('\n    }', start)
            block = text[start:end]
            block = re.sub(r'duration: Kirigami.Units.\w+Duration',
                f'duration: Kirigami.Units.longDuration > 0 ? {milliseconds} : 0', block)
            block = re.sub(r'easing.type: Easing.\w+', 'easing.type: Easing.' + curve, block)
            text = text[:start] + block + text[end:]
    elif name not in ('BusyIndicator.qml', 'ProgressBar.qml'):
        text = re.sub(r'(\b(?:duration|highlightMoveDuration): )Kirigami.Units.(?:veryShort|short|long|veryLong)Duration\b',
            r'\1(Kirigami.Units.longDuration > 0 ? 200 : 0)', text)
        if name == 'StackView.qml':
            text = text.replace('Easing.OutCubic', 'Easing.InOutCubic')
    return text


def prepare(stage, data, config):
    if stage.exists():
        raise RuntimeError('Use a new staging directory.')
    stage.mkdir(parents=True)
    changes = []
    def save(relative, contents):
        target = stage / 'data' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(contents)
    def package(source, relative):
        if not source.is_dir():
            raise RuntimeError(f'Missing installed component: {source}')
        target = stage / 'data' / relative
        shutil.copytree(source, target)
        return target
    # Dedicated IDs prevent ambiguity with distro packages and make rollback clear.
    upstream = Path('/usr/share/kwin-wayland/effects')
    for effect in EFFECTS:
        target = package(upstream / effect, f'kwin-wayland/effects/sequoia-motion-{effect}')
        meta = json.loads((target / 'metadata.json').read_text())
        plugin = meta['KPlugin']
        for key in list(plugin):
            if key.startswith(('Name[', 'Description[')):
                del plugin[key]
        plugin.update(Id='sequoia-motion-' + effect, Name='Sequoia Motion: ' + plugin['Name'], EnabledByDefault=False)
        meta.pop('X-KDE-ConfigModule', None)
        (target / 'metadata.json').write_text(json.dumps(meta, indent=2) + '\n')
        script = target / 'contents/code/main.js'
        script.write_text(kwin_effect(effect, script.read_text()))
    changes.append('KWin popups 160/112 ms, windows 240/168 ms, maximize/fullscreen 240 ms, session fades 200 ms')
    # Override the active native fork's QML without replacing its native adapter.
    dock_id = 'org.sequoia.icontasks.animated'
    dock_source = data / 'plasma/plasmoids' / dock_id
    if not dock_source.exists():
        dock_source = Path('/usr/share/plasma/plasmoids') / dock_id
    dock = package(dock_source, 'plasma/plasmoids/' + dock_id)
    task = dock / 'contents/ui/Task.qml'
    if 'Math.round(370 * 100' not in task.read_text():
        task.write_text(dock_task(task.read_text()))
    shutil.copy2(ROOT / 'desktop/control-center/taskmanager/DockMotion.qml', dock / 'contents/ui/DockMotion.qml')
    manifest = dock / 'sequoia-package.json'
    if manifest.exists():
        payload = json.loads(manifest.read_text())
        payload['files'] = {str(p.relative_to(dock)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in dock.rglob('*') if p.is_file() and p != manifest}
        manifest.write_text(json.dumps(payload, indent=2) + '\n')
    changes.append('Dock hover/reorder/launch 370 ms; one launch pulse; one 450 ms attention cue')
    for name in ('contents/ui/StartMenu.qml', 'contents/config/main.xml', 'contents/ui/AppTile.qml'):
        text = (ROOT / 'widgets/plasmoids/org.sequoia.startmenu' / name).read_text()
        if name.endswith('AppTile.qml'):
            text = text.replace('duration: 100', 'duration: tile.menu.motionDuration > 0 ? 200 : 0')
        save('plasma/plasmoids/org.sequoia.startmenu/' + name, text)
    changes.append('Start menu page navigation 200 ms; app-tile control feedback 200 ms')
    # The application overlay already handles native style initialization. Refresh
    # only timing declarations, preserving all visual and layout customizations.
    quick = data / 'sequoia-eleven/quick-islands/qml/org/kde/desktop'
    if not quick.is_dir():
        raise RuntimeError('Install the existing Sequoia Quick style before applying motion.')
    for source in quick.rglob('*.qml'):
        original = source.read_text()
        updated = qml_timings(original, source.name)
        if updated != original:
            save(str(source.relative_to(data)), updated)
    # Plasma has a different controls module; a shell-only import overlay keeps
    # its controls independent of the faster compositor workspace baseline.
    from PySide6.QtCore import QLibraryInfo
    imports = Path(QLibraryInfo.path(QLibraryInfo.QmlImportsPath))
    plasma_source = imports / 'org/kde/plasma/components'
    plasma = package(plasma_source, 'sequoia-eleven/motion/qml/org/kde/plasma/components')
    for target in plasma.rglob('*'):
        if not target.is_file():
            continue
        if target.suffix == '.qml':
            target.write_text(qml_timings(target.read_text(), target.name))
        elif target.name == 'qmldir':
            target.write_text(re.sub(r'^prefer .*\n', '', target.read_text(), flags=re.M))
        elif '.so' in target.name:
            relative = target.relative_to(plasma)
            target.unlink()
            target.symlink_to(plasma_source / relative)
    changes.append('Plasma and styled Qt Quick controls 200 ms, menus 160/112 ms, navigation 200 ms')
    lock = data / 'plasma/shells/org.kde.plasma.desktop/contents/lockscreen'
    for name in ('SequoiaSession.qml', 'SequoiaWallpaperFader.qml', 'LockScreenUi.qml'):
        source = lock / name
        if not source.is_file():
            continue
        text = source.read_text()
        if 'import org.kde.kirigami as Kirigami' not in text:
            text = text.replace('import QtQuick\n', 'import QtQuick\nimport org.kde.kirigami as Kirigami\n', 1)
        if name == 'SequoiaSession.qml':
            text = text.replace('duration: 120', 'duration: Kirigami.Units.longDuration > 0 ? 225 : 0')
        elif name == 'SequoiaWallpaperFader.qml':
            text = text.replace('duration: 250', 'duration: Kirigami.Units.longDuration > 0 ? 200 : 0')
        else:
            text = text.replace('duration: Kirigami.Units.veryLongDuration * 2', 'duration: Kirigami.Units.longDuration > 0 ? 200 : 0')
        save(str(source.relative_to(data)), text)
    settings = [
        ['kdeglobals', 'KDE', 'AnimationDurationFactor', str(FACTOR)],
        ['sequoiaelevenrc', 'Style', 'AnimationsEnabled', 'true'],
        ['sequoiaelevenrc', 'Style', 'AnimationsDuration', '200'],
        ['sequoiaelevenrc', 'Windeco', 'AnimationsEnabled', 'true'],
        ['sequoiaelevenrc', 'Windeco', 'AnimationsDuration', '200'],
        ['kwinrc', 'Effect-magiclamp', 'AnimationDuration', str(round(240 / FACTOR))],
    ]
    for effect in EFFECTS:
        settings += [['kwinrc', 'Plugins', effect + 'Enabled', 'false'],
                     ['kwinrc', 'Plugins', 'sequoia-motion-' + effect + 'Enabled', 'true']]
    # Use the same short travel for tray popups and notification entry as menus;
    # avoid stacking the native full-slide animation on top of this effect.
    settings += [['kwinrc', 'Plugins', name + 'Enabled', 'false'] for name in ('slidingpopups', 'slidingnotifications')]
    shell_conf = stage / 'config/systemd/user/plasma-plasmashell.service.d/70-sequoia-motion.conf'
    shell_conf.parent.mkdir(parents=True)
    overlay = str(data / 'sequoia-eleven/motion/qml').replace('%', '%%').replace('"', '\\"')
    shell_conf.write_text('[Service]\nEnvironment="QML_IMPORT_PATH=' + overlay + '"\n')
    palette = json.loads((ROOT / 'tools/motion-lab/palettes.json').read_text())['presets'][3]
    save('sequoia-eleven/motion/palette.json', json.dumps(palette, indent=2) + '\n')
    plan = {'version': 1, 'data': str(data), 'config': str(config), 'settings': settings,
        'effects': list(EFFECTS), 'changes': changes,
        'limitations': ['KWin desktop slide remains spring-driven; Overview uses the 200 ms workspace baseline.',
                        'Third-party/GTK/browser internal animations and operation progress keep their own timing.'],
        'sources': {str(upstream / e / 'contents/code/main.js'): hashlib.sha256((upstream / e / 'contents/code/main.js').read_bytes()).hexdigest() for e in EFFECTS}}
    (stage / 'plan.json').write_text(json.dumps(plan, indent=2) + '\n')
    return plan
