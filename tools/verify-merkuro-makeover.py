#!/usr/bin/env python3
"""Exercise Merkuro's installed native pages inside private config/data/bus scopes."""
import argparse, os, shlex, subprocess, tempfile, uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--overlay',type=Path,default=ROOT/'.staging/merkuro-makeover')
parser.add_argument('--mode',choices=['light','dark'],default='light')
parser.add_argument('--material',choices=['solid','transparent','glass'],default='solid')
args=parser.parse_args()
helper=ROOT/'.staging/verify-merkuro-makeover.so'
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Test'],text=True))
if not helper.exists() or helper.stat().st_mtime < (ROOT/'apps/merkuro/verify.cpp').stat().st_mtime:
    subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(ROOT/'apps/merkuro/verify.cpp'),'-o',str(helper),*flags],check=True)
output=ROOT/'preview'/('merkuro-makeover-'+args.mode+'-'+args.material);output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='mm-') as temp:
    root=Path(temp)
    for kind in ('config','data','cache','state','runtime'):(root/kind).mkdir(mode=0o700)
    # No user calendars or accounts are copied to the test instance.
    scheme=ROOT/'themes/color-schemes'/('SequoiaEleven'+args.mode.title()+('Solid' if args.material=='solid' else 'Transparent')+'.colors')
    import configparser
    cfg=configparser.ConfigParser(interpolation=None);cfg.optionxform=str;cfg.read(scheme)
    if not cfg.has_section('KDE'):cfg.add_section('KDE')
    cfg.set('KDE','widgetStyle','SequoiaElevenGlass' if args.material=='glass' else 'SequoiaEleven')
    alpha=255 if args.material=='solid' else (168 if args.mode=='dark' else 148) if args.material=='glass' else (192 if args.mode=='dark' else 184)
    for group in ('Colors:Window','Colors:Header'):
        for key in ('BackgroundNormal','BackgroundAlternate'):
            if cfg.has_option(group,key):cfg.set(group,key,','.join(cfg.get(group,key).split(',')[:3]+[str(alpha)]))
    with (root/'config/kdeglobals').open('w') as stream:cfg.write(stream,space_around_delimiters=False)
    (root/'config/kwinrc').write_text('[Plugins]\nsequoia_liquid_glass_v6Enabled='+str(args.material=='glass').lower()+'\n')
    # Expose only the theme modules; calendars still use isolated data.
    local=root/'data/sequoia-eleven/quick-islands';local.parent.mkdir(parents=True)
    local.symlink_to(args.overlay.resolve())
    (root/'data/icons').symlink_to(Path.home()/'.local/share/icons')
    bus=root/'bus.conf';bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir='+str(root/'runtime')+'</listen><policy context="default"><allow send_destination="*"/><allow receive_sender="*"/><allow own="*"/></policy></busconfig>')
    env=dict(os.environ,QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_QPA_PLATFORMTHEME='kde',QT_FORCE_STDERR_LOGGING='1',SEQUOIA_MERKURO_VERIFY=str(output),AKONADI_INSTANCE='sequoia-preview-'+uuid.uuid4().hex[:10])
    for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'):env['XDG_'+kind+('_DIR' if kind=='RUNTIME' else '_HOME')]=str(root/kind.lower())
    for key in ('LD_PRELOAD','QT_STYLE_OVERRIDE','QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','KDE_APPLICATIONS_AS_SCOPE'):env.pop(key,None)
    command=['dbus-run-session','--config-file='+str(bus),'--','python3',str(ROOT/'tools/quick-preview-session.py'),'env','LD_PRELOAD='+str(helper),str(args.overlay.resolve()/'sequoia-quick'),'merkuro-calendar']
    with (output/'verify.log').open('w') as log:result=subprocess.run(command,env=env,stdout=log,stderr=log,timeout=60)
    text=(output/'verify.log').read_text()
    errors=[line for line in text.split('MAKEOVER FINISHED')[0].splitlines() if any(x in line for x in ('ReferenceError','TypeError','is not a type','Cannot assign','Unable to assign','Binding loop','MAKEOVER FAIL','failed to load component'))]
    assert result.returncode==0 and not errors, '\n'.join(errors) or text[-5000:]
    assert text.count('MAKEOVER PASS:')==7,text[-3000:]
    print('PASS:',args.mode,args.material,'Month, Today, Week, Schedule, Tasks, compact layout, native create action')
    print(output)
