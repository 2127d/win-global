#!/usr/bin/env python3
"""Exercise backup, restoration, refresh and failure paths without changing the desktop."""
import configparser
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import motion_install as motion
from motion_palette import kwin_effect, duration, EFFECTS, effect_sources
from theme_files import fingerprint

RELOAD = motion.reload


class KWinPaletteTests(unittest.TestCase):
    def test_installed_effects_accept_the_current_kwin_sources(self):
        source = effect_sources()
        for name in EFFECTS:
            with self.subTest(effect=name):
                self.assertIn('Sequoia motion loaded:', kwin_effect(name, (source / name / 'contents/code/main.js').read_text()))

    def test_logout_preserves_68_retargeting_and_supports_67(self):
        legacy = 'duration: animationTime(800),\nduration: animationTime(400),'
        modern = ('duration: animationTime(400),\nconst duration = animationTime(200);\n'
                  'retarget(window.inAnimation, 0.0, duration);')
        for source in (legacy, modern):
            with self.subTest(source=source):
                updated = kwin_effect('logout', source)
                self.assertEqual(updated.count(duration(200)), 2)
                if source == modern:
                    self.assertIn('retarget(window.inAnimation, 0.0, duration);', updated)

    def test_unknown_logout_timing_is_still_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'Upstream animation changed'):
            kwin_effect('logout', 'duration: animationTime(900),\nconst duration = animationTime(200);')


class MotionTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='motion-installer-test-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.data, self.config = self.root / 'data', self.root / 'config'
        self.data.mkdir(); self.config.mkdir()
        self.base = self.data / 'plasma/plasmoids/example/main.qml'
        self.base.parent.mkdir(parents=True)
        self.base.write_text('original widget')
        (self.config / 'kdeglobals').write_text('[KDE]\nAnimationDurationFactor=1.5\nOther=keep\n')
        (self.config / 'kwinrc').write_text('[Plugins]\nslidingpopupsEnabled=true\nblurEnabled=true\n')
        self.receipt = self.data / 'sequoia-eleven/theme-installation/manifest.json'
        self.receipt.parent.mkdir(parents=True)
        self.receipt.write_text(json.dumps({'files': {str(self.base.relative_to(self.data)): {'installed': fingerprint(self.base)}}}))
        self.events = []
        self.effects = {'scale', 'slidingpopups', 'blur'}
        self.start_patch(patch.dict(os.environ, XDG_DATA_HOME=str(self.data), XDG_CONFIG_HOME=str(self.config)))
        self.start_patch(patch.object(motion, 'run', side_effect=self.run_command))
        self.start_patch(patch.object(motion, 'effect_call', side_effect=self.effect))
        self.start_patch(patch.object(motion, 'reload'))
        self.start_patch(patch.object(motion, 'notify_applications'))
        self.start_patch(patch.object(motion, 'preflight'))
        self.start_patch(patch('time.sleep'))

    def start_patch(self, patcher):
        value = patcher.start()
        self.addCleanup(patcher.stop)
        return value

    def effect(self, method, name):
        if method == 'unloadEffect':
            self.effects.discard(name)
        if method == 'loadEffect':
            self.effects.add(name)
        return 'true' if name in self.effects else 'false'

    def run_command(self, *args):
        self.events.append(args)
        if args[0] in ('kreadconfig6', 'kwriteconfig6'):
            path = Path(args[args.index('--file') + 1])
            if not path.is_absolute():
                path = self.config / path
            group = ']['.join(args[i+1] for i, a in enumerate(args) if a == '--group')
            key = args[args.index('--key') + 1]
            parser = configparser.ConfigParser(interpolation=None, strict=False)
            parser.optionxform = str
            parser.read(path)
            if args[0] == 'kreadconfig6':
                return parser.get(group, key, fallback='')
            # Every write must have a complete on-disk snapshot already.
            backups = list((self.data / 'sequoia-eleven/backups').glob('motion-*/manifest.json'))
            self.assertTrue(backups)
            self.assertTrue(all('originalEffects' in json.loads(p.read_text()) for p in backups))
            if not parser.has_section(group):
                parser.add_section(group)
            if '--delete' in args:
                parser.remove_option(group, key)
            else:
                parser[group][key] = args[-1]
            with path.open('w') as stream:
                parser.write(stream)
            return ''
        if args[-1] == 'org.kde.kwin.Effects.loadedEffects':
            return '\n'.join(sorted(self.effects))
        return 'active' if 'is-active' in args else ''

    def stage(self, path=None):
        stage = path or self.root / 'stage'
        path = stage / 'data' / self.base.relative_to(self.data)
        path.parent.mkdir(parents=True)
        path.write_text('custom widget')
        extra = stage / 'data/sequoia-eleven/motion/palette.json'
        extra.parent.mkdir(parents=True)
        extra.write_text('{"name":"Your palette"}')
        plan = {'data': str(self.data), 'config': str(self.config), 'sources': {},
                'effects': [], 'changes': [], 'limitations': [], 'settings': [
                    ['kdeglobals', 'KDE', 'AnimationDurationFactor', '0.6666666666666666'],
                    ['kwinrc', 'Plugins', 'slidingpopupsEnabled', 'true'],
                    ['kwinrc', 'Effect-slidingpopups', 'SlideInTime', '300']]}
        (stage / 'plan.json').write_text(json.dumps(plan))
        return stage

    def test_snapshot_precedes_changes_and_restore_preserves_other_settings_and_receipts(self):
        backup = motion.apply(self.stage())
        self.assertEqual(self.base.read_text(), 'custom widget')
        manifest = json.loads((backup / 'manifest.json').read_text())
        self.assertEqual(set(manifest['loadedEffectsBefore']), {'scale', 'slidingpopups', 'blur'})
        self.assertEqual(backup.stat().st_mode & 0o777, 0o700)
        # Simulate an unrelated theme installation and user setting change.
        tracked = json.loads(self.receipt.read_text())
        tracked['files']['color-schemes/another.colors'] = {'installed': 'keep new receipt entry'}
        self.receipt.write_text(json.dumps(tracked))
        self.run_command('kwriteconfig6', '--file', str(self.config / 'kdeglobals'), '--group', 'KDE', '--key', 'Other', 'changed')
        self.effects.add('sequoia-motion-scale')
        motion.restore(backup)
        self.assertEqual(self.base.read_text(), 'original widget')
        self.assertFalse((self.data / 'sequoia-eleven/motion/palette.json').exists())
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'), '--group', 'KDE', '--key', 'AnimationDurationFactor'), '1.5')
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'), '--group', 'KDE', '--key', 'Other'), 'changed')
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kwinrc'), '--group', 'Effect-slidingpopups', '--key', 'SlideInTime'), '')
        self.assertIn('color-schemes/another.colors', json.loads(self.receipt.read_text())['files'])
        self.assertEqual(self.effects, {'scale', 'slidingpopups', 'blur'})
        self.assertIsNone(motion.active_backup(self.data))

    def test_refresh_retains_initial_settings_and_new_base_files(self):
        first = motion.apply(self.stage())
        with patch.object(motion, 'prepare', side_effect=lambda stage, data, config: self.stage(stage)):
            with motion.theme_installation(self.data, ['widgets']):
                self.assertEqual(self.base.read_text(), 'original widget')
                self.base.write_text('updated base widget')
        second = motion.active_backup(self.data)
        self.assertNotEqual(first, second)
        motion.restore(second)
        self.assertEqual(self.base.read_text(), 'updated base widget')
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'), '--group', 'KDE', '--key', 'AnimationDurationFactor'), '1.5')

    def test_refresh_preserves_external_updates_and_retired_files_as_new_baseline(self):
        for retired in (False, True):
            with self.subTest(retired=retired):
                first = motion.apply(self.stage(self.root / ('initial-' + str(retired))))
                saved_manifest = (first / 'manifest.json').read_bytes()
                if retired:
                    self.base.unlink()
                else:
                    self.base.write_text('new base from theme upgrade')
                self.run_command('kwriteconfig6', '--file', str(self.config / 'kdeglobals'),
                                 '--group', 'KDE', '--key', 'AnimationDurationFactor', '2')
                with patch.object(motion, 'prepare', side_effect=lambda stage, data, config: self.stage(stage)):
                    with motion.theme_installation(self.data, ['animation-kit']):
                        self.assertEqual(self.base.exists(), not retired)
                        if not retired:
                            self.assertEqual(self.base.read_text(), 'new base from theme upgrade')
                second = motion.active_backup(self.data)
                self.assertNotEqual(first, second)
                self.assertEqual((first / 'manifest.json').read_bytes(), saved_manifest)
                motion.restore(second)
                self.assertEqual(self.base.exists(), not retired)
                if not retired:
                    self.assertEqual(self.base.read_text(), 'new base from theme upgrade')
                self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'),
                                                 '--group', 'KDE', '--key', 'AnimationDurationFactor'), '2')

    def test_apply_failure_rolls_back_files_settings_and_receipt(self):
        original_receipt = self.receipt.read_text()
        original_settings = (self.config / 'kwinrc').read_text()
        with patch.object(motion, 'reload', side_effect=RuntimeError('load failed')):
            with self.assertRaisesRegex(RuntimeError, 'load failed'):
                motion.apply(self.stage())
        self.assertEqual(self.base.read_text(), 'original widget')
        self.assertEqual(self.receipt.read_text(), original_receipt)
        self.assertEqual((self.config / 'kwinrc').read_text(), original_settings)
        self.assertIsNone(motion.active_backup(self.data))

    def test_overview_timing_failure_rolls_back_instead_of_reporting_success(self):
        self.effects.add('overview')
        original_receipt = self.receipt.read_bytes()
        def stale_overview(method, name):
            if method == 'supportInformation' and name == 'overview':
                return 'overview:\nanimationDuration: 300\n'
            return self.effect(method, name)
        with patch.object(motion, 'effect_call', side_effect=stale_overview):
            with self.assertRaisesRegex(RuntimeError, 'KWin workspace timing did not update'):
                motion.apply(self.stage())
        self.assertEqual(self.base.read_text(), 'original widget')
        self.assertEqual(self.receipt.read_bytes(), original_receipt)
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'),
                                         '--group', 'KDE', '--key', 'AnimationDurationFactor'), '1.5')
        self.assertEqual(self.run_command('kreadconfig6', '--file', str(self.config / 'kdeglobals'),
                                         '--group', 'KDE', '--key', 'Other'), 'keep')
        self.assertIsNone(motion.active_backup(self.data))

    def test_notification_rejects_other_config_directory_without_writes(self):
        with self.assertRaisesRegex(RuntimeError, 'current config directory'):
            motion.notify_factor(self.root / 'other-config')
        self.assertEqual(self.events, [])

    def test_changed_files_or_settings_stop_before_restoring(self):
        backup = motion.apply(self.stage())
        self.base.write_text('user edit')
        self.events.clear()
        with self.assertRaisesRegex(RuntimeError, 'File changed'):
            motion.restore(backup)
        self.assertFalse(any(e[0] in ('kwriteconfig6', 'systemctl') for e in self.events))
        self.base.write_text('custom widget')
        self.run_command('kwriteconfig6', '--file', str(self.config / 'kdeglobals'), '--group', 'KDE', '--key', 'AnimationDurationFactor', '2')
        self.events.clear()
        with self.assertRaisesRegex(RuntimeError, 'setting changed'):
            motion.restore(backup)
        self.assertFalse(any(e[0] in ('kwriteconfig6', 'systemctl') for e in self.events))

    def test_failed_theme_refresh_reinstates_existing_kit(self):
        motion.apply(self.stage())
        with patch.object(motion, 'prepare', side_effect=lambda stage, data, config: self.stage(stage)):
            with self.assertRaisesRegex(RuntimeError, 'build failed'):
                with motion.theme_installation(self.data, ['quick-style']):
                    raise RuntimeError('build failed')
        self.assertEqual(self.base.read_text(), 'custom widget')
        self.assertIsNotNone(motion.active_backup(self.data))

    def test_unselected_kit_never_mutates_settings(self):
        with motion.theme_installation(self.data, ['widgets', 'quick-style']):
            pass
        self.assertEqual(self.events, [])
        self.assertFalse((self.data / 'sequoia-eleven/backups').exists())

    def test_activation_restores_a_disabled_panel_slide_effect(self):
        self.effects.discard('slidingpopups')
        RELOAD([], True)
        self.assertIn('slidingpopups', self.effects)


if __name__ == '__main__':
    unittest.main()
