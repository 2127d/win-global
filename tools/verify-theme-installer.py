#!/usr/bin/env python3
"""Test ownership, rollback, scope, and installer orchestration without sudo."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import theme_files
import installer_platform
from theme_files import Receipt, payload_files

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('theme_installer', ROOT / 'tools/install-theme.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class ThemeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='sequoia-install-test-')
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.live = self.base / 'guest data with spaces'
        self.stage = self.base / 'payload'
        self.stage.mkdir()

    def test_switch_refresh_preserves_receipt(self):
        indicator = 'sequoia-eleven/quick-islands/qml/org/kde/desktop/private/SwitchIndicator.qml'
        self.put(self.stage, indicator, 'previous indicator')
        unrelated = 'plasma/desktoptheme/sequoia-eleven-light/metadata.json'
        self.put(self.stage, unrelated, '{}')
        for mode in ('light', 'dark'):
            self.put(self.stage, f'plasma/desktoptheme/sequoia-eleven-{mode}/colors', 'previous colors')
        receipt = Receipt(self.live, installer.USER_STATE, installer.user_allowed)
        receipt.install(self.stage)
        command = ['python3', str(ROOT / 'tools/install-switches.py'), '--prefix', str(self.live)]
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        current = Receipt(self.live, installer.USER_STATE, installer.user_allowed)
        current.validate()
        self.assertIn(unrelated, current.data['files'])
        for mode in ('light', 'dark'):
            theme = f'plasma/desktoptheme/sequoia-eleven-{mode}'
            self.assertFalse((self.live / theme / 'colors').exists())
            self.assertEqual((self.live / theme / 'widgets/switch.svg').read_bytes(),
                             (ROOT / theme / 'widgets/switch.svg').read_bytes())
        self.assertEqual((self.live / indicator).read_bytes(),
                         (ROOT / 'application-style/quick/SwitchIndicator.qml').read_bytes())
        (self.live / indicator).write_text('user modification')
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Preserve local edits', result.stderr)
        current.uninstall(force=True)
        self.assertFalse((self.live / indicator).exists())

        native = 'usr/lib64/qt6/plugins/styles/sequoiaeleven6.so'
        if (ROOT / '.staging/desktop' / native).is_file():
            system = self.base / 'native-root'
            payload = self.base / 'native-payload'
            self.put(payload, native, 'previous plugin')
            widget = f'usr/{installer.PLUGINDIR}/plasma/applets/org.sequoia.weather.so'
            self.put(payload, widget, 'existing weather widget')
            system_receipt = Receipt(system, installer.SYSTEM_STATE, installer.system_allowed)
            system_receipt.install(payload)
            subprocess.run(['python3', str(ROOT / 'tools/install-switches.py'), '--native', '--prefix', str(system)],
                           check=True, stdout=subprocess.DEVNULL)
            system_receipt = Receipt(system, installer.SYSTEM_STATE, installer.system_allowed)
            system_receipt.validate()
            self.assertEqual((system / native).read_bytes(), (ROOT / '.staging/desktop' / native).read_bytes())
            self.assertEqual((system / widget).read_text(), 'existing weather widget')
            system_receipt.uninstall()
            self.assertFalse((system / native).exists())

    def put(self, root, name, text):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    def receipt(self):
        return Receipt(self.live, 'state/theme', lambda n: n.startswith('theme/'))

    def test_install_upgrade_uninstall_restores_originals(self):
        self.put(self.live, 'theme/original', 'before')
        self.put(self.live, 'unrelated/keep', 'personal')
        self.put(self.stage, 'theme/original', 'version 1')
        self.put(self.stage, 'theme/new', 'new')
        (self.stage / 'theme/alias').symlink_to('new')
        self.receipt().install(self.stage)
        self.put(self.live, 'theme/user-added', 'keep')
        self.put(self.stage, 'theme/original', 'version 2')
        (self.stage / 'theme/new').unlink()
        (self.stage / 'theme/alias').unlink()
        self.receipt().install(self.stage)
        self.assertFalse((self.live / 'theme/new').exists())
        self.assertFalse((self.live / 'theme/alias').is_symlink())
        self.receipt().uninstall()
        self.assertEqual((self.live / 'theme/original').read_text(), 'before')
        self.assertEqual((self.live / 'theme/user-added').read_text(), 'keep')
        self.assertEqual((self.live / 'unrelated/keep').read_text(), 'personal')
        self.assertFalse((self.live / 'state/theme').exists())
        self.receipt().uninstall()  # Repeating removal is harmless.

    def test_modified_files_fail_before_removal(self):
        self.put(self.stage, 'theme/a', 'a')
        self.put(self.stage, 'theme/b', 'b')
        self.receipt().install(self.stage)
        self.put(self.live, 'theme/b', 'edited')
        with self.assertRaisesRegex(RuntimeError, 'edited'):
            self.receipt().uninstall()
        self.assertEqual((self.live / 'theme/a').read_text(), 'a')
        self.receipt().uninstall(force=True)
        self.assertFalse((self.live / 'theme').exists())

    def test_login_wallpaper_upgrade(self):
        old_files = installer.SYSTEM_ALLOWED_FILES - installer.SYSTEM_FILES
        for name in old_files:
            self.put(self.stage, name, 'old wallpaper')
        Receipt(self.live, installer.SYSTEM_STATE, old_files.__contains__).install(self.stage)
        for name in old_files:
            (self.stage / name).unlink()
        forest = 'usr/local/share/sequoia-eleven/login-screen/assets/sequoia-forest.jpg'
        self.put(self.stage, forest, 'forest wallpaper')
        def receipt():
            return Receipt(self.live, installer.SYSTEM_STATE, installer.SYSTEM_ALLOWED_FILES.__contains__)
        receipt().install(self.stage)
        self.assertEqual((self.live / forest).read_text(), 'forest wallpaper')
        self.assertTrue(all(not (self.live / name).exists() for name in old_files))
        receipt().uninstall()
        self.assertFalse((self.live / forest).exists())

    def test_failed_update_rolls_back_and_preserves_receipt(self):
        self.put(self.stage, 'theme/a', 'version 1')
        self.receipt().install(self.stage)
        before = (self.live / 'state/theme/manifest.json').read_bytes()
        self.put(self.stage, 'theme/a', 'version 2')
        self.put(self.stage, 'theme/b', 'new')
        real_copy = theme_files.copy_file

        def fail(source, target):
            if source == self.stage / 'theme/b':
                raise OSError('simulated disk failure')
            real_copy(source, target)

        with patch.object(theme_files, 'copy_file', side_effect=fail):
            with self.assertRaisesRegex(OSError, 'simulated'):
                self.receipt().install(self.stage)
        self.assertEqual((self.live / 'theme/a').read_text(), 'version 1')
        self.assertFalse((self.live / 'theme/b').exists())
        self.assertEqual((self.live / 'state/theme/manifest.json').read_bytes(), before)
        self.receipt().uninstall()

    def test_adoption_removes_old_theme_but_restores_shared_launcher(self):
        self.put(self.live, 'theme/style', 'old manual installation')
        self.put(self.live, 'theme/obsolete', 'old unused file')
        self.put(self.live, 'theme/launcher', 'old wrapper')
        original = self.put(self.base, 'saved-launcher', 'original application')
        self.put(self.stage, 'theme/style', 'current style')
        self.put(self.stage, 'theme/launcher', 'current wrapper')
        self.receipt().install(self.stage, original_overrides={
            'theme/style': None, 'theme/launcher': original}, obsolete=['theme/obsolete'])
        self.assertFalse((self.live / 'theme/obsolete').exists())
        self.receipt().install(self.stage)
        self.receipt().uninstall()
        self.assertFalse((self.live / 'theme/style').exists())
        self.assertEqual((self.live / 'theme/launcher').read_text(), 'original application')

    def test_symlinks_and_path_boundaries(self):
        external = self.put(self.base, 'outside', 'untouched')
        (self.live / 'theme').mkdir(parents=True)
        (self.live / 'theme/alias').symlink_to(external)
        self.put(self.stage, 'theme/alias', 'replacement')
        self.receipt().install(self.stage)
        self.assertEqual(external.read_text(), 'untouched')
        self.receipt().uninstall()
        self.assertTrue((self.live / 'theme/alias').is_symlink())
        outside_dir = self.base / 'outside-directory'
        outside_dir.mkdir()
        (self.live / 'theme/linked').symlink_to(outside_dir)
        self.put(self.stage, 'theme/linked/file', 'must not escape')
        with self.assertRaisesRegex(RuntimeError, 'symlink'):
            self.receipt().install(self.stage)
        self.assertFalse((outside_dir / 'file').exists())
        with self.assertRaises(RuntimeError):
            self.receipt().target('../outside')

    def test_scope_excludes_native_widget_forks_login_and_unrelated_files(self):
        for name in ('plasma/plasmoids/org.sequoia.systemtray/main.qml',
                     'systemd/user/plasma-plasmashell.service.d/widget.conf',
                     'sequoia-eleven/control-center/launch.py',
                     'applications/unrelated.desktop', 'etc/plasmalogin.conf'):
            self.assertFalse(installer.user_allowed(name), name)
            self.assertNotIn(name, installer.NATIVE_FILES)

    def test_entrypoints_and_unknown_distribution(self):
        for action in ('install', 'uninstall'):
            result = subprocess.run([sys.executable, str(ROOT / 'tools/install-theme.py'), action, '--plan'], cwd='/',
                                    capture_output=True, text=True, check=True)
            self.assertIn('custom tray, trash, animated task manager', result.stdout)
        with patch.object(installer.Path, 'read_text', return_value='ID=unsupported\n'):
            with self.assertRaisesRegex(RuntimeError, 'Arch, Fedora, Ubuntu or Debian'):
                installer.dependencies()

    def test_dependency_install_only_requests_missing_packages(self):
        class Result:
            returncode = 0

        def rpm(command, **kwargs):
            result = Result()
            result.returncode = int(command[-1] == 'cmake')
            return result

        with patch.object(installer.Path, 'read_text', return_value='ID=fedora\n'), \
                patch.object(installer.subprocess, 'run', side_effect=rpm), \
                patch.object(installer_platform, 'run') as install:
            installer.dependencies()
            install.assert_called_once_with(['sudo', 'dnf', 'install', '-y', 'cmake'])

    def test_component_refresh_retains_other_components_and_originals(self):
        self.put(self.live, 'theme/icons/one', 'original icons')
        self.put(self.stage, 'theme/icons/one', 'new icons')
        self.put(self.stage, 'theme/colors/old', 'old colors')
        self.receipt().install(self.stage)
        (self.stage / 'theme/icons/one').unlink()
        (self.stage / 'theme/colors/old').unlink()
        self.put(self.stage, 'theme/colors/new', 'new colors')
        self.receipt().install(self.stage, scope=lambda name: name.startswith('theme/colors/'))
        self.assertEqual((self.live / 'theme/icons/one').read_text(), 'new icons')
        self.assertFalse((self.live / 'theme/colors/old').exists())
        self.assertIn('theme/icons/one', self.receipt().data['files'])
        self.receipt().uninstall()
        self.assertEqual((self.live / 'theme/icons/one').read_text(), 'original icons')
        self.assertFalse((self.live / 'theme/colors/new').exists())

    def test_artwork_only_cli_install_add_component_and_remove(self):
        system = self.base / 'system'
        for components in [('colors',), ('plasma-style',)]:
            result = subprocess.run(['bash', str(ROOT / 'install.sh'), '--skip-deps',
                '--prefix', str(self.live), '--system-root', str(system), '--components', *components],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.live / 'color-schemes/SequoiaElevenLight.colors').exists())
        self.assertTrue((self.live / 'plasma/desktoptheme/sequoia-eleven-light/metadata.json').exists())
        self.assertFalse(system.exists(), 'Artwork-only installation must not install native files')
        self.assertFalse((self.live / 'icons').exists())
        subprocess.run([sys.executable, str(ROOT / 'tools/install-theme.py'), 'uninstall', '--prefix', str(self.live),
                        '--system-root', str(system)], check=True, capture_output=True)
        self.assertFalse((self.live / 'color-schemes/SequoiaElevenLight.colors').exists())

    def test_refresh_preserves_unselected_edits_but_protects_incoming(self):
        self.put(self.stage, 'theme/icons/one', 'icons')
        self.put(self.stage, 'theme/colors/one', 'colors')
        self.receipt().install(self.stage)
        self.put(self.live, 'theme/icons/one', 'custom icons')
        original_entry = self.receipt().data['files']['theme/icons/one'].copy()
        (self.stage / 'theme/icons/one').unlink()
        self.put(self.stage, 'theme/colors/one', 'updated colors')
        scope = lambda name: name.startswith('theme/colors/')
        self.receipt().install(self.stage, scope=scope)
        self.assertEqual((self.live / 'theme/icons/one').read_text(), 'custom icons')
        self.assertEqual(self.receipt().data['files']['theme/icons/one'], original_entry)
        self.assertEqual((self.live / 'theme/colors/one').read_text(), 'updated colors')
        # Incoming files are protected even outside the retirement scope.
        self.put(self.stage, 'theme/icons/one', 'replacement')
        with self.assertRaisesRegex(RuntimeError, 'edited'):
            self.receipt().install(self.stage, scope=scope)
        (self.stage / 'theme/icons/one').unlink()
        self.put(self.live, 'theme/colors/one', 'custom colors')
        with self.assertRaisesRegex(RuntimeError, 'edited'):
            self.receipt().install(self.stage, scope=scope)

    def test_cli_refresh_preserves_unselected_edits(self):
        unrelated = 'plasma/shells/org.kde.plasma.desktop/contents/lockscreen/SequoiaWallpaperFader.qml'
        self.put(self.stage, unrelated, 'installed lock screen')
        receipt = Receipt(self.live, installer.USER_STATE, installer.user_allowed)
        receipt.install(self.stage)
        self.put(self.live, unrelated, 'custom lock screen')
        original_entry = receipt.data['files'][unrelated].copy()
        result = subprocess.run([sys.executable, str(ROOT / 'tools/install-theme.py'),
            'install', '--skip-deps', '--prefix', str(self.live), '--components', 'colors'],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.live / unrelated).read_text(), 'custom lock screen')
        current = Receipt(self.live, installer.USER_STATE, installer.user_allowed)
        self.assertEqual(current.data['files'][unrelated], original_entry)
        self.assertTrue((self.live / 'color-schemes/SequoiaElevenLight.colors').exists())

    def test_widgets_are_independent_and_preserve_separate_installations(self):
        separate = self.put(self.live, 'plasma/plasmoids/org.kpple.kppleMenu/metadata.json', 'my widget')
        installer.stage_user(self.stage, self.live, {'widgets'})
        self.assertFalse((self.stage / 'plasma/look-and-feel').exists())
        self.assertFalse((self.stage / 'icons').exists())
        files = payload_files(self.stage)
        self.assertTrue(files)
        self.assertTrue(all(installer.component_for(name) == 'widgets' for name in files))
        obsolete = installer.legacy_files(self.live, {'widgets'})
        receipt = Receipt(self.live, installer.USER_STATE, installer.user_allowed)
        receipt.install(self.stage, obsolete=obsolete,
                        scope=lambda name: installer.component_for(name) == 'widgets')
        self.assertEqual(separate.read_text(), 'my widget')
        receipt.uninstall()
        self.assertEqual(separate.read_text(), 'my widget')
        self.assertFalse((self.live / 'plasma/plasmoids/org.sequoia.startmenu').exists())
        native = f'usr/{installer.PLUGINDIR}/plasma/applets/org.sequoia.systemtray.so'
        self.assertEqual(installer.component_for(native), 'widgets')

    def test_missing_selected_artwork_fails_instead_of_installing_partial_theme(self):
        with patch.object(installer, 'ROOT', self.base / 'incomplete-archive'):
            with self.assertRaisesRegex(RuntimeError, 'Missing installer source'):
                installer.stage_user(self.stage, self.live, {'plasma-style'})

    def test_cli_default_omits_unavailable_login_but_explicit_request_fails(self):
        with patch.object(installer_platform, 'login_available', return_value=False), \
                patch.object(installer, 'login_available', return_value=False), \
                patch.object(installer, 'dependencies') as deps, \
                patch.object(sys, 'argv', ['installer', 'install', '--plan']):
            installer.main()
            deps.assert_not_called()
        with patch.object(installer, 'login_available', return_value=False), \
                patch.object(installer, 'dependencies') as deps, \
                patch.object(sys, 'argv', ['installer', 'install', '--prefix', str(self.live),
                                          '--components', 'login']):
            with self.assertRaisesRegex(RuntimeError, 'Plasma Login Manager'):
                installer.main()
            deps.assert_not_called()


@unittest.skipUnless(os.environ.get('SEQUOIA_FULL_INSTALL_TEST') == '1',
                     'Set SEQUOIA_FULL_INSTALL_TEST=1 for real builds in temporary destinations')
class FullInstallationTest(unittest.TestCase):
    def test_native_style_only_install_refresh_remove(self):
        with tempfile.TemporaryDirectory(prefix='sequoia-style-install-') as temporary:
            base = Path(temporary)
            data, system = base / 'user data', base / 'system root'
            command = ['bash', str(ROOT / 'install.sh'), '--skip-deps', '--prefix', str(data),
                       '--system-root', str(system), '--components', 'application-style']
            result = subprocess.run(command, capture_output=True, text=True, timeout=300)
            self.assertEqual(result.returncode, 0, result.stdout[-1000:] + result.stderr[-3000:])
            receipt = Receipt(system, installer.SYSTEM_STATE, installer.system_allowed)
            self.assertEqual(set(receipt.data['files']), installer.selected_system_files({'application-style'}))
            self.assertFalse((data / 'plasma').exists())
            receipt.validate()
            subprocess.run([sys.executable, str(ROOT / 'tools/install-theme.py'), 'uninstall', '--prefix', str(data),
                            '--system-root', str(system)], check=True, capture_output=True)
            self.assertFalse((system / installer.SYSTEM_STATE).exists())

    def test_full_install_refresh_and_remove(self):
        with tempfile.TemporaryDirectory(prefix='sequoia-full-install-') as temporary:
            base = Path(temporary)
            data, system = base / 'user data', base / 'system root'
            env = dict(os.environ, HOME=str(base / 'home'), XDG_DATA_HOME=str(data),
                       XDG_CONFIG_HOME=str(base / 'config'), XDG_CACHE_HOME=str(base / 'cache'),
                       XDG_STATE_HOME=str(base / 'state'), QT_QPA_PLATFORM='offscreen',
                       QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
            for key in ('HOME', 'XDG_CONFIG_HOME', 'XDG_CACHE_HOME', 'XDG_STATE_HOME'):
                Path(env[key]).mkdir(parents=True)
            widget = data / 'plasma/plasmoids/org.example.keep/metadata.json'
            widget.parent.mkdir(parents=True)
            widget.write_text('keep widget')
            stale = data / 'icons/SequoiaTahoe/old-unused.svg'
            stale.parent.mkdir(parents=True)
            stale.write_text('obsolete manual theme file')
            launcher = data / 'applications/org.kde.plasma-systemmonitor.desktop'
            launcher.parent.mkdir(parents=True)
            original = '[Desktop Entry]\nName=Custom Monitor\nExec=plasma-systemmonitor --page-id history.page\nDBusActivatable=true\n'
            launcher.write_text(original)
            native_style = system / 'usr/lib64/qt6/plugins/styles/sequoiaeleven6.so'
            native_style.parent.mkdir(parents=True)
            native_style.write_text('old manually installed plugin')

            def run(action, *extra, success=True):
                result = subprocess.run([sys.executable, str(ROOT / 'tools/install-theme.py'), action, '--prefix', str(data),
                                         '--system-root', str(system), *extra], cwd=base, env=env,
                                        capture_output=True, text=True, timeout=300)
                self.assertEqual(result.returncode == 0, success, result.stdout[-1000:] + result.stderr[-4000:])

            run('install', '--skip-deps')
            for name in installer.widget_payload.REQUIRED:
                self.assertTrue((system / 'usr/share/plasma/plasmoids' / name / 'metadata.json').is_file(), name)
                self.assertTrue((system / 'usr' / installer.PLUGINDIR / 'plasma/applets' / (name + '.so')).is_file(), name)
            for name in installer.LAYOUT_WIDGETS:
                self.assertTrue((data / 'plasma/plasmoids' / name / 'contents/ui/main.qml').is_file(), name)
            self.assertFalse(stale.exists())
            self.assertTrue((data / 'plasma/shells/org.kde.plasma.desktop/.sequoia-eleven-lockscreen').is_file())
            self.assertTrue((data / 'sequoia-eleven/quick-islands/qml/org/sequoia/eleven/Island.qml').is_file())
            self.assertIn(str(data / 'sequoia-eleven/quick-islands/sequoia-quick'), launcher.read_text())
            self.assertIn('history.page', launcher.read_text())
            self.assertIn('Private.SwitchIndicator', (data / 'sequoia-eleven/quick-islands/qml/org/kde/desktop/MenuItem.qml').read_text())
            self.assertIn('sequoiaSwitchTrack', (data / 'sequoia-eleven/quick-islands/qml/org/kde/desktop/private/SwitchIndicator.qml').read_text())
            for mode in ('light', 'dark'):
                theme = data / f'plasma/desktoptheme/sequoia-eleven-{mode}'
                self.assertTrue((theme / 'widgets/switch.svg').is_file())
                self.assertFalse((theme / 'colors').exists())
            self.assertFalse((system / 'etc/plasmalogin.conf').exists())
            for name in installer.LOGIN_FILES:
                self.assertTrue((system / name).is_file(), name)
            run('install', '--skip-deps')
            manifest = json.loads((system / installer.SYSTEM_STATE / 'manifest.json').read_text())
            changed = data / 'plasma/desktoptheme/sequoia-eleven-light/widgets/switch.svg'
            changed.write_text(changed.read_text() + '\n# user edit\n')
            run('uninstall', success=False)
            self.assertTrue(native_style.exists(), 'Preflight must happen before removal')
            run('uninstall', '--force')
            self.assertEqual(launcher.read_text(), original)
            self.assertEqual(widget.read_text(), 'keep widget')
            for tree in installer.USER_TREES:
                self.assertFalse(any(p.is_file() or p.is_symlink() for p in (data / tree).rglob('*')), tree)
            for name in manifest['files']:
                self.assertFalse((system / name).exists(), name)
            self.assertFalse((data / installer.USER_STATE).exists())
            self.assertFalse((system / installer.SYSTEM_STATE).exists())


if __name__ == '__main__':
    unittest.main()
