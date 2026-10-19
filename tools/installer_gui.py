"""Native Qt installation wizard; all installation runs in a separate process."""
import codecs
import os
from pathlib import Path
import sys

from PySide6.QtCore import QProcess, QProcessEnvironment, Qt
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (QApplication, QButtonGroup, QCheckBox, QFileDialog,
    QFrame, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QMainWindow, QMessageBox,
    QPlainTextEdit, QProgressBar, QPushButton, QRadioButton, QScrollArea,
    QStackedWidget, QVBoxLayout, QWidget)

from installer_platform import (APPS, COMPONENTS, DISTROS, OPTIONAL_COMPONENTS, detected_distro,
                                login_available, packages_for, resolve_components)

ROOT = Path(__file__).resolve().parents[1]

def label(text, size=None):
    widget = QLabel(text)
    widget.setWordWrap(True)
    widget.setTextFormat(Qt.PlainText)
    if size:
        font = widget.font()
        font.setPointSize(size)
        font.setBold(True)
        widget.setFont(font)
    return widget

class InstallerWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Sequoia Eleven • Installer')
        self.resize(900, 790)
        self.setMinimumSize(660, 580)
        self.process = None
        self.completed = []
        self.decoder = codecs.getincrementaldecoder('utf-8')('replace')
        self.detected = detected_distro()
        self.login_available = login_available()
        self.distro_buttons = {}
        self.component_boxes = {}
        self.individual_choices = {}
        self.app_boxes = {}
        outer = QWidget()
        self.setCentralWidget(outer)
        layout = QVBoxLayout(outer)
        layout.setContentsMargins(30, 24, 30, 24)
        layout.setSpacing(16)
        layout.addWidget(label('SEQUOIA ELEVEN', 11))
        self.steps = label('1  Distribution     /     2  Components     /     3  Review & install')
        layout.addWidget(self.steps)
        self.pages = QStackedWidget()
        layout.addWidget(self.pages, 1)
        self.make_distro_page()
        self.make_components_page()
        self.make_review_page()
        self.make_progress_page()
        footer = QHBoxLayout()
        self.back = QPushButton('Back')
        self.back.clicked.connect(self.go_back)
        footer.addWidget(self.back)
        footer.addStretch()
        self.next = QPushButton('Continue')
        self.next.setDefault(True)
        self.next.clicked.connect(self.advance)
        footer.addWidget(self.next)
        layout.addLayout(footer)
        self.sync()

    def page(self, title, subtitle):
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(0, 4, 0, 0)
        layout.setSpacing(14)
        layout.addWidget(label(title, 24))
        layout.addWidget(label(subtitle))
        self.pages.addWidget(widget)
        return layout

    def make_distro_page(self):
        layout = self.page('Which distribution do you use?',
                           'Choose the Linux distribution installed on this computer. We’ll use its package manager for dependencies.')
        group = QButtonGroup(self)
        for key, title in DISTROS.items():
            button = QRadioButton(title + ('  ·  Detected' if key == self.detected else ''))
            button.setMinimumHeight(58)
            group.addButton(button)
            self.distro_buttons[key] = button
            layout.addWidget(button)
            button.setChecked(key == self.detected)
            button.toggled.connect(self.sync)
        self.distro_note = label('')
        layout.addWidget(self.distro_note)
        layout.addStretch()
        layout.addWidget(label('Designed for KDE Plasma 6. Available components depend on your installed KDE and Qt versions.'))

    def make_components_page(self):
        layout = self.page('Make it yours', 'Install the complete desktop theme, choose individual components, or add an app makeover.')
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)
        content = QWidget()
        choices = QVBoxLayout(content)
        choices.setContentsMargins(0, 0, 8, 0)
        self.global_box = QCheckBox('Install the complete desktop')
        self.global_box.setFont(QFont(self.font().family(), 13, QFont.Bold))
        self.global_box.setChecked(True)
        self.global_box.toggled.connect(self.global_toggled)
        choices.addWidget(self.global_box)
        choices.addWidget(label('Includes the theme components below. The Animation kit and app makeovers are optional.'))
        theme = QGroupBox('Theme components')
        grid = QGridLayout(theme)
        for i, (key, (title, description)) in enumerate(COMPONENTS.items()):
            box = QCheckBox(title)
            box.setToolTip(description)
            box.setAccessibleDescription(description)
            box.toggled.connect(lambda checked, component=key: self.component_toggled(component, checked))
            self.component_boxes[key] = box
            self.individual_choices[key] = False
            box.blockSignals(True)
            box.setChecked(key not in OPTIONAL_COMPONENTS and (key != 'login' or self.login_available))
            box.blockSignals(False)
            grid.addWidget(box, i // 2, i % 2)
        choices.addWidget(theme)
        choices.addWidget(label(COMPONENTS['animation-kit'][1]))
        if not self.login_available:
            choices.addWidget(label('Login screen unavailable: this computer does not have Plasma Login Manager. '
                                    'It will be omitted from the Global Theme installation.'))
        apps = QGroupBox('App makeovers')
        app_layout = QVBoxLayout(apps)
        for key, (title, description) in APPS.items():
            box = QCheckBox(title)
            box.toggled.connect(self.sync)
            self.app_boxes[key] = box
            app_layout.addWidget(box)
            app_layout.addWidget(label(description))
        choices.addWidget(apps)
        choices.addWidget(label('Global Theme presets include their required styles, artwork and widgets. '
                               'You can also install widgets or individual styles on their own. '
                               'Choose your theme in System Settings and enable Desktop layout to use the top bar and dock.'))
        choices.addStretch()
        scroll.setWidget(content)
        layout.addWidget(scroll, 1)
        self.selection_note = label('')
        layout.addWidget(self.selection_note)

    def make_review_page(self):
        layout = self.page('Ready to install', 'Review your choices. Administrator permission is requested only when needed.')
        self.review = QPlainTextEdit()
        self.review.setReadOnly(True)
        self.review.setAccessibleName('Installation summary')
        layout.addWidget(self.review, 1)
        layout.addWidget(label('Native builds can take several minutes. Keep this window open until installation finishes. '
                               'App versions are checked before installation; incompatible makeovers stop the run.'))

    def make_progress_page(self):
        layout = self.page('Installing Sequoia Eleven', 'Follow progress below. You can save the log if you need help.')
        self.status = label('Preparing…', 14)
        layout.addWidget(self.status)
        self.progress = QProgressBar()
        self.progress.setRange(0, 0)
        layout.addWidget(self.progress)
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setAccessibleName('Installation log')
        layout.addWidget(self.log, 1)
        self.save = QPushButton('Save log…')
        self.save.clicked.connect(self.save_log)
        layout.addWidget(self.save, alignment=Qt.AlignLeft)

    def distro(self):
        return next((key for key, button in self.distro_buttons.items() if button.isChecked()), None)

    def global_toggled(self, checked):
        self.sync()

    def component_toggled(self, key, checked):
        self.individual_choices[key] = checked
        self.sync()

    def requested_components(self):
        return [key for key in COMPONENTS
                      if ((self.global_box.isChecked() and key not in OPTIONAL_COMPONENTS) or self.individual_choices[key])
                      and (key != 'login' or self.login_available)]

    def selection(self):
        return resolve_components(self.requested_components()), [key for key, box in self.app_boxes.items() if box.isChecked()]

    def sync(self):
        if not hasattr(self, 'next'):
            return
        page = self.pages.currentIndex()
        components, apps = self.selection()
        required = set(components) - set(self.requested_components())
        for key, box in self.component_boxes.items():
            box.blockSignals(True)
            box.setChecked(key in components)
            box.blockSignals(False)
            box.setEnabled((not self.global_box.isChecked() or key in OPTIONAL_COMPONENTS)
                           and key not in required and (key != 'login' or self.login_available))
        distro = self.distro()
        matches = distro == self.detected
        self.distro_note.setText('' if matches else 'Choose the distribution detected on this computer. '
                                f'Detected: {DISTROS.get(self.detected, self.detected or "unknown")}.')
        self.selection_note.setText(f'{len(components)} theme components · {len(apps)} app makeovers'
                                    if components or apps else 'Select at least one component or app makeover to continue.')
        if required:
            self.selection_note.setText(self.selection_note.text() + '\nRequired by your selection: '
                                        + ', '.join(COMPONENTS[c][0] for c in components if c in required))
        self.back.setVisible(page in (1, 2))
        self.next.setText('Install' if page == 2 else 'Continue')
        self.next.setEnabled(bool(distro and matches) if page == 0 else bool(components or apps))

    def go_back(self):
        self.pages.setCurrentIndex(self.pages.currentIndex() - 1)
        self.sync()

    def advance(self):
        page = self.pages.currentIndex()
        if page == 3:
            self.close()
            return
        if page == 2:
            self.start_install()
            return
        self.pages.setCurrentIndex(page + 1)
        if page == 1:
            components, apps = self.selection()
            summary = [DISTROS[self.distro()], '', 'Theme components:']
            summary += ['  • ' + COMPONENTS[key][0] for key in components] or ['  None']
            required = set(components) - set(self.requested_components())
            if required:
                summary += ['', 'Included for your selected components: '
                            + ', '.join(COMPONENTS[c][0] for c in components if c in required)]
            summary += ['', 'App makeovers:']
            summary += ['  • ' + APPS[key][0] + ' — ' + APPS[key][1] for key in apps] or ['  None']
            summary += ['', 'Dependencies (only missing packages will be installed):',
                        ', '.join(packages_for(self.distro(), components, apps)) or 'None', '',
                        'Theme files install for your account. Native plugins and login assets install system-wide.',
                        ('Animation kit: saves a restorable snapshot, applies Your palette and restarts Plasma. Panel layout is preserved.'
                         if 'animation-kit' in components else 'Existing components are preserved. Desktop settings and panel layout stay as they are.'),
                        'Remove themes or app makeovers with ./uninstall-gui.py.']
            if not self.login_available and self.global_box.isChecked():
                summary += ['', 'Login screen omitted: Plasma Login Manager is not installed.']
            self.review.setPlainText('\n'.join(summary))
        self.sync()

    def start_install(self):
        components, apps = self.selection()
        self.pages.setCurrentIndex(3)
        self.back.hide()
        self.next.setEnabled(False)
        self.next.setText('Installing…')
        self.steps.setText('Installation in progress')
        self.process = QProcess(self)
        self.process.setProcessChannelMode(QProcess.MergedChannels)
        environment = QProcessEnvironment.systemEnvironment()
        environment.insert('SEQUOIA_GUI', '1')
        environment.insert('SUDO_ASKPASS', str(ROOT / 'tools/installer-askpass.py'))
        environment.insert('PYTHONUNBUFFERED', '1')
        self.process.setProcessEnvironment(environment)
        self.process.setWorkingDirectory(str(ROOT))
        self.process.readyReadStandardOutput.connect(self.read_output)
        self.process.finished.connect(self.finished)
        self.process.errorOccurred.connect(self.process_error)
        self.process.start('/usr/bin/python3', ['-u', str(ROOT / 'tools/installer_run.py'),
                            '--distro', self.distro(), '--components', *components, '--apps', *apps])

    def read_output(self):
        text = self.decoder.decode(bytes(self.process.readAllStandardOutput()))
        cursor = self.log.textCursor()
        cursor.movePosition(cursor.MoveOperation.End)
        cursor.insertText(text)
        self.log.setTextCursor(cursor)
        # Scan the complete log so split process-output lines are handled too.
        for line in self.log.toPlainText().splitlines():
            if line.startswith('STEP: '):
                self.status.setText(line[6:])
            if line.startswith('COMPLETED: ') and line[11:] not in self.completed:
                self.completed.append(line[11:])

    def process_error(self, error):
        if error == QProcess.FailedToStart:
            self.log.appendPlainText(self.process.errorString())
            self.finished(1, QProcess.CrashExit)

    def finished(self, code, exit_status):
        self.read_output()
        success = code == 0 and exit_status == QProcess.NormalExit
        self.progress.setRange(0, 1)
        self.progress.setValue(1 if success else 0)
        if success:
            self.status.setText('Installation complete')
            self.log.appendPlainText('\nSelect installed theme components in System Settings. '
                                     'Close and reopen apps to load their makeovers.'
                                     + (' Reboot to load the login screen.' if 'login' in self.selection()[0] else ''))
            if 'settings' in self.selection()[1]:
                self.log.appendPlainText('Open “System Settings (Sequoia Eleven)” from the application menu.')
        else:
            self.status.setText('Installation stopped — see the details below')
            self.log.appendPlainText('\nCompleted: ' + (', '.join(self.completed) or 'No complete installation steps.')
                                     + '\nEarlier completed steps and installed dependencies remain. '
                                     'Fix the reported issue and reopen the installer to retry.')
        self.steps.setText('Finished' if success else 'Needs attention')
        self.next.setText('Close')
        self.next.setEnabled(True)

    def save_log(self):
        filename, _ = QFileDialog.getSaveFileName(self, 'Save installation log', 'sequoia-install.log', 'Log files (*.log)')
        if filename:
            try:
                Path(filename).write_text(self.log.toPlainText())
            except OSError as error:
                QMessageBox.warning(self, 'Could not save log', str(error))

    def closeEvent(self, event):
        if self.process and self.process.state() != QProcess.NotRunning:
            QMessageBox.information(self, 'Installation is running',
                                    'Wait for installation to finish before closing this window.')
            event.ignore()
        else:
            event.accept()

def main():
    app = QApplication(sys.argv)
    app.setApplicationName('Sequoia Eleven Installer')
    if os.geteuid() == 0:
        QMessageBox.critical(None, 'Open as your desktop user',
                             'Start the installer without sudo. It requests administrator permission when needed.')
        sys.exit(1)
    window = InstallerWindow()
    window.show()
    sys.exit(app.exec())
