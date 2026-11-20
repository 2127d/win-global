#!/usr/bin/env python3
"""Check native global-menu ownership during widget upgrades."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import global_menu
import widget_payload


class GlobalMenuInstallation(unittest.TestCase):
    def test_only_the_named_native_override_is_allowed(self):
        prefix = 'usr/lib64/qt6/plugins/plasma/applets/'
        self.assertTrue(widget_payload.allowed(prefix + global_menu.PLUGIN, 'lib64/qt6/plugins'))
        self.assertFalse(widget_payload.allowed(prefix + 'org.kde.plasma.other.so', 'lib64/qt6/plugins'))

    def test_upgrade_preserves_a_backup_and_retires_owned_shadow_copy(self):
        with tempfile.TemporaryDirectory() as folder:
            data = Path(folder)
            target = data / global_menu.USER_PLUGIN
            target.parent.mkdir(parents=True)
            target.write_bytes(b'owned applet')
            receipt = data / global_menu.USER_RECEIPT
            receipt.write_text(json.dumps({'sha256': hashlib.sha256(target.read_bytes()).hexdigest()}))
            global_menu.retire_user_override(data)
            self.assertFalse(target.exists())
            self.assertFalse(receipt.exists())
            backups = list(receipt.parent.glob('global-menu-upgraded-*'))
            self.assertEqual(len(backups), 1)
            self.assertEqual((backups[0] / global_menu.PLUGIN).read_bytes(), b'owned applet')

    def test_upgrade_preserves_a_modified_shadow_copy(self):
        with tempfile.TemporaryDirectory() as folder:
            data = Path(folder)
            target = data / global_menu.USER_PLUGIN
            target.parent.mkdir(parents=True)
            target.write_bytes(b'local modification')
            receipt = data / global_menu.USER_RECEIPT
            receipt.write_text(json.dumps({'sha256': 'different installed version'}))
            with self.assertRaises(RuntimeError):
                global_menu.retire_user_override(data)
            self.assertEqual(target.read_bytes(), b'local modification')
            self.assertTrue(receipt.exists())


if __name__ == '__main__':
    unittest.main()
