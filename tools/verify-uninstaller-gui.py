#!/usr/bin/env python3
"""Check selective removal, preflight, and the Qt uninstaller in temporary roots."""
import argparse
import os
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from PySide6.QtCore import QProcess
from PySide6.QtGui import QCloseEvent
from PySide6.QtWidgets import QApplication
import uninstaller_gui as gui
import uninstaller_run as runner
from installer_platform import COMPONENTS, resolve_removals
from theme_files import Receipt
import widget_payload

installer = widget_payload.module('install-theme')
APP = QApplication([])


class RemovalTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='sequoia-uninstall-test-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.data = self.root / 'data with spaces'
        self.system = self.root / 'system'
        self.bins = self.root / 'bin'
        self.stage = self.root / 'payload'
        self.stage.mkdir()

    def put(self, root, name, content):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        return path

    def receipt(self):
        return Receipt(self.data, installer.USER_STATE, installer.user_allowed)

    def command(self, *args):
        return [sys.executable, str(gui.ROOT / 'tools/uninstaller_run.py'),
                '--prefix', str(self.data), '--system-root', str(self.system),
                '--bin-prefix', str(self.bins), *args]

    def test_partial_removal_restores_originals_and_keeps_other_backups(self):
        color = 'color-schemes/SequoiaElevenLight.colors'
        icon = 'icons/SequoiaTahoe/custom.svg'
        preset = 'plasma/look-and-feel/org.sequoia.eleven.light/metadata.json'
        self.put(self.data, color, 'original color')
        self.put(self.data, icon, 'original icon')
        for name in (color, icon, preset):
            self.put(self.stage, name, 'installed')
        self.receipt().install(self.stage)
        self.put(self.data, icon, 'personal icon edit')
        result = subprocess.run(self.command('--components', 'colors'), capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.data / color).read_text(), 'original color')
        self.assertFalse((self.data / preset).exists())
        self.assertEqual((self.data / icon).read_text(), 'personal icon edit')
        self.assertEqual(set(self.receipt().data['files']), {icon})
        self.receipt().uninstall(force=True)
        self.assertEqual((self.data / icon).read_text(), 'original icon')
        self.assertFalse(self.receipt().manifest.exists())

    def test_shared_files_are_retained_when_requested(self):
        name = next(n for n in installer.NATIVE_FILES if '/styles/' in n)
        self.put(self.stage, name, 'native fixture')
        Receipt(self.system, installer.SYSTEM_STATE, installer.system_allowed).install(self.stage)
        result = subprocess.run(self.command('--components', 'application-style', '--keep-system'), capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.system / name).exists())
        result = subprocess.run(self.command('--components', 'application-style'), capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.system / name).exists())

    def test_gui_worker_removes_staged_files(self):
        color = 'color-schemes/SequoiaElevenLight.colors'
        self.put(self.stage, color, 'installed color')
        self.receipt().install(self.stage)
        destinations = argparse.Namespace(prefix=self.data, system_root=self.system, bin_prefix=self.bins)
        window = gui.UninstallerWindow(destinations)
        self.addCleanup(window.close)
        window.all_theme.setChecked(False)
        window.component_boxes['colors'].setChecked(True)
        window.advance()
        self.assertIn('No shared system files are selected', window.review.toPlainText())
        self.assertTrue((self.data / color).exists())
        window.advance()
        self.assertTrue(window.process.waitForFinished(30000))
        self.assertEqual(window.status.text(), 'Removal complete', window.log.toPlainText())
        self.assertFalse((self.data / color).exists())

    def test_gwenview_removal_restores_original_without_version_checks(self):
        target = self.data.relative_to(self.root) / 'sequoia-eleven/gwenview'
        launcher = self.bins.relative_to(self.root) / 'gwenview'
        self.put(self.root, launcher, 'original gwenview launcher')
        self.put(self.stage, launcher, 'installed launcher')
        Receipt(self.root, str(target / 'installation'), lambda n: n == str(launcher)).install(self.stage)
        result = subprocess.run(self.command('--apps', 'gwenview'), capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.root / launcher).read_text(), 'original gwenview launcher')

    def test_app_preflight_failure_preserves_theme_and_force_restores_app(self):
        color = 'color-schemes/SequoiaElevenLight.colors'
        self.put(self.stage, color, 'installed color')
        self.receipt().install(self.stage)
        target = self.data.relative_to(self.root) / 'sequoia-eleven/settings-navigation'
        launcher = self.bins.relative_to(self.root) / 'systemsettings'
        self.put(self.root, launcher, 'original launcher')
        stage = self.root / 'app-stage'
        self.put(stage, launcher, 'installed launcher')
        Receipt(self.root, str(target / 'installation'), lambda n: n == str(launcher)).install(stage)
        self.put(self.root, launcher, 'local edits')
        command = self.command('--components', 'colors', '--apps', 'settings')
        failed = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn('edited', failed.stderr)
        self.assertTrue((self.data / color).exists())
        success = subprocess.run([*command, '--force'], capture_output=True, text=True)
        self.assertEqual(success.returncode, 0, success.stderr)
        self.assertFalse((self.data / color).exists())
        self.assertEqual((self.root / launcher).read_text(), 'original launcher')

    def test_removal_dependencies_do_not_install_or_expand_assets(self):
        self.assertEqual(resolve_removals(['colors']), ['colors', 'global-theme'])
        self.assertEqual(resolve_removals(['global-theme']), ['global-theme'])
        self.assertEqual(resolve_removals(['login']), ['login'])
        with patch.object(sys, 'argv', self.command('--components', 'widgets', '--plan')[1:]), \
                patch.object(runner, 'run') as run:
            runner.main()
            run.assert_not_called()

    def test_selected_system_edit_blocks_user_removal(self):
        self.put(self.stage, 'color-schemes/SequoiaElevenLight.colors', 'installed')
        self.receipt().install(self.stage)
        native_stage = self.root / 'native-stage'
        name = next(n for n in installer.NATIVE_FILES if '/styles/' in n)
        self.put(native_stage, name, 'installed')
        Receipt(self.system, installer.SYSTEM_STATE, installer.system_allowed).install(native_stage)
        self.put(self.system, name, 'edited')
        result = subprocess.run(self.command('--components', 'colors', 'application-style'), capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.data / 'color-schemes/SequoiaElevenLight.colors').exists())


