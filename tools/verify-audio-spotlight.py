#!/usr/bin/env python3
"""Render and exercise the Audio spotlight without changing real devices."""
import argparse, os, subprocess, sys, tempfile
from pathlib import Path
from preview_session import ensure_private_session
ensure_private_session()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--layout',choices=['spotlight','channels','mixer'],default='spotlight')
parser.add_argument('--menu',choices=['layout','options','devices'])
parser.add_argument('--dark',action='store_true')
parser.add_argument('--large',action='store_true')
parser.add_argument('--rtl',action='store_true')
parser.add_argument('--render-only',action='store_true')
parser.add_argument('--output',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
config=tempfile.TemporaryDirectory(prefix='sequoia-audio-preview-')
variant='Dark' if args.dark else 'Light'
(Path(config.name)/'kdeglobals').write_text((root/f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text())
(Path(config.name)/'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant.lower()}\n')
os.environ.update(XDG_DATA_DIRS=str(root/'themes')+':'+os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share'),XDG_CONFIG_HOME=config.name,QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',QT_STYLE_OVERRIDE='Fusion',QT_QPA_PLATFORMTHEME='generic')
if not args.render_only:
 result=subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner','-input',str(root/'desktop/control-center/audio/tests'),'-o','-,txt'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
 print(result.stdout,flush=True)
 bad=any(s in result.stdout for s in ['ReferenceError','TypeError','Binding loop','Unable to assign','Cannot assign','is not a type','polish() loop'])
 if result.returncode or bad:raise SystemExit(result.returncode or 1)
from PySide6.QtCore import QUrl,QTimer,Qt,qInstallMessageHandler,QObject,QMetaObject
from PySide6.QtGui import QFont,QColor,QPalette
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
errors=[]
def message(kind,context,text):
 print(text,file=sys.stderr)
 if any(s in text for s in ['ReferenceError','TypeError','Binding loop','Unable to assign','Cannot assign','is not a type','polish() loop']):errors.append(text)
qInstallMessageHandler(message)
app=QApplication([])
palette=QPalette()
for role,light,dark in [(QPalette.Window,'#edf0f6','#252630'),(QPalette.Base,'#ffffff','#33343f'),(QPalette.WindowText,'#242632','#f3f1fa'),(QPalette.Text,'#242632','#f3f1fa'),(QPalette.ButtonText,'#242632','#f3f1fa'),(QPalette.Highlight,'#ac8cc8','#bfa3df'),(QPalette.HighlightedText,'#ffffff','#22192b')]:palette.setColor(role,QColor(dark if args.dark else light))
app.setPalette(palette);app.setFont(QFont('Adwaita Sans',13 if args.large else 10))
if args.rtl:app.setLayoutDirection(Qt.RightToLeft)
view=QQuickView();view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(root/'desktop/control-center/audio/Preview.qml')))
assert view.status()==QQuickView.Ready,view.errors()
view.rootObject().property('fixture').setProperty('layoutMode', args.layout)
view.show()
if args.menu:
 def open_menu():
  name = {'layout':'audioLayoutPicker','options':'audioOptions:headphones','devices':'audioOutputPicker'}[args.menu]
  item = view.rootObject().findChild(QObject,name)
  assert item,name
  if args.menu == 'devices':
   from PySide6.QtQml import QQmlExpression, qmlContext
   expression=QQmlExpression(qmlContext(item),item,'popup.open()');expression.evaluate()
   assert not expression.hasError(),expression.error().toString()
  else: assert QMetaObject.invokeMethod(item,'clicked')
 QTimer.singleShot(250,open_menu)
def finish():
 try:
  assert not errors,errors
  output=args.output or root/('preview/audio-spotlight-'+variant.lower()+'.png')
  assert view.grabWindow().save(str(output));print(output);app.exit(0)
 except Exception as error:print(error,file=sys.stderr);app.exit(1)
QTimer.singleShot(900,finish)
raise SystemExit(app.exec())
