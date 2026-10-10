#!/usr/bin/env python3
"""Headless wizard, distro dispatch, preflight and process-failure checks. No sudo."""
import os
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'

from pathlib import Path
import importlib.util
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from PySide6.QtCore import QProcess
from PySide6.QtWidgets import QApplication
import installer_gui as gui
import installer_platform as platform
import installer_run as runner

APP = QApplication([])

class WizardTests(unittest.TestCase):
    def setUp(self):
        with patch.object(gui, 'detected_distro', return_value='fedora'):
            self.window = gui.InstallerWindow()
        self.addCleanup(self.window.close)

    def test_distribution_and_global_exclusivity(self):
        window = self.window
        self.assertEqual(set(window.distro_buttons), {'arch', 'fedora', 'ubuntu', 'debian'})
        self.assertEqual(window.pages.currentIndex(), 0)
        window.distro_buttons['arch'].setChecked(True)
        self.assertFalse(window.next.isEnabled())
        window.distro_buttons['fedora'].setChecked(True)
        window.advance()
        self.assertEqual(window.pages.currentIndex(), 1)
        self.assertTrue(all(not box.isEnabled() for box in window.component_boxes.values()))
        self.assertTrue(all(box.isEnabled() for box in window.app_boxes.values()))
        window.global_box.setChecked(False)
        self.assertFalse(window.next.isEnabled())
        for name in ('icons', 'colors'):
            window.component_boxes[name].setChecked(True)
        window.global_box.setChecked(True)
        window.global_box.setChecked(False)
        self.assertEqual(set(window.selection()[0]), {'icons', 'colors'})
        window.app_boxes['settings'].setChecked(True)
        self.assertEqual(window.selection()[1], ['settings'])
        window.advance()
        self.assertIn('Settings makeover', window.review.toPlainText())
        self.assertIn('Fedora', window.review.toPlainText())
        window.go_back()
        self.assertEqual(set(window.selection()[0]), {'icons', 'colors'})

    def test_apps_only(self):
        self.window.advance()
        self.window.global_box.setChecked(False)
        self.window.app_boxes['gwenview'].setChecked(True)
        self.assertTrue(self.window.next.isEnabled())
        self.assertEqual(self.window.selection(), ([], ['gwenview']))

    def test_widgets_only_and_preset_dependencies(self):
        window = self.window
        window.advance()
        window.global_box.setChecked(False)
        window.component_boxes['widgets'].setChecked(True)
        self.assertEqual(window.selection(), (['widgets'], []))
        window.component_boxes['global-theme'].setChecked(True)
        for component in platform.COMPONENT_REQUIRES['global-theme']:
            self.assertTrue(window.component_boxes[component].isChecked())
        self.assertFalse(window.component_boxes['icons'].isEnabled())
        self.assertNotIn('login', window.selection()[0])
        self.assertNotIn('quick-style', window.selection()[0])
        window.advance()
        self.assertIn('Included for a working Global Theme', window.review.toPlainText())
        window.go_back()
        window.component_boxes['global-theme'].setChecked(False)
        self.assertEqual(window.selection(), (['widgets'], []))
        self.assertTrue(window.component_boxes['icons'].isEnabled())

    def test_unavailable_login_is_omitted(self):
        self.window.login_available = False
        self.window.sync()
        self.assertNotIn('login', self.window.selection()[0])
        self.window.advance()
        self.window.advance()
        self.assertIn('Login screen omitted', self.window.review.toPlainText())

    def test_launch_uses_selected_options_and_askpass(self):
        self.window.global_box.setChecked(False)
        self.window.component_boxes['colors'].setChecked(True)
        self.window.app_boxes['gwenview'].setChecked(True)
        with patch.object(gui.QProcess, 'start') as start:
            self.window.start_install()
            command, args = start.call_args.args
        self.assertEqual(command, '/usr/bin/python3')
        self.assertEqual(args[-6:], ['--distro', 'fedora', '--components', 'colors', '--apps', 'gwenview'])
        self.assertEqual(self.window.process.processEnvironment().value('SEQUOIA_GUI'), '1')
        self.assertTrue(self.window.process.processEnvironment().value('SUDO_ASKPASS').endswith('installer-askpass.py'))
        self.assertFalse(self.window.next.isEnabled())

    def test_process_output_failure_and_success(self):
        with patch.object(gui.QProcess, 'start'):
            self.window.start_install()
        self.window.process.start('/usr/bin/python3', ['-c',
            'print("COMPLETED: Selected theme components"); print("STEP: App makeover"); raise SystemExit(2)'])
        self.assertTrue(self.window.process.waitForFinished(5000))
        self.assertIn('stopped', self.window.status.text())
        self.assertIn('Selected theme components', self.window.completed)
        self.assertIn('remain', self.window.log.toPlainText())
        self.assertTrue(self.window.next.isEnabled())
        self.window.finished(0, QProcess.NormalExit)
        self.assertEqual(self.window.status.text(), 'Installation complete')

    def test_failed_process_start(self):
        with patch.object(gui.QProcess, 'start'):
            self.window.start_install()
        self.window.process_error(QProcess.FailedToStart)
        self.assertIn('stopped', self.window.status.text())
        self.assertTrue(self.window.next.isEnabled())

