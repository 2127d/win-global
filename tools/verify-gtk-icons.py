#!/usr/bin/env python3
"""Exercise GTK icon activation/restoration using isolated config and fake D-Bus."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import gtk_icons
import installer_platform

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('gtk_theme_installer', ROOT / 'tools/install-theme.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class GtkIconsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='gtk icons test ')
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.data, self.config = self.base / 'data', self.base / 'config'
        self.config.mkdir()
        self.env = patch.dict(os.environ, XDG_DATA_HOME=str(self.data), XDG_CONFIG_HOME=str(self.config))
        self.env.start(); self.addCleanup(self.env.stop)
        self.icon = "'OriginalIcons'"
        self.loaded, self.autoload = True, True
        self.events = []
        self.fail_set = False
        self.bus = True
        for method, replacement in [('command', self.command), ('kded', self.kded),
                                    ('reload_xsettings', lambda: self.events.append('reload'))]:
            mock = patch.object(gtk_icons, method, side_effect=replacement)
            mock.start(); self.addCleanup(mock.stop)

    def command(self, *args):
        self.assertEqual(args[0], 'gsettings')
        self.assertEqual(args[2:4], (gtk_icons.SCHEMA, gtk_icons.KEY))
        if args[1] == 'get': return self.icon
        if self.fail_set:
            self.fail_set = False
            raise subprocess.CalledProcessError(1, args)
        self.icon = args[4]
        self.events.append(('icon', self.icon))
        return ''

    def kded(self, method, *args):
        if not self.bus: raise subprocess.CalledProcessError(1, method)
        self.events.append(method)
        if method == 'loadedModules': return "(['gtkconfig'],)" if self.loaded else '([],)'
        if method == 'isModuleAutoloaded': return '(true,)' if self.autoload else '(false,)'
        if method == 'setModuleAutoloading': self.autoload = args[1] == 'true'
        if method == 'unloadModule': self.loaded = False
        if method == 'loadModule': self.loaded = True
        return '(true,)'

    def seed(self):
        (self.config / 'kded5rc').write_text('[Module-other]\nautoload=true\n')
        for version in (3, 4):
            p = self.config / f'gtk-{version}.0/settings.ini'
            p.parent.mkdir()
            p.write_text('[Settings]\ngtk-icon-theme-name=OriginalIcons\ngtk-font-name=My Font 12\n')
        p = self.config / gtk_icons.XSETTINGS
        p.parent.mkdir()
        p.write_text('Net/IconThemeName "OriginalIcons"\nGtk/FontName "My Font 12"\n')
        (self.config / 'kdeglobals').write_text('[Icons]\nTheme=SequoiaTahoe-light\n')

    def test_fresh_install_refresh_and_removal(self):
        self.seed()
        before = gtk_icons.read_values()
        gtk_icons.activate(self.data)
        saved = gtk_icons.state_path(self.data).read_bytes()
        self.assertEqual(gtk_icons.read_values(), gtk_icons.desired_values())
        self.assertEqual(self.icon, "'Adwaita'")
        self.assertFalse(self.loaded); self.assertFalse(self.autoload)
        self.assertLess(self.events.index('unloadModule'), self.events.index(('icon', "'Adwaita'")))
        self.assertIn('My Font 12', (self.config / 'gtk-4.0/settings.ini').read_text())
        self.assertIn('SequoiaTahoe-light', (self.config / 'kdeglobals').read_text())
        gtk_icons.activate(self.data)
        self.assertEqual(gtk_icons.state_path(self.data).read_bytes(), saved)
        gtk_icons.restore(self.data)
        self.assertEqual(gtk_icons.read_values(), before)
        self.assertEqual(self.icon, "'OriginalIcons'")
        self.assertTrue(self.loaded); self.assertTrue(self.autoload)
        self.assertFalse(gtk_icons.state_path(self.data).exists())
        gtk_icons.restore(self.data)

    def test_missing_settings_and_no_running_kde(self):
        self.bus = False
        gtk_icons.activate(self.data)
        self.assertEqual(gtk_icons.read_values(), gtk_icons.desired_values())
        self.assertEqual(self.icon, "'Adwaita'")
        gtk_icons.restore(self.data)
        self.assertEqual(set(gtk_icons.read_values().values()), {None})

    def test_later_user_edits_preserved_on_removal(self):
        self.seed(); gtk_icons.activate(self.data)
        values = gtk_icons.read_values()
        values['gtk-4.0/settings.ini'] = 'LaterTheme'
        values['kded5rc'] = 'true'
        gtk_icons.write_values(values)
        self.icon = "'LaterTheme'"
        gtk_icons.restore(self.data)
        self.assertEqual(gtk_icons.read_values()['gtk-4.0/settings.ini'], 'LaterTheme')
        self.assertEqual(gtk_icons.read_values()['kded5rc'], 'true')
        self.assertEqual(self.icon, "'LaterTheme'")
        self.assertNotIn('loadModule', self.events)

    def test_user_icon_choice_does_not_restart_sync_writer(self):
        self.seed(); gtk_icons.activate(self.data)
        self.icon = "'UserChosenIcons'"
        gtk_icons.restore(self.data)
        self.assertEqual(self.icon, "'UserChosenIcons'")
        self.assertEqual(gtk_icons.read_values()['kded5rc'], 'false')
        self.assertFalse(self.loaded); self.assertFalse(self.autoload)

    def test_activation_failure_rolls_back(self):
        self.seed(); before = gtk_icons.read_values()
        self.fail_set = True
        with self.assertRaises(subprocess.CalledProcessError): gtk_icons.activate(self.data)
        self.assertEqual(gtk_icons.read_values(), before)
        self.assertEqual(self.icon, "'OriginalIcons'")
        self.assertTrue(self.loaded); self.assertTrue(self.autoload)
        self.assertFalse(gtk_icons.state_path(self.data).exists())

    def test_symlink_config_rejected_before_changes(self):
        target = self.base / 'other-file'; target.write_text('[Settings]\n')
        p = self.config / 'gtk-4.0'; p.mkdir()
        (p / 'settings.ini').symlink_to(target)
        with self.assertRaisesRegex(RuntimeError, 'symlink'): gtk_icons.activate(self.data)
        self.assertTrue(self.loaded)
        self.assertEqual(self.icon, "'OriginalIcons'")

    def test_foreign_receipt_rejected(self):
        self.seed(); gtk_icons.activate(self.data)
        p = gtk_icons.state_path(self.data)
        state = json.loads(p.read_text()); state['config'] = '/other/config'
        p.write_text(json.dumps(state))
        with self.assertRaisesRegex(RuntimeError, 'receipt'): gtk_icons.activate(self.data)

    def test_installer_live_only_and_component_scope(self):
        for components, prefix, system, expected in [
                (['icons'], self.data, Path('/'), True),
                (['colors'], self.data, Path('/'), False),
                (['icons'], self.base / 'staged-data', Path('/'), False),
                (['icons'], self.data, self.base / 'staged-system', False)]:
            with self.subTest(components=components, prefix=prefix, system=system):
                args = SimpleNamespace(action='install', components=components, prefix=prefix,
                                       system_root=system, force=True)
                with patch.object(installer, 'stage_user', side_effect=lambda stage, *_: stage.mkdir()), \
                        patch.object(installer, 'refresh'), patch.object(gtk_icons, 'activate') as activate:
                    installer.perform(args)
                    self.assertEqual(activate.called, expected)

    def test_remove_check_does_not_restore(self):
        self.seed(); gtk_icons.activate(self.data)
        args = SimpleNamespace(action='uninstall', components=['icons'], prefix=self.data,
                               system_root=Path('/'), force=False, check=True, keep_system=True)
        with patch.object(installer, 'refresh'), patch.object(gtk_icons, 'restore') as restore:
            installer.perform(args); restore.assert_not_called()
            args.check = False
            installer.perform(args); restore.assert_called_once_with(self.data)

    def test_dependencies_for_fresh_install(self):
        for distro in installer_platform.DISTROS:
            packages = installer_platform.packages_for(distro, ['icons'])
            self.assertIn('adwaita-icon-theme', packages)
            self.assertIn('gsettings-desktop-schemas', packages)
            self.assertIn('libglib2.0-bin' if distro in ('ubuntu', 'debian') else 'glib2', packages)
        self.assertNotIn('adwaita-icon-theme', installer_platform.packages_for('fedora', ['colors']))


if __name__ == '__main__':
    unittest.main()
