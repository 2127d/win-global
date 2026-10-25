#!/usr/bin/env python3
"""Check rounded Dolphin breadcrumbs using real KIO widgets and a built style."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--plugin', type=Path, required=True, help='Built sequoiaeleven6.so')
parser.add_argument('--output-dir', type=Path, help='Keep rendered previews here')
parser.add_argument('--quick', action='store_true', help='Check light/dark at 1x/2x')
args = parser.parse_args()
plugin = args.plugin.resolve(strict=True)
source = Path(__file__).with_suffix('.cpp')
with tempfile.TemporaryDirectory(prefix='sequoia-breadcrumbs-') as temporary:
    root = Path(temporary)
    shutil.copy2(source, root/'main.cpp')
    (root/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.16)
project(breadcrumbprobe LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(KF6KIO REQUIRED)
add_executable(dolphin main.cpp)
target_link_libraries(dolphin PRIVATE Qt6::Widgets Qt6::Test KF6::KIOFileWidgets)
''')
    subprocess.run(['cmake', '-S', str(root), '-B', str(root/'build'), '-G', 'Ninja'], check=True, capture_output=True)
    subprocess.run(['cmake', '--build', str(root/'build')], check=True, capture_output=True)
    plugins = root/'plugins/styles'
    plugins.mkdir(parents=True)
    # A dedicated search path guarantees the tested binary wins over the system style.
    shutil.copy2(plugin, plugins/'sequoiaeleven6.so')
    test_path = root/'Documents/Codex'
    test_path.mkdir(parents=True)
    previews = args.output_dir.resolve() if args.output_dir else root/'previews'
    previews.mkdir(parents=True, exist_ok=True)
    count = 0
    for opacity in ([100] if args.quick else [100, 75]):
        config = root/f'config-{opacity}'
        config.mkdir()
        (config/'sequoiaelevenrc').write_text(
            f'[Style]\nAnimationsEnabled=false\nToolBarOpacity={opacity}\nDisableDolphinUrlNavigatorBackground=true\n')
        for scale in (['1', '2'] if args.quick else ['1', '1.25', '2']):
            for mode in ['light', 'dark']:
                for width in (['700'] if args.quick else ['700', '300']):
                    env = dict(os.environ, XDG_CONFIG_HOME=str(config), XDG_CACHE_HOME=str(root/'cache'),
                               QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                               QT_PLUGIN_PATH=str(plugins.parent), BREADCRUMB_EXPECT_PLUGIN=str(plugins/'sequoiaeleven6.so'),
                               BREADCRUMB_TEST_PATH=str(test_path), DBUS_SESSION_BUS_ADDRESS='unix:path='+str(root/'no-session'),
                               QT_SCALE_FACTOR=scale)
                    env.pop('LD_PRELOAD', None)
                    prefix = previews/f'{opacity}-{scale}-{mode}-{width}'
                    result = subprocess.run([str(root/'build/dolphin'), str(prefix), mode, width],
                                            env=env, capture_output=True, text=True, timeout=15)
                    if result.returncode:
                        raise RuntimeError(result.stdout + result.stderr)
                    print(result.stdout, end='', flush=True)
                    count += 1
    print(f'PASS: {count} real-widget combinations; corners, hover containment, editable mode and navigation.')
