#!/usr/bin/env python3
"""Generate original, Qt-compatible nine-slice SVG assets and theme packages."""
from pathlib import Path
import json
import tarfile
import shutil
from typography import FONT_SETTINGS
from wallpaper import WALLPAPER_ID, build_wallpaper

ROOT = Path(__file__).resolve().parents[1]
VERSION = "2.0.0"
PALETTES = {
    "light": dict(surface="#f2f3f5", raised="#ffffff", view="#ffffff", text="#24262b", muted="#666b75",
                  border="#cdd0d6", accent="#2463d4", hover="#e7ebf2", selected="#d5e2ff", track="#d6dce8",
                  panel="#f3f5fa", panel_border="#c7cedb", widget_active="#ffffff"),
    "dark": dict(surface="#252629", raised="#303135", view="#1e1f22", text="#f2f3f5", muted="#adb0b8",
                 border="#494b51", accent="#8db3ff", hover="#36383e", selected="#3b5279", track="#42454d",
                 panel="#202020", panel_border="#484848", widget_active="#b0b0b0"),
}


class Svg:
    def __init__(self):
        self.parts = []
        self.row = 0

    def hint(self, name, width=1, height=1):
        self.parts.append(f'<rect id="{name}" x="800" y="{len(self.parts)*3}" width="{width}" height="{height}" fill="#ff00ff"/>')

    def frame(self, prefix, fill, radius=8, border=None, opacity=1, margin=None, stroke=1, inset=0, border_opacity=1):
        """Independent slices; no clipping, filters, or CSS unsupported by QtSvg."""
        ix, iy = inset if isinstance(inset, tuple) else (inset, inset)
        rx, ry, c, b = radius + ix, radius + iy, 32, stroke
        key = prefix + "-" if prefix else ""
        y0 = self.row * 100
        self.row += 1
        outer = f'M {rx} {iy} A {radius} {radius} 0 0 0 {ix} {ry} L {rx} {ry} Z'
        inner = f'M {rx} {iy+b} A {radius-b} {radius-b} 0 0 0 {ix+b} {ry} L {rx} {ry} Z'
        positions = {
            "topleft": (0, 0, rx, ry, ""),
            "top": (rx, 0, c, ry, None),
            "topright": (rx+c, 0, rx, ry, f'translate({rx} 0) scale(-1 1)' if ix != iy else f'translate({rx} 0) rotate(90)'),
            "left": (0, ry, rx, c, None),
            "center": (rx, ry, c, c, None),
            "right": (rx+c, ry, rx, c, None),
            "bottomleft": (0, ry+c, rx, ry, f'translate(0 {ry}) scale(1 -1)' if ix != iy else f'translate(0 {ry}) rotate(-90)'),
            "bottom": (rx, ry+c, c, ry, None),
            "bottomright": (rx+c, ry+c, rx, ry, f'translate({rx} {ry}) rotate(180)'),
        }
        for pos, (x, y, w, h, transform) in positions.items():
            # Invisible bounds make even empty/outline slices reliably measurable.
            content = f'<rect width="{w}" height="{h}" fill="#000" fill-opacity="0"/>'
            if transform is not None:
                shape = f'<path d="{outer}" fill="{fill}" fill-opacity="{opacity}"/>'
                if border:
                    shape += f'<path d="{outer} {inner}" fill-rule="evenodd" fill="{border}" fill-opacity="{border_opacity}"/>'
                content += f'<g transform="{transform}">{shape}</g>' if transform else shape
            else:
                fx, fy, fw, fh = {
                    "top": (0, iy, w, h-iy), "bottom": (0, 0, w, h-iy),
                    "left": (ix, 0, w-ix, h), "right": (0, 0, w-ix, h),
                    "center": (0, 0, w, h),
                }[pos]
                origin = f' x="{fx}" y="{fy}"' if inset else ''
                content += f'<rect{origin} width="{fw}" height="{fh}" fill="{fill}" fill-opacity="{opacity}"/>'
                if border and pos != "center":
                    bx, by, bw, bh = {"top":(0,iy,w,b), "bottom":(0,h-iy-b,w,b),
                                      "left":(ix,0,b,h), "right":(w-ix-b,0,b,h)}[pos]
                    content += f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" fill="{border}" fill-opacity="{border_opacity}"/>'
            self.parts.append(f'<g id="{key}{pos}" transform="translate({x} {y+y0})">{content}</g>')
        if margin is not None:
            for direction in ("top", "bottom", "left", "right"):
                self.hint(f'{key}hint-{direction}-margin', margin, margin)

    def shadow(self, radius):
        # A hollow shadow stays outside the surface, so translucent widgets do
        # not reveal a dark rectangular band around the transparent center tile.
        spread = 12
        r = radius + spread
        # Use the same opacity at each distance from the surface for both the
        # straight edges and rounded corners, avoiding seams at tile boundaries.
        profile = [(0, 0.14), (3, 0.09), (6, 0.045), (9, 0.015), (spread, 0)]
        def stop(offset, opacity):
            return f'<stop offset="{offset:.8f}" stop-color="#091326" stop-opacity="{opacity}"/>'
        edge_stops = ''.join(stop((spread-distance)/r, opacity)
                             for distance, opacity in reversed(profile))
        edge_stops += stop(spread/r, 0) + stop(1, 0)
        corner_stops = stop(0, 0) + stop(radius/r, 0)
        corner_stops += ''.join(stop((radius+distance)/r, opacity)
                               for distance, opacity in profile)
        self.parts.append(f'''<defs>
          <linearGradient id="shade" x1="0" y1="0" x2="0" y2="1" gradientUnits="objectBoundingBox">
            {edge_stops}
          </linearGradient>
          <radialGradient id="cornerShade" cx="1" cy="1" r="1">
            {corner_stops}
          </radialGradient></defs>''')
        y = self.row * 100
        self.row += 1
        for name, x, yy, w, h, transform in [
            ("topleft",0,0,r,r,""), ("topright",r+32,0,r,r,f'translate({r} 0) rotate(90)'),
            ("bottomleft",0,r+32,r,r,f'translate(0 {r}) rotate(-90)'),
            ("bottomright",r+32,r+32,r,r,f'translate({r} {r}) rotate(180)'),
            ("top",r,0,32,r,""), ("bottom",r,r+32,32,r,f'translate(32 {r}) rotate(180)'),
            ("left",0,r,r,32,f'translate(0 32) rotate(-90)'),
            ("right",r+32,r,r,32,f'translate({r} 0) rotate(90)'),
        ]:
            corner = "left" in name or "right" in name
            corner = corner and name not in ("left", "right")
            sw, sh = (r,r) if corner else (32,r)
            self.parts.append(f'<g id="shadow-{name}" transform="translate({x} {y+yy})"><g transform="{transform}"><rect width="{sw}" height="{sh}" fill="url(#{"cornerShade" if corner else "shade"})"/></g></g>')
        self.parts.append(f'<rect id="shadow-center" x="{r}" y="{y+r}" width="32" height="32" fill="#091326" fill-opacity="0"/>')
        for d in ("top", "bottom", "left", "right"):
            self.hint(f'shadow-hint-{d}-margin',spread,spread)
            self.hint(f'shadow-hint-{d}-inset',spread,spread)

    def save(self, path):
        path.parent.mkdir(parents=True, exist_ok=True)
        self.hint("hint-stretch-borders")
        path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="900" height="{max(100,self.row*100)}" viewBox="0 0 900 {max(100,self.row*100)}">\n' + '\n'.join(self.parts) + '\n</svg>\n')


