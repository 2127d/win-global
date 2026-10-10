#!/usr/bin/env python3
"""Preview and exercise native Plasma/desktop switches with isolated accent colors."""
import argparse
import configparser
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from quick_islands import ROOT, build


def capture(output):
    from PySide6.QtCore import QObject, QTimer, QUrl
    from PySide6.QtGui import QColor
    from PySide6.QtWidgets import QApplication
    from PySide6.QtQuick import QQuickView
    app = QApplication([])
    view = QQuickView()
    view.rootContext().setContextProperty("previewMode", os.environ["SEQUOIA_SWITCH_MODE"])
    view.setSource(QUrl.fromLocalFile(str(ROOT / 'tools/preview/Switches.qml')))
    if view.status() != QQuickView.Ready:
        raise RuntimeError(view.errors())
    view.show()
    errors = []
    def save():
        picture = view.grabWindow()
        if picture.isNull() or not picture.save(str(output)):
            errors.append('Unable to capture switches')
        # A native Plasma on-track must contain the configured accent, proving
        # that KSvg loaded our artwork and resolved its live color-scheme role.
        control = next(child for child in view.rootObject().findChildren(QObject)
                       if child.objectName() == 'plasmaOn')
        point = control.mapToItem(view.rootObject(), 0, 0)
        accent = QColor(os.environ['SEQUOIA_SWITCH_ACCENT'])
        matches = sum(picture.pixelColor(x, y).rgb() == accent.rgb()
                      for x in range(round(point.x()), round(point.x()) + 42)
                      for y in range(round(point.y()), round(point.y()) + 24))
        if matches < 50:
            errors.append(f'Native Plasma track did not use the accent ({matches} matching pixels)')
        app.quit()
    QTimer.singleShot(1000, save)
    app.exec()
    if errors:
        raise RuntimeError(errors)
    print('Saved', output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.capture:
        capture(args.capture)
        return
    overlay = build(ROOT / '.staging/quick-islands')
    with tempfile.TemporaryDirectory(prefix='sequoia-switches-') as temporary:
        temporary = Path(temporary)
        for mode, accent in [('light', '#2463d4'), ('dark', '#b45fe0')]:
            config = temporary / mode
            config.mkdir()
            data = config / 'data'
            data.mkdir()
            (data / 'icons').symlink_to(ROOT / 'icons', target_is_directory=True)
            (data / 'plasma').symlink_to(ROOT / 'plasma', target_is_directory=True)
            scheme = configparser.ConfigParser(interpolation=None)
            scheme.optionxform = str
            scheme.read(ROOT / 'color-schemes' / f'SequoiaEleven{mode.title()}.colors')
            rgb = ','.join(str(int(accent[i:i+2], 16)) for i in (1, 3, 5))
            scheme['General']['AccentColor'] = rgb
            scheme['Colors:Selection']['BackgroundNormal'] = rgb
            for section in scheme.sections():
                if section.startswith('Colors:'):
                    scheme[section]['DecorationFocus'] = rgb
                    scheme[section]['DecorationHover'] = rgb
            with (config / 'kdeglobals').open('w') as stream:
                scheme.write(stream)
            (config / 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{mode}\n')
            env = dict(os.environ, XDG_CONFIG_HOME=str(config), XDG_DATA_HOME=str(data),
                       XDG_CACHE_HOME=str(config / 'cache'), QT_QPA_PLATFORM='offscreen',
                       QT_QPA_PLATFORMTHEME='kde', QT_QUICK_BACKEND='software',
                       QML_IMPORT_PATH=str(overlay), QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
                       QT_FORCE_STDERR_LOGGING='1', SEQUOIA_SWITCH_ACCENT=accent, SEQUOIA_SWITCH_MODE=mode)
            subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner', '-input',
                            str(ROOT / 'application-style/quick/tests/tst_switch.qml')], env=env, check=True, timeout=30)
            subprocess.run([sys.executable, __file__, '--capture', str(ROOT / 'preview' / f'switches-{mode}.png')],
                           env=env, check=True, timeout=20)


if __name__ == '__main__':
    main()
