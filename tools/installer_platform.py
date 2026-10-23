"""Distribution packages and native paths shared by the CLI and GUI installers.

Package references: https://archlinux.org/packages/,
https://packages.fedoraproject.org/, https://packages.debian.org/,
https://packages.ubuntu.com/. Availability still depends on the enabled release.
"""
import os
import re
from pathlib import Path
import shlex
import subprocess
import sysconfig

DISTROS = {'arch': 'Arch Linux', 'fedora': 'Fedora', 'ubuntu': 'Ubuntu', 'debian': 'Debian'}
COMPONENTS = {
    'application-style': ('Application style', 'Native controls, menus and floating surfaces.'),
    'decorations': ('Window decorations', 'Rounded title bars and window buttons.'),
    'plasma-style': ('Plasma style', 'Panel, widget and switch artwork.'),
    'colors': ('Color schemes', 'Light and dark, solid and transparent palettes.'),
    'icons': ('Icons', 'The complete Sequoia Tahoe icon family.'),
    'wallpaper': ('Wallpaper', 'Adaptive light and dark wallpaper.'),
    'fonts': ('Fonts', 'Adwaita Sans and Nunito.'),
    'widgets': ('Desktop widgets', 'Custom tray, trash, animated task manager, Start Menu, Downloads Stack, Dock Separator, Kpple Menu and Window Title Fork.'),
    'global-theme': ('Global Theme, desktop layout & splash', 'Coordinated presets and top bar/dock layout. Includes the styles, colors, icons, wallpaper, fonts and widgets referenced by the presets.'),
    'lockscreen': ('Lock screen', 'Clock, password field and media controls.'),
    'quick-style': ('Qt Quick application styling', 'Shared KDE application surfaces and switches.'),
    'login': ('Login screen', 'Requires the existing Plasma Login Manager; takes effect after reboot.'),
    'animation-kit': ('Animation kit — Your palette', 'Optional system animations from Motion Lab. Saves a restorable snapshot before applying; restarts Plasma. Includes desktop widgets and Qt Quick styling.'),
}
OPTIONAL_COMPONENTS = {'animation-kit'}
APPS = {'settings': ('Settings makeover', 'Requires System Settings 6.7.5.'),
        'gwenview': ('Gwenview makeover', 'Requires Gwenview 26.08.1.'),
        'kcalc': ('KCalc makeover', 'Modern keys and mode-aware resizing. Requires KCalc 26.08.1.'),
        'kweather': ('Weather makeover', 'Atmospheric skies and Sequoia forecast cards. Requires KWeather 26.08.1 from Flatpak.')}
NATIVE = {'application-style', 'decorations'}

# A preset must never point at theme assets or layout widgets we did not install.
# Individual components remain independent choices in both entry points.
COMPONENT_REQUIRES = {
    'animation-kit': {'widgets', 'quick-style'},
    'global-theme': {'application-style', 'decorations', 'plasma-style', 'colors',
                     'icons', 'wallpaper', 'fonts', 'widgets'},
}

def resolve_components(components):
    selected = set(components)
    unknown = selected - COMPONENTS.keys()
    if unknown:
        raise ValueError('Unknown components: ' + ', '.join(sorted(unknown)))
    while True:
        expanded = selected | set().union(*(COMPONENT_REQUIRES.get(c, set()) for c in selected))
        if expanded == selected:
            return [c for c in COMPONENTS if c in selected]
        selected = expanded

def login_available():
    return Path('/usr/libexec/plasma-login-greeter').is_file()

def resolve_removals(components):
    # Removing an asset also retires presets that would reference missing files.
    selected = set(components)
    unknown = selected - COMPONENTS.keys()
    if unknown:
        raise ValueError('Unknown components: ' + ', '.join(sorted(unknown)))
    while True:
        expanded = selected | {c for c, required in COMPONENT_REQUIRES.items() if selected & required}
        if expanded == selected:
            return [c for c in COMPONENTS if c in selected]
        selected = expanded

def default_components():
    return [c for c in COMPONENTS if c not in OPTIONAL_COMPONENTS and (c != 'login' or login_available())]

def detected_distro():
    try:
        release = dict(line.split('=', 1) for line in Path('/etc/os-release').read_text().splitlines()
                       if '=' in line and not line.startswith('#'))
        return release.get('ID', '').strip('"\'')
    except OSError:
        return ''

def native_layout(distro=None):
    distro = distro or detected_distro()
    if distro == 'fedora':
        lib = 'lib64' if sysconfig.get_config_var('SIZEOF_VOID_P') == 8 else 'lib'
    elif distro in ('ubuntu', 'debian'):
        lib = 'lib/' + sysconfig.get_config_var('MULTIARCH')
    else:
        lib = 'lib'
    return lib, lib + '/qt6/plugins'