def colors(p, name, material=None):
    def rgb(h):
        return ','.join(str(int(h[i:i+2],16)) for i in (1,3,5))
    sections = []
    for role in ["Window", "View", "Button", "Selection", "Tooltip", "Complementary", "Header"]:
        bg = p["accent"] if role == "Selection" else p["raised"] if role == "Button" else p["surface"]
        fg = ("#ffffff" if "Light" in name.split() else "#15213a") if role == "Selection" else p["text"]
        values = dict(BackgroundNormal=bg, BackgroundAlternate=p["hover"], ForegroundNormal=fg,
                      ForegroundInactive=p["muted"], ForegroundActive=p["accent"], ForegroundLink=p["accent"],
                      ForegroundVisited="#a17cce", ForegroundNegative="#d34a58", ForegroundNeutral="#ba770e",
                      ForegroundPositive="#25805d", DecorationFocus=p["accent"], DecorationHover=p["accent"])
        if role == 'Complementary':
            values.update(BackgroundNormal='#252629',BackgroundAlternate='#303135',ForegroundNormal='#f2f3f5',ForegroundInactive='#adb0b8')
        # Only the canvas/chrome backing is glass; content views are solid islands.
        alpha = 184 if role in ('Window', 'Header') else 255
        sections.append(f'[Colors:{role}]\n'+'\n'.join(
            f'{k}={rgb(v)}' + (f',{alpha}' if material == 'transparent' and k.startswith('Background') and alpha < 255 else '')
            for k,v in values.items()))
    sections.extend([f'[General]\nName={name}\nColorScheme={name.replace(" ", "")}\nshadeSortColumn=true',
                     '[KDE]\ncontrast=4\nframeContrast=0',
                     '[ColorEffects:Inactive]\nEnable=false\nChangeSelectionColor=true',
                     '[ColorEffects:Disabled]\nColor=112,115,122\nColorAmount=0\nColorEffect=0\nContrastAmount=0.55\nContrastEffect=1\nIntensityAmount=0\nIntensityEffect=0',
                     '[WM]\n'+ '\n'.join(f'{k}={rgb(v)}' + (',184' if material == 'transparent' and k.endswith('Background') else '') for k,v in dict(activeBackground=p["surface"], activeForeground=p["text"], inactiveBackground=p["surface"], inactiveForeground=p["muted"]).items())])
    return '\n\n'.join(sections)+'\n'


