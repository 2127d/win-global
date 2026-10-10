#!/usr/bin/env python3
"""Capture the real greeter in offscreen testing mode with sample media."""
import os
import argparse
import configparser
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mode', choices=['light', 'dark'], help='Preview a palette in isolated settings.')
parser.add_argument('--size', default='1280x720', help='Preview dimensions, e.g. 1280x720.')
parser.add_argument('--renderer', choices=['software', 'opengl'], default='software')
args = parser.parse_args()
width, height = map(int, args.size.split('x'))
output = root/'preview'/f'lockscreen{"-" + args.mode if args.mode else ""}.png'
if output.exists():
    output.unlink()
with tempfile.TemporaryDirectory(prefix='sequoia-lockscreen-') as staging:
    subprocess.run(['python3', str(root/'tools/install-lockscreen.py'), '--prefix', staging], check=True)
    shell = Path(staging)/'plasma/shells/org.kde.plasma.desktop'
    qml = shell/'contents/lockscreen'
    # These fixtures are confined to the temporary preview, never installed.
    (qml/'MediaControls.qml').write_text(('''import QtQuick
SequoiaMediaCard {
    trackTitle: "Golden Hour"
    artist: "Sequoia Eleven · Media preview"
    playerName: "Music"
    playing: true
    artwork: "ARTWORK"
}
''').replace('ARTWORK', (root/'tools/preview/media-art.svg').as_uri()))
    p=qml/'LockScreenUi.qml';s=p.read_text().replace('    id: lockScreenUi','''    id: lockScreenUi
    Timer { interval: 300; running: true; onTriggered: lockScreenRoot.uiVisible = true }
''',1);p.write_text(s)
    p=qml/'LockScreen.qml';s=p.read_text().replace('    id: root',f'''    id: root
    implicitWidth: {width}
    implicitHeight: {height}
    Timer {{
        interval: 2500; running: true
        onTriggered: {{
            console.warn("SEQUOIA_RENDER_API", root.GraphicsInfo.api);
            root.grabToImage(function(result) {{
            if (!result.saveToFile("{output}")) console.error("CAPTURE_FAILED");
            else console.log("SEQUOIA_CAPTURE_OK");
            }});
        }}
    }}
''',1).replace('    implicitWidth: 800', '').replace('    implicitHeight: 600', '');p.write_text(s)
    platform_config = Path(staging) / 'offscreen.json'
    platform_config.write_text(json.dumps({'screens': [{'name': 'preview', 'width': width, 'height': height}]}))
    env=os.environ.copy();env.update(QT_QPA_PLATFORM=f'offscreen:configfile={platform_config}', QT_FORCE_STDERR_LOGGING='1')
    if args.renderer == 'software': env['QT_QUICK_BACKEND'] = 'software'
    else:
        env.pop('QT_QUICK_BACKEND', None)
        env['QSG_RHI_BACKEND'] = 'opengl'
    if args.mode:
        # Exercise real palette selection without changing the user's session.
        config = Path(staging)/'config'
        config.mkdir()
        current = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home()/'.config')
        for name in ['kscreenlockerrc', 'plasmarc']:
            if (current/name).exists(): shutil.copy2(current/name, config/name)
        palette = configparser.ConfigParser(interpolation=None, strict=False)
        palette.optionxform = str
        palette.read([current/'kdeglobals', root/'color-schemes'/f'SequoiaEleven{args.mode.title()}.colors'])
        with (config/'kdeglobals').open('w') as stream: palette.write(stream, space_around_delimiters=False)
        subprocess.run(['kwriteconfig6', '--file', str(config/'plasmarc'), '--group', 'Theme',
                        '--key', 'name', f'sequoia-eleven-{args.mode}'], check=True)
        env['XDG_CONFIG_HOME'] = str(config)
    process=subprocess.Popen(['/usr/libexec/kscreenlocker_greet','--testing','--shell',str(shell)],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    try:
        log,_=process.communicate(timeout=5)
    except subprocess.TimeoutExpired:
        process.terminate();log,_=process.communicate(timeout=5)
    journal = subprocess.run(['journalctl','--user','--no-pager','-o','cat',f'_PID={process.pid}'],capture_output=True,text=True).stdout
    log += journal
    output.with_suffix('.log').write_text(log)
    errors = [line for line in log.splitlines() if any(term in line for term in ('ReferenceError:', 'TypeError:', 'unavailable', 'Cannot assign', 'Failed to load lockscreen', 'Binding loop', 'Detected anchors', 'Cannot override'))]
    print('\n'.join(errors) if errors else 'Greeter loaded without QML errors.')
    assert output.exists() and output.stat().st_size > 1000 and not errors, 'Lock-screen preview failed'
    print('Saved',output)