def sudo_command(*command):
    return ['sudo', *(['-A'] if os.environ.get('SEQUOIA_GUI') == '1' else []), *map(str, command)]

def run(command):
    print('+', shlex.join(command), flush=True)
    subprocess.run(command, check=True)

def build_jobs():
    """Use every available logical CPU unless BUILD_JOBS explicitly overrides it."""
    override = os.environ.get('BUILD_JOBS')
    if override:
        try:
            jobs = int(override)
        except ValueError:
            jobs = 0
        if jobs < 1:
            raise ValueError('BUILD_JOBS must be a positive integer.')
        return jobs
    try:
        available = len(os.sched_getaffinity(0))
        if available:
            return available
    except (AttributeError, OSError):
        pass
    return max(1, os.cpu_count() or 1)

_KF = 'kcolorscheme kconfig kconfigwidgets kcoreaddons kguiaddons ki18n kiconthemes kwindowsystem kcmutils'
_APP_KF = 'kauth kcrash kitemviews kitemmodels kio kservice kwidgetsaddons kxmlgui kdbusaddons kjobwidgets kparts knotifications'

def debian_kf_package(name):
    return 'libkf6' + (name if name in ('kcmutils', 'kio') else name[1:]) + '-dev'

BUILD = {
    'fedora': ['cmake', 'gcc-c++', 'ninja-build', 'extra-cmake-modules', 'pkgconf-pkg-config',
               'qt6-qtbase-devel', 'qt6-qtdeclarative-devel', 'kf6-kirigami-devel',
               'kf6-frameworkintegration-devel', *('kf6-' + p + '-devel' for p in _KF.split())],
    'arch': ['cmake', 'gcc', 'ninja', 'extra-cmake-modules', 'pkgconf', 'qt6-base',
             'qt6-declarative', 'kirigami', 'frameworkintegration', *_KF.split()],
    'debian': ['cmake', 'g++', 'ninja-build', 'extra-cmake-modules', 'pkg-config',
               'qt6-base-dev', 'qt6-declarative-dev', 'libkirigami-dev',
               'libkf6style-dev', *(debian_kf_package(p) for p in _KF.split())],
}
BUILD['ubuntu'] = BUILD['debian']
GUI_PACKAGES = {'fedora': ['python3-pyside6'], 'arch': ['pyside6'],
                'ubuntu': ['python3-pyside6.qtwidgets'], 'debian': ['python3-pyside6.qtwidgets']}