def build_tabbar(base, p, widget_alpha):
    s = Svg()
    s.parts.append(f'''<style id="current-color-scheme" type="text/css">
.ColorScheme-Highlight {{ color: {p['accent']}; }}
</style>''')
    for direction in ('north', 'south', 'east', 'west'):
        prefix = f'{direction}-active-tab'
        # Keep the native frame geometry and hit areas, but paint only a 3px
        # underline, inset 6px horizontally and 3px from the bottom edge.
        start = len(s.parts)
        s.frame(prefix, '#000000', radius=6, opacity=0, margin=6)
        for index in range(start, len(s.parts)):
            if f'id="{prefix}-bottom"' in s.parts[index]:
                s.parts[index] = s.parts[index].replace('</g>',
                    '<rect width="32" height="3" class="ColorScheme-Highlight" fill="currentColor"/></g>')
    # The stock launcher has its own compact square feedback in the shell.
    s.frame('sequoia-launcher', p['widget_active'], 7, opacity=widget_alpha,
            margin=6, inset=4)
    s.save(base / 'widgets/tabbar.svg')


def build_theme(mode,p):
    slug = f'sequoia-eleven-{mode}'
    base = ROOT / 'themes/plasma/desktoptheme' / slug
    base.mkdir(parents=True, exist_ok=True)
    (base/'LICENSE').write_text((ROOT/'LICENSE').read_text())
    name = f'Sequoia Eleven {mode.title()}'
    (base/'metadata.json').write_text(json.dumps({'KPlugin': {'Id':slug,'Name':name,
        'Description':'Frosted, softly rounded surfaces with crisp blue focus and selection.',
        'Version':VERSION,'License':'MIT','Authors':[{'Name':'Sequoia Eleven contributors'}]},
        'X-Plasma-API':'5.0'},indent=2)+'\n')
    (base/'plasmarc').write_text('[Settings]\nFallbackTheme=default\n\n[BlurBehindEffect]\nenabled=true\n\n[ContrastEffect]\nenabled=false\n\n[AdaptiveTransparency]\nenabled=true\n')
    scheme = colors(p,name)
    # A theme-local palette overrides the account accent in KSvg controls.
    # Keep named application color schemes, but let Plasma use kdeglobals.
    (base/'colors').unlink(missing_ok=True)
    (ROOT/'themes/color-schemes').mkdir(exist_ok=True)
    (ROOT/'themes/color-schemes'/f'SequoiaEleven{mode.title()}.colors').write_text(scheme)
    for folder, alpha in [('',0.76),('translucent',0.64),('opaque',1),('solid',1)]:
        for file,radius,margin in [('widgets/panel-background',16,6),('dialogs/background',14,12),
                                   ('widgets/background',16,12),('widgets/tooltip',9,8)]:
            s=Svg()
            is_panel = file.endswith('panel-background')
            surface = p['panel'] if is_panel else p['surface']
            border = p['panel_border'] if is_panel else p['border']
            s.frame('',surface,radius,border,alpha,margin)
            if is_panel:
                # Keep compositor blur inside the opaque 1px panel rim. A mask
                # reaching the antialiased outer contour can leave a halo at
                # rounded corners; preserve slice sizes while insetting it.
                s.frame('mask','#000000',radius - 1,inset=1)
            else:
                s.frame('mask','#000000',radius)
            if file == 'widgets/background' and folder not in ('opaque', 'solid'):
                # BasicAppletContainer enables wallpaper blur only when this
                # prefix exists, and clips the effect with blurred-mask.
                # Frosted glass: wallpaper provides the diffuse color/texture;
                # a neutral tint and a fine translucent rim keep it legible.
                glass_tint = '#ffffff' if mode == 'light' else '#141518'
                glass_alpha = 0.32 if mode == 'light' else 0.50
                rim_alpha = 0.50 if mode == 'light' else 0.22
                s.frame('blurred',glass_tint,radius,'#ffffff',glass_alpha,margin,
                        border_opacity=rim_alpha)
                s.frame('blurred-mask','#ffffff',radius)
            if is_panel:
                s.frame('thick',surface,radius,border,alpha,10)
            s.shadow(radius)
            s.save(base/folder/(file+'.svg'))
    from toggle_assets import build_switch
    build_switch(base, p, Svg)
    # Plasma uses Breeze assets for controls not explicitly overridden here.
    widget_alpha = 0.45 if mode == 'light' else 0.18
    files = {
      'button': [('normal','raised','border',1),('hover','widget_active',None,widget_alpha),
                 ('pressed','widget_active',None,widget_alpha),
                 ('focus','surface','accent',0),('toolbutton-hover','widget_active',None,widget_alpha),
                 ('toolbutton-pressed','widget_active',None,widget_alpha),('toolbutton-focus','surface','accent',0)],
      'lineedit': [('base','raised','border',1),('hover','surface','accent',0),
                   ('focus','surface','accent',0),('focusframe','surface','accent',0)],
      'viewitem': [('normal','surface',None,0),('hover','hover',None,1),
                   ('selected','accent',None,1),('selected+hover','accent','accent',1)],
      'pager': [('normal','raised','border',0.45),('hover','hover','border',1),('active','selected','accent',1)],
    }
    for file,states in files.items():
        s=Svg()
        for state,color,border,alpha in states:
            # Plasma expands line-edit overlays by their frame margins. Only
            # the base needs text padding; outlines must follow its border.
            s.frame(state,p[color],7,p[border] if border else None,alpha,6,
                    stroke=2 if 'focus' in state else 1)
            if file == 'lineedit' and state != 'base':
                s.hint(f'{state}-hint-no-border-padding')
        if file=='lineedit': s.hint('hint-focus-over-base')
        s.save(base/'widgets'/f'{file}.svg')
    build_tabbar(base, p, widget_alpha)
    s=Svg()
    # Pinned launchers have no running window and use the unprefixed frame.
    s.frame('',p['surface'],7,opacity=0,margin=5,inset=4)
    running = '#b8c1cf'
    running_alpha = 0.5 if mode == 'light' else 0.25
    task_states = [
        ('normal',running,running_alpha),
        ('minimized',running,running_alpha),
        ('focus',p['accent'],0.38),
        ('hover',running,running_alpha+0.12),
        ('normal-hover',running,running_alpha+0.12),
        ('minimized-hover',running,running_alpha+0.12),
        ('focus-hover',p['accent'],0.48),
        ('launcher-hover',running,0.15),
        ('attention',p['accent'],0.5),
        ('attention-hover',p['accent'],0.6),
        ('progress',p['accent'],0.3),
    ]
    for direction in ['', 'north-', 'south-', 'west-', 'east-']:
        for state,color,alpha in task_states:
            # Keep task layout margins stable; transparent edges shrink the painted
            # background even when Plasma stretches its frame to the panel height.
            s.frame(direction+state,color,7,opacity=alpha,margin=5,inset=4)
    s.save(base/'widgets/tasks.svg')


