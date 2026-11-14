#!/usr/bin/env python3
"""Run QRCA's native and UI checks with private settings, clipboard and bus."""
import argparse
import os
from pathlib import Path
import subprocess
from preview_session import ensure_private_session
ensure_private_session()
root = Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir',type=Path,default=root/'apps/qrca/.build-verify')
parser.add_argument('--mode',choices=['light','dark'],default='dark')
parser.add_argument('--material',choices=['solid','transparent','glass'],default='glass')
args=parser.parse_args()
output=root/'preview'/('qrca-'+args.mode+'-'+args.material)
output.mkdir(parents=True,exist_ok=True)
variant='Light' if args.mode=='light' else 'Dark'
(Path(os.environ['XDG_CONFIG_HOME'])/'kdeglobals').write_text((root/f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text()+'\n[Icons]\nTheme=SequoiaTahoe-'+args.mode+'\n')
env=dict(os.environ,QT_FORCE_STDERR_LOGGING='1',QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',QT_STYLE_OVERRIDE='Fusion',QT_QPA_PLATFORMTHEME='generic',
    SEQUOIA_QRCA_ICONS=str(root/"themes/icons"),SEQUOIA_QRCA_VERIFY_DIR=str(output),SEQUOIA_QRCA_MODE=args.mode,SEQUOIA_QRCA_MATERIAL=args.material)
env['XDG_DATA_DIRS']=str(root/'themes')+':'+os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share')
env.pop('LD_PRELOAD',None)
result=subprocess.run([str(args.build_dir/'bin/qrca'),'--encode','https://example.org/sequoia'],env=env,capture_output=True,text=True,timeout=60)
print(result.stdout+result.stderr)
errors=('ReferenceError','TypeError','Binding loop','Unable to assign','Cannot assign','is not a type','Failed to create','FAIL:')
if result.returncode or any(error in result.stdout+result.stderr for error in errors):raise SystemExit(result.returncode or 1)
assert 'PASS:' in result.stdout+result.stderr
if args.mode == 'dark' and args.material == 'glass':
    subprocess.run([str(args.build_dir/'bin/qrcatest')], env=env, check=True, timeout=30)
print(output)
