#!/usr/bin/env python3
"""Verify modal-effect ownership and key-scoped restoration without a live session."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import modal_dimming as modal
from theme_files import Receipt
from installer_platform import default_components, resolve_components, resolve_removals, packages_for

spec = importlib.util.spec_from_file_location('theme', Path(__file__).with_name('install-theme.py'))
theme = importlib.util.module_from_spec(spec); spec.loader.exec_module(theme)

class ModalInstallerTests(unittest.TestCase):
    def test_optional_independent(self):
        self.assertNotIn('modal-dimming', default_components())
        self.assertEqual(resolve_components(['modal-dimming']), ['modal-dimming'])
        self.assertEqual(resolve_removals(['modal-dimming']), ['modal-dimming'])
        self.assertIn('kwin-devel', packages_for('fedora', ['modal-dimming']))

    def test_plugin_receipt_restores_only_selected_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            base=Path(temporary); root=base/'system'; stage=base/'stage'
            name=modal.plugin_path(theme.PLUGINDIR)
            self.assertEqual(theme.component_for(name), 'modal-dimming')
            self.assertEqual(theme.selected_system_files({'modal-dimming'}), {name})
            for parent, text in ((root,'prior plugin'), (stage,'new plugin')):
                target=parent/name;target.parent.mkdir(parents=True);target.write_text(text)
            receipt=Receipt(root,theme.SYSTEM_STATE,theme.system_allowed)
            receipt.install(stage)
            (root/name).write_text('local edit')
            with self.assertRaises(RuntimeError): receipt.validate()
            (root/name).write_text('new plugin')
            receipt.uninstall(scope=lambda path: theme.component_for(path)=='modal-dimming')
            self.assertEqual((root/name).read_text(),'prior plugin')

    def test_reinstall_keeps_original_key_and_other_settings(self):
        with tempfile.TemporaryDirectory() as temporary:
            base=Path(temporary); config=base/'config';config.mkdir();data=base/'data'
            with patch.dict(os.environ, XDG_CONFIG_HOME=str(config)), patch.object(modal,'kwin',return_value='true'):
                (config/'kwinrc').write_text('[Plugins]\notherEffectEnabled=true\n'+modal.KEY+'=false\n')
                modal.activate(data);modal.activate(data)
                self.assertEqual(modal.read_key(),'true')
                with (config/'kwinrc').open('a') as stream: stream.write('\n[UserAdded]\nValue=preserved\n')
                modal.restore(data)
                self.assertEqual(modal.read_key(),'false')
                text=(config/'kwinrc').read_text()
                self.assertIn('otherEffectEnabled=true',text);self.assertIn('Value=preserved',text)
                self.assertFalse(modal.state_path(data).exists())

    def test_missing_key_and_later_user_disable(self):
        with tempfile.TemporaryDirectory() as temporary:
            base=Path(temporary);config=base/'config';config.mkdir();data=base/'data'
            with patch.dict(os.environ,XDG_CONFIG_HOME=str(config)),patch.object(modal,'kwin',return_value='true'):
                modal.activate(data);modal.restore(data)
                self.assertIsNone(modal.read_key())
                modal.activate(data);modal.write_key('false');modal.restore(data)
                self.assertEqual(modal.read_key(),'false')

    def test_failed_load_restores_key(self):
        with tempfile.TemporaryDirectory() as temporary:
            base=Path(temporary);config=base/'config';config.mkdir()
            with patch.dict(os.environ,XDG_CONFIG_HOME=str(config)),patch.object(modal,'kwin',return_value='false'):
                with self.assertRaises(RuntimeError):modal.activate(base/'data')
                self.assertIsNone(modal.read_key())

if __name__=='__main__':unittest.main()