def packages_for(distro, components, apps=()):
    selected = set(resolve_components(components))
    apps = set(apps)
    packages = []
    if selected & NATIVE or selected & {'login', 'widgets'} or apps:
        packages += BUILD[distro] + ['gettext']
    if 'decorations' in selected:
        packages += [{'fedora': 'kdecoration-devel', 'arch': 'kdecoration',
                      'ubuntu': 'libkdecorations3-dev', 'debian': 'libkdecorations3-dev'}[distro]]
    if 'widgets' in selected:
        packages += GUI_PACKAGES[distro] + ['plasma-workspace', 'plasma-desktop', 'plasma-nm', 'plasma-pa', 'powerdevil']
        packages += ['bluedevil', 'kdeplasma-addons']
        packages += [{'fedora': 'plasma-print-manager', 'arch': 'print-manager',
                      'ubuntu': 'print-manager', 'debian': 'print-manager'}[distro]]
    if selected & {'widgets', 'lockscreen', 'login', 'global-theme'}:
        packages += [{'fedora': 'qt6-qt5compat', 'arch': 'qt6-5compat',
                      'ubuntu': 'qml6-module-qt5compat-graphicaleffects',
                      'debian': 'qml6-module-qt5compat-graphicaleffects'}[distro]]
    if 'fonts' in selected or 'login' in selected:
        packages += ['fontconfig']
    if 'login' in selected:
        packages += ['plasma-nm', 'plasma-pa']
    if 'icons' in selected:
        packages += ['gtk-update-icon-cache']
    if 'quick-style' in selected or 'lockscreen' in selected:
        packages += GUI_PACKAGES[distro] + ['plasma-workspace']
    if 'animation-kit' in selected:
        packages += {'fedora': ['qt6-qttools'], 'arch': ['qt6-tools'],
                     'ubuntu': ['qdbus-qt6', 'python3-pyside6.qtdbus'],
                     'debian': ['qdbus-qt6', 'python3-pyside6.qtdbus']}[distro]
    if 'quick-style' in selected:
        packages += {'fedora': ['kf6-qqc2-desktop-style', 'kf6-kirigami', 'kf6-kcmutils', 'plasma-systemmonitor', 'qt6-qtsvg'],
                     'arch': ['qqc2-desktop-style', 'kirigami', 'kcmutils', 'plasma-systemmonitor', 'qt6-svg'],
                     'ubuntu': ['qml6-module-org-kde-desktop', 'qml6-module-org-kde-kirigami',
                                'qml6-module-org-kde-kcmutils', 'plasma-systemmonitor'],
                     'debian': ['qml6-module-org-kde-desktop', 'qml6-module-org-kde-kirigami',
                                'qml6-module-org-kde-kcmutils', 'plasma-systemmonitor']}[distro]
    if apps:
        packages += {'fedora': ['qt6-qttools-devel'], 'arch': ['qt6-tools'],
                     'ubuntu': ['qt6-tools-dev', 'qt6-tools-dev-tools'],
                     'debian': ['qt6-tools-dev', 'qt6-tools-dev-tools']}[distro]
        packages += {'fedora': ['plasma-activities-devel', *('kf6-' + p + '-devel' for p in _APP_KF.split())],
                     'arch': ['plasma-activities', *_APP_KF.split()],
                     'ubuntu': ['libplasmaactivities-dev', *(debian_kf_package(p) for p in _APP_KF.split())],
                     'debian': ['libplasmaactivities-dev', *(debian_kf_package(p) for p in _APP_KF.split())]}[distro]
    if 'gwenview' in apps:
        packages += {
            'fedora': ['qt6-qtsvg-devel', 'qt6-qtmultimedia-devel', 'qt6-qtwayland-devel',
                       'qt6-qtbase-private-devel', 'wayland-devel', 'wayland-protocols-devel',
                       'libjpeg-turbo-devel', 'libpng-devel', 'exiv2-devel', 'lcms2-devel',
                       'kimageannotator-qt6-devel', 'kcolorpicker-qt6-devel', 'kf6-baloo-devel', 'kf6-purpose-devel',
                       'libkdcraw-devel', 'cfitsio-devel', 'libtiff-devel', 'libX11-devel', 'binutils'],
            'arch': ['qt6-svg', 'qt6-multimedia', 'qt6-wayland', 'wayland', 'wayland-protocols',
                     'libjpeg-turbo', 'libpng', 'exiv2', 'lcms2', 'kimageannotator', 'kcolorpicker',
                     'baloo', 'purpose', 'libkdcraw', 'cfitsio', 'libtiff', 'libx11', 'binutils'],
            'ubuntu': ['qt6-svg-dev', 'qt6-multimedia-dev', 'qt6-wayland-dev', 'qt6-base-private-dev',
                       'libwayland-dev', 'wayland-protocols', 'libjpeg-dev', 'libpng-dev',
                       'libexiv2-dev', 'liblcms2-dev', 'libkimageannotator-qt6-dev', 'libkcolorpicker-qt6-dev', 'libkf6baloo-dev',
                       'libkf6purpose-dev', 'libkdcrawqt6-dev', 'libcfitsio-dev', 'libtiff-dev', 'libx11-dev', 'binutils'],
            'debian': ['qt6-svg-dev', 'qt6-multimedia-dev', 'qt6-wayland-dev', 'qt6-base-private-dev',
                       'libwayland-dev', 'wayland-protocols', 'libjpeg-dev', 'libpng-dev',
                       'libexiv2-dev', 'liblcms2-dev', 'libkimageannotator-qt6-dev', 'libkcolorpicker-qt6-dev', 'libkf6baloo-dev',
                       'libkf6purpose-dev', 'libkdcrawqt6-dev', 'libcfitsio-dev', 'libtiff-dev', 'libx11-dev', 'binutils'],
        }[distro]
    if 'kcalc' in apps:
        packages += {'fedora': ['gmp-devel', 'mpfr-devel', 'libmpc-devel'],
                     'arch': ['gmp', 'mpfr', 'libmpc'],
                     'ubuntu': ['libgmp-dev', 'libmpfr-dev', 'libmpc-dev'],
                     'debian': ['libgmp-dev', 'libmpfr-dev', 'libmpc-dev']}[distro]
    if 'kweather' in apps:
        packages += {
            'fedora': ['flatpak', 'qt6-qtcharts-devel', 'qt6-qtpositioning-devel', 'qt6-qtsvg-devel',
                       'kf6-kholidays-devel', 'kf6-kirigami-addons-devel', 'kf6-qqc2-desktop-style'],
            'arch': ['flatpak', 'qt6-charts', 'qt6-positioning', 'qt6-svg', 'kholidays', 'kirigami-addons', 'qqc2-desktop-style'],
            'ubuntu': ['flatpak', 'qt6-charts-dev', 'qt6-positioning-dev', 'qt6-svg-dev', 'libkf6holidays-dev',
                       'kirigami-addons-dev', 'qml6-module-org-kde-kholidays', 'qml6-module-org-kde-desktop'],
            'debian': ['flatpak', 'qt6-charts-dev', 'qt6-positioning-dev', 'qt6-svg-dev', 'libkf6holidays-dev',
                       'kirigami-addons-dev', 'qml6-module-org-kde-kholidays', 'qml6-module-org-kde-desktop'],
        }[distro]
    return sorted(set(packages))

