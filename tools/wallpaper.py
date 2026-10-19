"""Build the paired wallpaper from the original Preview.qml backdrop geometry."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WALLPAPER_ID = 'SequoiaEleven'
PALETTES = {
    'light': ('#c7d7ee', '#e8d4c7', '#b9bee1', '#d7bdd1', '#ecc4ae'),
    'dark': ('#233453', '#473746', '#514566', '#735269', '#986451'),
}


def build_wallpaper():
    # Render the editable vector source with Qt, the same renderer used by KDE.
    from PySide6.QtCore import QByteArray, QRectF, Qt
    from PySide6.QtGui import QImage, QPainter
    from PySide6.QtSvg import QSvgRenderer

    package = ROOT / 'themes/wallpapers' / WALLPAPER_ID
    sources = package / 'contents' / 'sources'
    sources.mkdir(parents=True, exist_ok=True)
    (package / 'LICENSE').write_text((ROOT / 'LICENSE').read_text())
    for mode, (top, bottom, lavender, rose, peach) in PALETTES.items():
        svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="3840" height="2160" viewBox="0 0 1312 606" preserveAspectRatio="none">
  <title>Sequoia Eleven — {mode.title()}</title>
  <desc>Original preview backdrop, expanded to a full desktop. MIT license.</desc>
  <defs>
    <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="{top}"/>
      <stop offset="1" stop-color="{bottom}"/>
    </linearGradient>
  </defs>
  <rect width="1312" height="606" fill="url(#sky)"/>
  <rect x="800" y="-310" width="500" height="900" rx="250" fill="{lavender}" transform="rotate(-34 1050 140)"/>
  <rect x="900" y="-240" width="360" height="890" rx="180" fill="{rose}" transform="rotate(-34 1080 205)"/>
  <rect x="975" y="-210" width="230" height="890" rx="115" fill="{peach}" transform="rotate(-34 1090 235)"/>
</svg>
'''
        (sources / f'{mode}.svg').write_text(svg)
        renderer = QSvgRenderer(QByteArray(svg.encode()))
        if not renderer.isValid():
            raise RuntimeError(f'Invalid {mode} wallpaper SVG')
        renderer.setAspectRatioMode(Qt.IgnoreAspectRatio)
        folder = package / 'contents' / ('images_dark' if mode == 'dark' else 'images')
        folder.mkdir(parents=True, exist_ok=True)
        image = QImage(3840, 2160, QImage.Format_RGB32)
        image.fill(Qt.black)
        painter = QPainter(image)
        painter.setRenderHint(QPainter.Antialiasing)
        renderer.render(painter, QRectF(0, 0, image.width(), image.height()))
        painter.end()
        if not image.save(str(folder / '3840x2160.png')):
            raise RuntimeError(f'Could not save {mode} wallpaper')
        preview = ROOT / 'preview' / f'wallpaper-{mode}.png'
        preview.parent.mkdir(parents=True, exist_ok=True)
        if not image.scaled(960, 540, Qt.KeepAspectRatio, Qt.SmoothTransformation).save(str(preview)):
            raise RuntimeError(f'Could not save {mode} wallpaper preview')
    (package / 'metadata.json').write_text(json.dumps({
        'KPackageStructure': 'Wallpaper/Images',
        'KPlugin': {
            'Id': WALLPAPER_ID,
            'Name': 'Sequoia Eleven',
            'Description': 'Calm blue gradients and rounded ribbons that follow the system color scheme.',
            'License': 'MIT',
            'Version': '1.0.0',
        },
        'X-KDE-PlasmaImageWallpaper-AccentColor': {
            'Light': '#2463d4', 'Dark': '#8db3ff',
        },
    }, indent=2) + '\n')
    return package


if __name__ == '__main__':
    print('Built wallpaper:', build_wallpaper())
