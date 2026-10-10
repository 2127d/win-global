#!/usr/bin/env python3
"""Check selective installation/removal of the shared launcher/lock-screen shell."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import shell_overrides as shell
from theme_files import Receipt, payload_files

spec = importlib.util.spec_from_file_location('installer', Path(__file__).with_name('install-theme.py'))
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)

class ShellTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='sequoia-shell-test-')
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.live = self.base / 'user data'
        self.count = 0

    def receipt(self):
        return Receipt(self.live, installer.USER_STATE, installer.user_allowed)

    def install(self, *components):
        self.count += 1
        stage = self.base / f'stage-{self.count}'
        installer.stage_user(stage, self.live, set(components))
        self.receipt().install(stage, scope=lambda n: installer.user_scope(n, components))
        self.receipt().validate()

    def remove(self, *components):
        command = [sys.executable, str(installer.ROOT / 'tools/install-theme.py'), 'uninstall',
                   '--prefix', str(self.live), '--keep-system', '--components', *components]
        subprocess.run([*command, '--check'], check=True, capture_output=True)
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def read(self, name):
        return (self.live / shell.SHELL / name).read_bytes()

    def test_style_only_keeps_stock_lock_screen_and_removes_cleanly(self):
        self.install('plasma-style')
        self.assertEqual(shell.installed(self.receipt().data['files']), {'plasma-style'})
        self.assertIn(b'squareLauncher', self.read(shell.COMPACT))
        self.assertEqual(self.read('contents/lockscreen/LockScreenUi.qml'), (shell.SYSTEM_SHELL / 'contents/lockscreen/LockScreenUi.qml').read_bytes())
        self.remove('plasma-style')
        self.assertFalse(payload_files(self.live / shell.SHELL))
        self.assertFalse(self.receipt().manifest.exists())

    def test_remove_style_retains_lock_screen_and_refresh(self):
        self.install('lockscreen')
        original = self.read('contents/lockscreen/LockScreenUi.qml')
        self.install('plasma-style')
        self.assertEqual(self.read('contents/lockscreen/LockScreenUi.qml'), original)
        self.remove('plasma-style')
        self.assertEqual(shell.installed(self.receipt().data['files']), {'lockscreen'})
        self.assertEqual(self.read('contents/lockscreen/LockScreenUi.qml'), original)
        self.assertEqual(self.read(shell.COMPACT), (shell.SYSTEM_SHELL / shell.COMPACT).read_bytes())
        self.install('plasma-style')
        self.remove('plasma-style', 'lockscreen')
        self.assertFalse(payload_files(self.live / shell.SHELL))

    def test_remove_lock_screen_retains_launcher_and_refresh(self):
        self.install('plasma-style')
        original = self.read(shell.COMPACT)
        self.install('lockscreen')
        self.assertEqual(self.read(shell.COMPACT), original)
        self.remove('lockscreen')
        self.assertEqual(shell.installed(self.receipt().data['files']), {'plasma-style'})
        self.assertEqual(self.read(shell.COMPACT), original)
        self.assertEqual(self.read('contents/lockscreen/LockScreenUi.qml'), (shell.SYSTEM_SHELL / 'contents/lockscreen/LockScreenUi.qml').read_bytes())
        self.assertFalse((self.live / shell.SHELL / 'contents/lockscreen/SequoiaClock.qml').exists())
        self.install('plasma-style')
        self.remove('plasma-style')
        self.assertFalse(payload_files(self.live / shell.SHELL))

    def test_custom_shell_and_changed_upstream_are_rejected(self):
        (self.live / shell.SHELL).mkdir(parents=True)
        with self.assertRaisesRegex(RuntimeError, 'custom Plasma shell'):
            self.install('plasma-style')
        with self.assertRaisesRegex(RuntimeError, 'CompactApplet has changed'):
            shell.patch_launcher('unrecognized upstream')

    def test_modified_shell_stops_before_removal(self):
        self.install('plasma-style', 'lockscreen')
        path = self.live / shell.SHELL / shell.COMPACT
        path.write_text(path.read_text() + '\n// local edit\n')
        result = subprocess.run([sys.executable, str(installer.ROOT / 'tools/install-theme.py'), 'uninstall',
                                 '--prefix', str(self.live), '--keep-system', '--components', 'lockscreen'], capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.live / shell.SHELL / shell.MARKERS['lockscreen']).exists())
        self.assertTrue(path.read_text().endswith('// local edit\n'))

if __name__ == '__main__':
    unittest.main()