class WizardTests(unittest.TestCase):
    def setUp(self):
        parser = argparse.ArgumentParser()
        runner.add_destinations(parser)
        self.window = gui.UninstallerWindow(parser.parse_args([]))
        self.addCleanup(self.window.close)

    def test_selection_review_and_restored_choices(self):
        w = self.window
        self.assertEqual(w.selection(), (list(COMPONENTS), []))
        w.all_theme.setChecked(False)
        self.assertFalse(w.next.isEnabled())
        w.component_boxes['colors'].setChecked(True)
        self.assertEqual(w.selection()[0], ['colors', 'global-theme'])
        self.assertFalse(w.component_boxes['global-theme'].isEnabled())
        w.all_theme.setChecked(True)
        w.all_theme.setChecked(False)
        self.assertEqual(w.selection()[0], ['colors', 'global-theme'])
        w.keep_system.setChecked(True)
        w.advance()
        self.assertIn('Color schemes', w.review.toPlainText())
        self.assertIn('Shared system files will be kept', w.review.toPlainText())
        self.assertEqual(w.next.text(), 'Remove selected components')
        w.go_back()
        self.assertTrue(w.component_boxes['colors'].isChecked())

    def test_apps_only_launch_and_no_removal_before_review(self):
        w = self.window
        w.all_theme.setChecked(False)
        w.app_boxes['gwenview'].setChecked(True)
        self.assertEqual(w.selection(), ([], ['gwenview']))
        w.force.setChecked(True)
        with patch.object(gui.QProcess, 'start') as start:
            w.advance()
            start.assert_not_called()
            w.advance()
            program, args = start.call_args.args
        self.assertEqual(program, '/usr/bin/python3')
        self.assertEqual(args[-4:], ['--components', '--apps', 'gwenview', '--force'])
        self.assertEqual(w.process.processEnvironment().value('SEQUOIA_GUI'), '1')
        self.assertTrue(w.process.processEnvironment().value('SUDO_ASKPASS').endswith('installer-askpass.py'))

    def test_process_output_failure_success_and_close_guard(self):
        w = self.window
        with patch.object(gui.QProcess, 'start'):
            w.start_removal()
        event = QCloseEvent()
        with patch.object(gui.QProcess, 'state', return_value=QProcess.Running), \
                patch.object(gui.QMessageBox, 'information'):
            w.closeEvent(event)
        self.assertFalse(event.isAccepted())
        w.process.start('/usr/bin/python3', ['-c', 'print("COMPLETED: Color schemes"); raise SystemExit(2)'])
        self.assertTrue(w.process.waitForFinished(5000))
        self.assertIn('stopped', w.status.text())
        self.assertIn('Color schemes', w.completed)
        self.assertIn('Earlier completed removals remain', w.log.toPlainText())
        self.assertTrue(w.next.isEnabled())
        w.finished(0, QProcess.NormalExit)
        self.assertEqual(w.status.text(), 'Removal complete')

    def test_failed_start(self):
        with patch.object(gui.QProcess, 'start'):
            self.window.start_removal()
        self.window.process_error(QProcess.FailedToStart)
        self.assertIn('stopped', self.window.status.text())
        self.assertTrue(self.window.next.isEnabled())


if __name__ == '__main__':
    unittest.main()
