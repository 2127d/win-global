#!/usr/bin/env python3
"""Exercise the real journal/config writer in isolated XDG homes; mock only desktop notifications."""
import configparser
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / 'apps/settings/appearance-material.py'
CHILD = '''import importlib.util,sys
spec=importlib.util.spec_from_file_location("material",sys.argv.pop(1))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
m.refresh=lambda:None
m.main()
'''

class TrialTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='appearance-material-test-')
        self.base = Path(self.temp.name)
        self.env = dict(os.environ)
        for kind in ('CONFIG','DATA','STATE'):
            folder = self.base / kind.lower(); folder.mkdir()
            self.env['XDG_'+kind+'_HOME'] = str(folder)
        (self.base/'data/color-schemes').symlink_to(ROOT/'themes/color-schemes')
        themes = self.base / 'data/plasma'; themes.mkdir()
        (themes / 'desktoptheme').symlink_to(Path.home()/'.local/share/plasma/desktoptheme')
        (self.base/'config/kdeglobals').write_text('[Colors:Window]\nBackgroundNormal=31,34,39,150\n[KDE]\nwidgetStyle=SequoiaElevenGlass\n[Icons]\nTheme=keep-icons\n[General]\nfont=keep-font\n')
        (self.base/'config/kwinrc').write_text('[Plugins]\nsequoia_liquid_glass_v6Enabled=true\nblurEnabled=false\n[org.kde.kdecoration2]\nlibrary=org.kde.sequoiaeleven.glass-adjustable\nButtonsOnLeft=XIA\n')
        (self.base/'config/sequoiaelevenrc').write_text('[Windeco]\nGlassRimIntensity=67\nDecorationScale=115\n')
        (self.base/'config/plasmarc').write_text('[Theme]\nname=sequoia-eleven-glass-dark\n')
        (self.base/'config/plasma-org.kde.plasma.desktop-appletsrc').write_text('unchanged-layout-wallpaper')
        self.before = self.values()

    def tearDown(self): self.temp.cleanup()
    def values(self):
        result={}
        for f in ['kdeglobals','kwinrc','sequoiaelevenrc','plasmarc']:
            c=configparser.ConfigParser(interpolation=None);c.optionxform=str;c.read(self.base/'config'/f)
            result[f] = {(g,k):v for g in c.sections() for k,v in c.items(g) if not k.endswith('[$d]')}
        return result
    def start(self, material='solid', mode='current'):
        return subprocess.Popen([sys.executable,'-c',CHILD,str(HELPER),material,'--mode',mode], env=self.env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    def ready(self,p):
        line=p.stdout.readline()
        self.assertTrue(line, p.stderr.read() if p.poll() is not None else 'Missing trial output')
        self.assertEqual(json.loads(line)['seconds'],15)
    def finish(self,p,command=None):
        if command: p.stdin.write(command+'\n');p.stdin.flush()
        p.wait(timeout=25)
        error = p.stderr.read()
        for stream in (p.stdin,p.stdout,p.stderr):
            if stream and not stream.closed: stream.close()
        self.assertEqual(p.returncode,0,error)
    def restored(self):
        self.assertEqual(self.values(),self.before)
        self.assertEqual((self.base/'config/plasma-org.kde.plasma.desktop-appletsrc').read_text(),'unchanged-layout-wallpaper')
    def test_explicit_revert_each_material(self):
        for material in ('solid','transparent','glass'):
            p=self.start(material);self.ready(p);self.finish(p,'revert');self.restored()
    def test_light_dark_color_trials_restore_every_key(self):
        for mode in ('light','dark'):
            p=self.start('glass',mode);self.ready(p)
            values=self.values()
            rgb=values['kdeglobals'][('Colors:Window','BackgroundNormal')].split(',')
            self.assertEqual(sum(map(int,rgb[:3])) < 384,mode=='dark')
            self.assertEqual(values['kdeglobals'][('KDE','AutomaticLookAndFeel')],'false')
            self.assertEqual(values['kdeglobals'][('General','font')],'keep-font')
            self.finish(p,'revert');self.restored()
    def test_timeout(self):
        p=self.start();self.ready(p);self.finish(p);self.restored()
    def test_parent_disappears(self):
        p=self.start();self.ready(p);p.stdin.close();self.finish(p);self.restored()
    def test_signal_interrupt(self):
        p=self.start();self.ready(p);p.send_signal(signal.SIGTERM);self.finish(p);self.restored()
    def test_keep_then_undo(self):
        p=self.start();self.ready(p);self.finish(p,'keep')
        self.assertNotEqual(self.values(),self.before)
        p=self.start('restore');self.finish(p);self.restored()
    def test_hard_kill_recovery(self):
        p=self.start();self.ready(p);p.kill();p.wait()
        for stream in (p.stdin,p.stdout,p.stderr): stream.close()
        p=self.start('recover');self.finish(p);self.restored()
    def test_concurrent_manual_edit_wins(self):
        p=self.start();self.ready(p)
        subprocess.run(['kwriteconfig6','--file',str(self.base/'config/kdeglobals'),'--group','KDE','--key','widgetStyle','Fusion'],check=True)
        self.finish(p,'revert')
        self.before['kdeglobals'][('KDE','widgetStyle')]='Fusion'
        self.restored()
    def test_mutually_exclusive_provider_order(self):
        spec=importlib.util.spec_from_file_location('material_order',HELPER)
        m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        m.CONFIG=self.base/'config'
        loaded={'blur'}
        def effect(method,*args):
            if method=='loadedEffects': return '\n'.join(sorted(loaded))
            name=args[0]
            if method=='unloadEffect': loaded.discard(name);return ''
            if method=='loadEffect':
                self.assertFalse(loaded, 'Incoming effect loaded before outgoing provider was removed')
                loaded.add(name);return 'true'
            if method=='reconfigureEffect': self.assertIn(name,loaded);return ''
            self.fail('Unexpected effect operation')
        m.effect_call=effect
        for material in ['glass','transparent','solid','glass','glass','transparent','glass']:
            for entry in m.plan(material): m.write(entry,entry['after'])
            m.sync_effects()
            self.assertEqual(loaded,{m.EFFECT} if material=='glass' else {'blur'} if material=='transparent' else set())

    def test_failed_apply_restores(self):
        child=CHILD.replace('m.refresh=lambda:None','''calls=0
def refresh():
 global calls
 calls+=1
 if calls==1: raise RuntimeError("simulated compositor failure")
m.refresh=refresh''')
        p=subprocess.run([sys.executable,'-c',child,str(HELPER),'solid'], env=self.env, input='',capture_output=True,text=True)
        self.assertNotEqual(p.returncode,0)
        self.restored()

if __name__=='__main__': unittest.main(verbosity=2)