def main():
    wallpaper = build_wallpaper()
    for mode,p in PALETTES.items():
        build_theme(mode,p)
        package = ROOT/'themes/plasma/look-and-feel'/f'org.sequoia.eleven.{mode}'
        (package/'contents').mkdir(parents=True,exist_ok=True)
        (package/'contents/splash').mkdir(exist_ok=True)
        splash = (ROOT/'themes/plasma/Splash.qml').read_text()
        if mode == 'light':
            for old,new in [('#202124','#f2f3f5'),('#f1f3f7','#24262b'),('#34373e','#d7dae1'),('#9ebfff','#5085de')]:
                splash = splash.replace(old,new)
        (package/'contents/splash/Splash.qml').write_text(splash)
        preview = ROOT/'preview'/f'{mode}.png'
        if preview.exists():
            (package/'contents/previews').mkdir(exist_ok=True)
            shutil.copy2(preview,package/'contents/previews/preview.png')
        (package/'metadata.json').write_text(json.dumps({
            'KPackageStructure':'Plasma/LookAndFeel',
            'KPlugin':{'Id':f'org.sequoia.eleven.{mode}','Name':f'Sequoia Eleven {mode.title()}',
                       'Description':'Coordinated Sequoia Eleven desktop, application style, colors, and window decoration.',
                       'Version':VERSION,'License':'MIT'},
            'X-Plasma-APIVersion':'2'},indent=2)+'\n')
        (package/'contents/defaults').write_text(f'''[kdeglobals][KDE]
widgetStyle=SequoiaEleven

[kdeglobals][Icons]
Theme=SequoiaTahoe-{mode}

[kdeglobals][General]
ColorScheme=SequoiaEleven{mode.title()}
{chr(10).join(f'{key}={value}' for key, value in FONT_SETTINGS['General'].items())}

[kdeglobals][WM]
activeFont={FONT_SETTINGS['WM']['activeFont']}

[plasmarc][Theme]
name=sequoia-eleven-{mode}

[Wallpaper]
Image={WALLPAPER_ID}

[kwinrc][org.kde.kdecoration2]
library=org.kde.sequoiaeleven
ButtonsOnLeft=XIA
ButtonsOnRight=
BorderSize=None

[ksplashrc][KSplash]
Engine=KSplashQML
Theme=org.sequoia.eleven.{mode}
''')
    # Keep the original theme IDs for existing installations, and offer explicit
    # material choices as coordinated Global Themes and color schemes.
    for mode, palette in PALETTES.items():
        original = ROOT/'themes/plasma/look-and-feel'/f'org.sequoia.eleven.{mode}'
        for material in ('solid', 'transparent'):
            scheme_id = f'SequoiaEleven{mode.title()}{material.title()}'
            name = f'Sequoia Eleven {mode.title()} {material.title()}'
            (ROOT/'themes/color-schemes'/f'{scheme_id}.colors').write_text(colors(palette, name, material))
            package = original.with_name(f'{original.name}.{material}')
            shutil.copytree(original, package, dirs_exist_ok=True)
            metadata = json.loads((package/'metadata.json').read_text())
            metadata['KPlugin'].update(Id=package.name, Name=name,
                Description=f'{material.title()} application backgrounds with matching window decoration.')
            (package/'metadata.json').write_text(json.dumps(metadata, indent=2)+'\n')
            defaults = (original/'contents/defaults').read_text()
            defaults = defaults.replace(f'ColorScheme=SequoiaEleven{mode.title()}', f'ColorScheme={scheme_id}')
            defaults = defaults.replace('library=org.kde.sequoiaeleven', f'library=org.kde.sequoiaeleven.{material}')
            if material == 'transparent':
                defaults += '\n[kwinrc][Plugins]\nblurEnabled=true\n'
            (package/'contents/defaults').write_text(defaults)
    dist=ROOT/'dist'
    dist.mkdir(exist_ok=True)
    with tarfile.open(dist/f'sequoia-eleven-wallpaper-{VERSION}.tar.gz','w:gz') as archive:
        archive.add(wallpaper, arcname=WALLPAPER_ID)
    for mode in PALETTES:
        slug=f'sequoia-eleven-{mode}'
        with tarfile.open(dist/f'{slug}-{VERSION}.tar.gz','w:gz') as archive:
            archive.add(ROOT/'themes/plasma/desktoptheme'/slug,arcname=slug)
    with tarfile.open(dist/f'sequoia-tahoe-icons-{VERSION}.tar.gz','w:gz') as archive:
        for theme in sorted((ROOT/'themes/icons').glob('SequoiaTahoe*')):
            archive.add(theme, arcname=theme.name)
        archive.add(ROOT/'tools/build-tahoe-icons.py', arcname='build-tahoe-icons.py')
        archive.add(ROOT/'themes/icons/bold-icons.json', arcname='bold-icons.json')
    with tarfile.open(dist/f'sequoia-eleven-desktop-{VERSION}-source.tar.gz','w:gz') as archive:
        for name in ['themes', 'widgets', 'desktop', 'apps', 'tools', 'docs', 'preview', 'README.md', 'LICENSE', 'install.sh', 'install-gui.py', 'uninstall-gui.py', 'Install Sequoia Eleven.desktop', 'Uninstall Sequoia Eleven.desktop']:
            archive.add(ROOT/name,arcname=f'sequoia-eleven/{name}',filter=lambda item: None if (
                '__pycache__' in item.name or item.name.endswith('.pyc')
                or (item.name.startswith('sequoia-eleven/preview/') and item.name.endswith(('.log', '.tree')))
            ) else item)
    print('Built light and dark styles, matching color schemes, adaptive wallpaper, and installation archives.')


if __name__=='__main__': main()
