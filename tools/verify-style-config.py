#!/usr/bin/env python3
from pathlib import Path
import ctypes,os,json
from PySide6.QtWidgets import QApplication,QWidget,QListWidget,QComboBox,QTabWidget
from PySide6.QtCore import QMetaObject,QSettings
from PySide6.QtTest import QTest
from shiboken6 import wrapInstance
ROOT=Path(__file__).resolve().parents[1]
import argparse,tempfile,subprocess,sys
parser=argparse.ArgumentParser(description='Verify every style page and tooltip save/reset/defaults with private configuration.')
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--child',action='store_true')
args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
if not args.child:
    with tempfile.TemporaryDirectory(prefix='sequoia-style-config-') as config:
        env=dict(os.environ,XDG_CONFIG_HOME=config,QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='SequoiaEleven',QT_PLUGIN_PATH=str(ROOT/'.staging/desktop/usr/lib64/qt6/plugins'))
        subprocess.run([sys.executable,__file__,'--child','--output',str(args.output.resolve())],env=env,check=True)
    raise SystemExit()
app=QApplication([])
lib=ctypes.CDLL(str(ROOT/'.staging/desktop/usr/lib64/qt6/plugins/kstyle_config/sequoiaelevenstyleconfig.so'))
f=lib.allocate_kstyle_config;f.argtypes=[ctypes.c_void_p];f.restype=ctypes.c_void_p
w=wrapInstance(f(None),QWidget);w.resize(1060,720);w.show();QTest.qWait(150)
nav=w.findChild(QListWidget,'styleNavigation');assert nav
out=args.output
for i in range(nav.count()):
 nav.setCurrentRow(i);QTest.qWait(75);w.grab().save(str(out/('tab-'+str(i)+'.png')))
 print('PASS page:',nav.item(i).text())
fields={s:w.findChild(QComboBox,'tooltipShape'+s) for s in ['Applications','Settings','Desktop','RunningApps']}
assert all(fields.values())
path=Path(os.environ['XDG_CONFIG_HOME'])/'sequoiatooltipsrc'
assert not path.exists()
fields['Desktop'].setCurrentIndex(2);QMetaObject.invokeMethod(w,'reset');assert fields['Desktop'].currentData()=='pill';assert not path.exists()
fields['Settings'].setCurrentIndex(1);fields['RunningApps'].setCurrentIndex(2);QMetaObject.invokeMethod(w,'save');assert path.exists()
config=QSettings(str(path),QSettings.IniFormat);assert config.value('Tooltips/Settings')=='rounded';assert config.value('Tooltips/RunningApps')=='square'
QMetaObject.invokeMethod(w,'defaults');assert fields['Settings'].currentData()=='pill';assert fields['RunningApps'].currentData()=='rounded';assert config.value('Tooltips/Settings')=='rounded'
QMetaObject.invokeMethod(w,'reset');assert fields['Settings'].currentData()=='rounded'
print('PASS tooltip Apply, Reset, Defaults staging and persisted section choices')
w.close()
