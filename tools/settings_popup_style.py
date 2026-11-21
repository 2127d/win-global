#!/usr/bin/env python3
"""Bundle Sequoia popup controls for the Hub and every native KCM it hosts."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from quick_islands import QML, ROOT, background, replace
import quick_menus


def build(destination):
    destination = Path(destination)
    source = QML / 'org/kde/desktop'
    destination.mkdir(parents=True, exist_ok=True)
    fingerprints = {}
    for path in source.rglob('*'):
        if not path.is_file():
            continue
        target = destination / path.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.unlink(missing_ok=True)
        if '.so' in path.name:
            target.symlink_to(path)
        else:
            shutil.copy2(path, target)
        fingerprints[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        if path.name == 'qmldir':
            target.write_text(''.join(line for line in path.read_text().splitlines(True) if not line.startswith('prefer ')))
    for name, transform in [('Menu.qml', quick_menus.menu), ('MenuItem.qml', quick_menus.item), ('MenuSeparator.qml', quick_menus.separator)]:
        target = destination / name
        target.write_text(transform(target.read_text(), replace))
    shutil.copy2(ROOT / 'themes/application-style/quick/SwitchIndicator.qml', destination / 'private/SwitchIndicator.qml')
    tip = destination / 'ToolTip.qml'
    text = replace(tip.read_text(), '    padding: 6', '''    padding: 10
    horizontalPadding: Math.max(18, implicitHeight / 4)
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0''')
    text = text.replace('import QtQuick\n', 'import QtQuick\nimport org.sequoia.glass 1.0 as Glass\n', 1)
    text = background(text, '''    background: Rectangle {
        objectName: "sequoiaTooltipCapsule"
        readonly property string shape: {
            Glass.Settings.tooltipRevision;
            return Glass.Settings.tooltipShape(Glass.Settings.applicationSection);
        }
        radius: shape === "pill" ? Math.min(width, height) / 2 : shape === "rounded" ? Math.min(20, Math.min(width, height) / 2) : 0
        antialiasing: true
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Math.max(0, parent.radius - 1)
            antialiasing: true
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, Kirigami.Theme.backgroundColor.hslLightness < .5 ? .16 : .55)
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(1, 1, 1, .12) }
                GradientStop { position: .5; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, .025) }
            }
        }
    }''')
    tip.write_text(text)
    (destination / 'sequoia-popups.json').write_text(json.dumps({'sources': fingerprints}, indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, required=True)
    build(parser.parse_args().destination)