def package_command(distro, packages):
    manager = {'fedora': ['dnf', 'install', '-y'], 'arch': ['pacman', '-S', '--needed', '--noconfirm'],
               'ubuntu': ['apt-get', 'install', '-y'], 'debian': ['apt-get', 'install', '-y']}[distro]
    return sudo_command(*manager, *packages)

def install_dependencies(distro, components, apps=()):
    if detected_distro() != distro:
        raise RuntimeError(f'The selected distribution ({distro}) does not match /etc/os-release '
                           f'({detected_distro() or "unknown"}). Select the installed distribution.')
    packages = packages_for(distro, components, apps)
    missing = []
    for name in packages:
        query = (['rpm', '-q', '--quiet', name] if distro == 'fedora' else
                 ['pacman', '-Q', name] if distro == 'arch' else
                 ['dpkg-query', '-W', '-f=${db:Status-Status}', name])
        result = subprocess.run(query, capture_output=True, text=True)
        if result.returncode or (distro in ('ubuntu', 'debian') and result.stdout != 'installed'):
            missing.append(name)
    if missing:
        run(package_command(distro, missing))
    else:
        print('Selected dependencies are already installed.', flush=True)

def application_version(executable):
    # Query the distribution database without starting a single-instance app.
    # Gwenview can wait for D-Bus even for --version on a disconnected session.
    distro = detected_distro()
    query = {'fedora': ['rpm', '-qf', '--qf', '%{VERSION}', '/usr/bin/' + executable],
             'arch': ['pacman', '-Q', executable],
             'ubuntu': ['dpkg-query', '-W', '-f=${Version}', executable],
             'debian': ['dpkg-query', '-W', '-f=${Version}', executable]}.get(distro)
    if query:
        try:
            result = subprocess.check_output(query, text=True, stderr=subprocess.DEVNULL, timeout=15)
            version = result.strip().split()[-1].split(':')[-1]
            match = re.match(r'\d+(?:\.\d+)+', version)
            return match.group(0) if match else None
        except (OSError, subprocess.SubprocessError, IndexError):
            return None
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic',
               QT_STYLE_OVERRIDE='Fusion', DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-sequoia-check')
    env.pop('LD_PRELOAD', None)
    try:
        result = subprocess.check_output(['/usr/bin/' + executable, '--version'], env=env,
                                         text=True, stderr=subprocess.DEVNULL, timeout=15)
        return result.strip().split()[-1]
    except (OSError, subprocess.SubprocessError, IndexError):
        return None

def app_problem(app):
    if app == 'kweather':
        try:
            import json
            location = Path(subprocess.check_output(['flatpak', 'info', '--show-location', 'org.kde.kweather'], text=True, stderr=subprocess.DEVNULL).strip())
            manifest = json.loads((location / 'files/manifest.json').read_text())
            source = next(m['sources'][0]['url'] for m in manifest['modules'] if m['name'] == 'kweather')
            if source.endswith('/kweather-26.08.1.tar.xz'):
                return ''
        except (OSError, ValueError, KeyError, StopIteration, subprocess.SubprocessError):
            pass
        return 'Weather makeover requires KWeather 26.08.1 installed through Flatpak (org.kde.kweather).'
    executable, required = {'settings': ('systemsettings', '6.7.5'), 'gwenview': ('gwenview', '26.08.1'), 'kcalc': ('kcalc', '26.08.1')}[app]
    version = application_version(executable)
    if version != required:
        return f'{APPS[app][0]} requires {executable} {required}; found {version or "not installed"}.'
    return ''

def gwenview_library():
    # ldd follows the installed binary's loader paths, including Debian multiarch.
    result = subprocess.check_output(['ldd', '/usr/bin/gwenview'], text=True)
    for line in result.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0] == 'libgwenviewlib.so.5' and parts[1] == '=>':
            path = Path(parts[2])
            if path.is_absolute() and path.is_file():
                return path
    raise RuntimeError('Cannot find the installed Gwenview library.')
