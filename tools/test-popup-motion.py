#!/usr/bin/env python3
"""Check popup material ownership and interruption in the generated KWin script."""
import unittest
from PySide6.QtCore import QCoreApplication
from PySide6.QtQml import QJSEngine
from motion_palette import kwin_effect, effect_sources

APP = QCoreApplication([])

class PopupMotionTests(unittest.TestCase):
    def setUp(self):
        self.engine = QJSEngine()
        self.evaluate('''
            var Effect = {WindowForceBlurRole:1, WindowForceBackgroundContrastRole:2,
                WindowAddedGrabRole:3, WindowClosedGrabRole:4, Opacity:5, Translation:6};
            var QEasingCurve = {OutCubic:1, InCubic:2};
            function signal() { return {connect:function(f){this.callback=f;}}; }
            var effect = {configChanged:signal(), animationEnded:signal(),
                grab:function(){return true;}, isGrabbed:function(){return true;}};
            var effects = {hasActiveFullScreenEffect:false, windowAdded:signal(),
                windowClosed:signal(), windowDataChanged:signal()};
            var animations=[], cancellations=[];
            function animationTime(t){return t;}
            function animate(a) {
                if (!a.window.data(Effect.WindowForceBlurRole)) throw Error('Motion started without blur');
                if (!a.window.data(Effect.WindowForceBackgroundContrastRole)) throw Error('Motion started without contrast');
                animations.push(a); return [animations.length*2, animations.length*2+1];
            }
            function cancel(ids){cancellations.push(ids);}
            function print(){}
            function assert(value) { if (!value) throw Error('Assertion failed'); }
            function popup() { return {visible:true,popupWindow:true,windowClass:'test',roles:{},
                data:function(role){return this.roles[role];},
                setData:function(role,value){this.roles[role]=value;}}; }
            var w=popup();
        ''')
        self.evaluate(kwin_effect('fadingpopups', (effect_sources()/'fadingpopups/contents/code/main.js').read_text()))

    def evaluate(self, code):
        result = self.engine.evaluate(code)
        self.assertFalse(result.isError(), result.toString())

    def test_complete_both_transitions_and_restore_previous_roles(self):
        self.evaluate('''
            w.roles[1]=true; w.roles[2]=false;
            fadingPopupsEffect.slotWindowAdded(w);
            effect.animationEnded.callback(w,Effect.Opacity);
            assert(w.roles[1] && w.roles[2]);
            effect.animationEnded.callback(w,Effect.Translation);
            assert(w.roles[1]===true && w.roles[2]===false && !w.fadeInAnimation);
            fadingPopupsEffect.slotWindowClosed(w);
            effect.animationEnded.callback(w,Effect.Translation);
            assert(w.roles[1]===true && w.roles[2]===false && !w.fadeOutAnimation);
        ''')

    def test_close_interrupts_open_without_overlapping_tracks(self):
        self.evaluate('''
            fadingPopupsEffect.slotWindowAdded(w);
            fadingPopupsEffect.slotWindowClosed(w);
            assert(cancellations.length===1 && !w.fadeInAnimation && w.fadeOutAnimation);
            assert(w.roles[1] && w.roles[2]);
            effect.animationEnded.callback(w,Effect.Translation);
            assert(w.roles[1]===null && w.roles[2]===null && !w.sequoiaPopupMaterial);
        ''')

    def test_another_effect_taking_the_grab_releases_material(self):
        self.evaluate('''
            fadingPopupsEffect.slotWindowAdded(w);
            fadingPopupsEffect.slotWindowDataChanged(w,Effect.WindowAddedGrabRole);
            assert(!w.fadeInAnimation && w.roles[1]===null && w.roles[2]===null);
            fadingPopupsEffect.slotWindowClosed(w);
            fadingPopupsEffect.slotWindowDataChanged(w,Effect.WindowClosedGrabRole);
            assert(!w.fadeOutAnimation && w.roles[1]===null && w.roles[2]===null);
        ''')

    def test_rejected_windows_leave_roles_untouched(self):
        self.evaluate('''
            w.dock=true; fadingPopupsEffect.slotWindowAdded(w);
            w.dock=false; w.visible=false; fadingPopupsEffect.slotWindowAdded(w);
            w.visible=true; effect.grab=function(){return false;}; fadingPopupsEffect.slotWindowAdded(w);
            assert(animations.length===0 && Object.keys(w.roles).length===0);
        ''')

if __name__ == '__main__': unittest.main()