class PlatformTests(unittest.TestCase):
    def test_build_jobs_detection_and_override(self):
        with patch.dict(os.environ, {}, clear=True), \
                patch.object(platform.os, 'sched_getaffinity', return_value=set(range(16))), \
                patch.object(platform.os, 'cpu_count', return_value=32):
            self.assertEqual(platform.build_jobs(), 16)
            with patch.dict(os.environ, BUILD_JOBS='3'):
                self.assertEqual(platform.build_jobs(), 3)
            for value in ('0', '-1', 'auto'):
                with patch.dict(os.environ, BUILD_JOBS=value):
                    with self.assertRaisesRegex(ValueError, 'positive integer'):
                        platform.build_jobs()
            with patch.object(platform.os, 'sched_getaffinity', side_effect=OSError):
                self.assertEqual(platform.build_jobs(), 32)
                with patch.object(platform.os, 'cpu_count', return_value=None):
                    self.assertEqual(platform.build_jobs(), 1)

    def test_app_builders_use_detected_jobs(self):
        for name, target in [('gwenview', 'gwenview'), ('settings-navigation', 'systemsettings')]:
            spec = importlib.util.spec_from_file_location('builder', gui.ROOT / f'tools/build-{name}.py')
            builder = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(builder)
            with patch.object(builder, 'installed_version', return_value=builder.VERSION), \
                    patch.dict(os.environ, {}, clear=True), \
                    patch.object(platform.os, 'sched_getaffinity', return_value=set(range(12))), \
                    patch.object(builder.subprocess, 'run') as run:
                builder.build(Path('/tmp/build'))
                self.assertEqual(run.call_args.args[0],
                                 ['cmake', '--build', '/tmp/build', '--target', target, '--parallel', '12'])

    def test_desktop_builder_uses_shared_job_count(self):
        with tempfile.TemporaryDirectory(prefix='sequoia-build-jobs-') as temporary:
            root = Path(temporary)
            (root / 'tools').mkdir()
            for name in ('build-desktop.sh', 'installer_platform.py'):
                shutil.copy2(gui.ROOT / 'tools' / name, root / 'tools' / name)
            fake_cmake = root / 'cmake'
            fake_cmake.write_text('#!/bin/sh\nprintf "%s\\n" "$*"\n')
            fake_cmake.chmod(0o755)
            for override in ('', '3'):
                env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ['PATH'], BUILD_JOBS=override)
                result = subprocess.run(['bash', str(root / 'tools/build-desktop.sh')],
                                        env=env, text=True, capture_output=True, check=True)
                with patch.dict(os.environ, BUILD_JOBS=override):
                    jobs = platform.build_jobs()
                self.assertIn(f'--build {root}/build/desktop --parallel {jobs}', result.stdout)

    def test_gwenview_annotation_dependencies_and_fedora_retry(self):
        packages = {'fedora': 'kcolorpicker-qt6-devel', 'arch': 'kcolorpicker',
                    'ubuntu': 'libkcolorpicker-qt6-dev', 'debian': 'libkcolorpicker-qt6-dev'}
        for distro, package in packages.items():
            self.assertIn(package, platform.packages_for(distro, [], ['gwenview']))
            self.assertNotIn(package, platform.packages_for(distro, [], ['settings']))
        # Reproduce the failed host: all previous dependencies are installed,
        # but the Qt 6 color-picker development package is absent.
        def query(args, **kwargs):
            return subprocess.CompletedProcess(args, int(args[-1] == packages['fedora']), '', '')
        with patch.object(platform, 'detected_distro', return_value='fedora'), \
                patch.object(platform.subprocess, 'run', side_effect=query), \
                patch.object(platform, 'run') as install, \
                patch.dict(os.environ, SEQUOIA_GUI='1'):
            platform.install_dependencies('fedora', [], ['gwenview'])
            install.assert_called_once_with(['sudo', '-A', 'dnf', 'install', '-y', packages['fedora']])

    def test_fresh_machine_defaults_and_dependency_closure(self):
        with patch.object(platform, 'login_available', return_value=False):
            self.assertNotIn('login', platform.default_components())
            self.assertIn('widgets', platform.default_components())
        with patch.object(platform, 'login_available', return_value=True):
            self.assertIn('login', platform.default_components())
        for component in platform.COMPONENTS:
            resolved = platform.resolve_components([component])
            self.assertEqual(platform.resolve_components(resolved), resolved)
            if component != 'global-theme':
                self.assertEqual(resolved, [component])
        self.assertEqual(platform.resolve_components([]), [])

    def test_widget_and_quick_runtime_dependencies(self):
        for distro in platform.DISTROS:
            widgets = platform.packages_for(distro, ['widgets'])
            self.assertIn('plasma-desktop', widgets)
            self.assertIn('kdeplasma-addons', widgets)
            self.assertIn('bluedevil', widgets)
            self.assertTrue(any('5compat' in p for p in widgets))
            self.assertTrue(any('print-manager' in p for p in widgets))
            preset = platform.packages_for(distro, ['global-theme'])
            self.assertTrue(set(widgets) <= set(preset))
            quick = platform.packages_for(distro, ['quick-style'])
            self.assertTrue(any('kcmutils' in p for p in quick))
            self.assertTrue(any('kirigami' in p for p in quick))

    def test_unavailable_login_stops_runner_before_dependencies(self):
        with patch.object(sys, 'argv', ['runner', '--distro', 'fedora', '--components', 'login']), \
                patch.object(runner, 'login_available', return_value=False), \
                patch.object(runner, 'install_dependencies') as deps:
            with self.assertRaisesRegex(RuntimeError, 'Plasma Login Manager'):
                runner.main()
            deps.assert_not_called()

    def test_application_package_versions(self):
        for distro, result in [('fedora', '26.08.1'), ('arch', 'gwenview 26.08.1-1\n'),
                              ('debian', '4:26.08.1-1+b1'), ('ubuntu', '4:26.08.1-0ubuntu1')]:
            with patch.object(platform, 'detected_distro', return_value=distro), \
                    patch.object(platform.subprocess, 'check_output', return_value=result) as query:
                self.assertEqual(platform.application_version('gwenview'), '26.08.1')
                if distro == 'fedora':
                    self.assertIn('-qf', query.call_args.args[0])
                    self.assertEqual(query.call_args.args[0][-1], '/usr/bin/gwenview')

    def test_package_dispatch_and_scope(self):
        for distro, manager in [('arch', 'pacman'), ('fedora', 'dnf'), ('ubuntu', 'apt-get'), ('debian', 'apt-get')]:
            self.assertEqual(platform.package_command(distro, ['example'])[1], manager)
            self.assertEqual(platform.packages_for(distro, ['colors', 'wallpaper']), [])
            self.assertIn('cmake', platform.packages_for(distro, ['application-style']))
            self.assertFalse(any('decoration' in p for p in platform.packages_for(distro, ['application-style'])))
            self.assertTrue(any('decoration' in p for p in platform.packages_for(distro, ['decorations'])))
            self.assertIn('binutils', platform.packages_for(distro, [], ['gwenview']))
        for distro in ('ubuntu', 'debian'):
            packages = platform.packages_for(distro, ['application-style'], ['settings'])
            self.assertIn('libkf6kcmutils-dev', packages)
            self.assertIn('libkf6kio-dev', packages)

    def test_wrong_distribution_never_invokes_package_manager(self):
        with patch.object(platform, 'detected_distro', return_value='fedora'), \
                patch.object(platform.subprocess, 'run') as run:
            with self.assertRaisesRegex(RuntimeError, 'does not match'):
                platform.install_dependencies('debian', ['colors'])
            run.assert_not_called()

    def test_debian_missing_packages_only(self):
        def query(args, **kwargs):
            return subprocess.CompletedProcess(args, 0, 'installed' if args[-1] == 'fontconfig' else 'config-files')
        with patch.object(platform, 'detected_distro', return_value='debian'), \
                patch.object(platform.subprocess, 'run', side_effect=query), \
                patch.object(platform, 'run') as install:
            platform.install_dependencies('debian', ['fonts', 'icons'])
            install.assert_called_once_with(['sudo', 'apt-get', 'install', '-y', 'gtk-update-icon-cache'])

    def test_paths_and_askpass(self):
        self.assertEqual(platform.native_layout('arch'), ('lib', 'lib/qt6/plugins'))
        with patch.object(platform.sysconfig, 'get_config_var', return_value='aarch64-linux-gnu'):
            self.assertEqual(platform.native_layout('debian'),
                             ('lib/aarch64-linux-gnu', 'lib/aarch64-linux-gnu/qt6/plugins'))
        with patch.dict(os.environ, SEQUOIA_GUI='1'):
            self.assertEqual(platform.sudo_command('dnf', 'install'), ['sudo', '-A', 'dnf', 'install'])

    def test_incompatible_apps_stop_before_dependencies(self):
        with patch.object(sys, 'argv', ['runner', '--distro', 'fedora', '--apps', 'settings']), \
                patch.object(runner, 'app_problem', return_value='Version mismatch'), \
                patch.object(runner, 'install_dependencies') as deps, patch.object(runner, 'run') as run:
            with self.assertRaisesRegex(RuntimeError, 'Version mismatch'):
                runner.main()
            deps.assert_not_called()
            run.assert_not_called()

    def test_plan_is_read_only_for_every_distribution(self):
        for distro in platform.DISTROS:
            with patch.object(sys, 'argv', ['runner', '--distro', distro, '--components', 'icons', '--plan']), \
                    patch.object(runner, 'install_dependencies') as deps, patch.object(runner, 'run') as run:
                runner.main()
                deps.assert_not_called()
                run.assert_not_called()

class AppInstallTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('SEQUOIA_APP_INSTALL_TEST') == '1',
                         'Set SEQUOIA_APP_INSTALL_TEST=1 to test existing app binaries in temporary destinations')
    def test_existing_binaries_install_refresh_remove(self):
        for app, script, options, desktop in (
            ('settings', 'install-settings-navigation.py', ['--binary', str(gui.ROOT / '.staging/settings-build/bin/systemsettings')],
             'org.sequoia.eleven.systemsettings.desktop'),
            ('gwenview', 'install-gwenview.py', ['--skip-build'], 'org.kde.gwenview.desktop'),
        ):
            with self.subTest(app=app), tempfile.TemporaryDirectory(prefix='sequoia-app-gui-test-') as temporary:
                base = Path(temporary)
                data, bins = base / 'data with spaces', base / 'bin with spaces'
                bins.mkdir()
                binary_name = 'systemsettings' if app == 'settings' else 'gwenview'
                launcher = bins / binary_name
                launcher.write_text('#!/bin/sh\necho original\n')
                launcher.chmod(0o755)
                command = ['/usr/bin/python3', str(gui.ROOT / 'tools' / script),
                           '--prefix', str(data), '--bin-prefix', str(bins)]
                for _ in range(2):
                    result = subprocess.run([*command, *options], capture_output=True, text=True, timeout=60)
                    self.assertEqual(result.returncode, 0, result.stderr)
                entry = data / 'applications' / desktop
                self.assertIn(str(launcher), entry.read_text())
                subprocess.run(['desktop-file-validate', str(entry)], check=True)
                subprocess.run([*command, '--remove'], capture_output=True, check=True, timeout=20)
                self.assertEqual(launcher.read_text(), '#!/bin/sh\necho original\n')
                self.assertFalse(entry.exists())

if __name__ == '__main__':
    unittest.main()
