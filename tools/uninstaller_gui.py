"""Graphical, selective removal of Sequoia Eleven installations."""
import argparse
import codecs
import os
from pathlib import Path
import sys

from PySide6.QtCore import QProcess, QProcessEnvironment, Qt
from PySide6.QtWidgets import (QApplication, QCheckBox, QFileDialog, QFrame,
    QGridLayout, QGroupBox, QHBoxLayout, QMainWindow, QMessageBox, QPlainTextEdit,
    QProgressBar, QPushButton, QScrollArea, QStackedWidget, QVBoxLayout, QWidget)

from installer_gui import label
from installer_platform import APPS, COMPONENTS, NATIVE, resolve_removals
from uninstaller_run import add_destinations

ROOT = Path(__file__).resolve().parents[1]


class UninstallerWindow(QMainWindow):
    def __init__(self, destinations):
        super().__init__()
        self.destinations = destinations
        self.process = None
        self.completed = []
        self.decoder = codecs.getincrementaldecoder('utf-8')('replace')
        self.choices = dict.fromkeys(COMPONENTS, False)
        self.component_boxes = {}
        self.app_boxes = {}
        self.setWindowTitle('Sequoia Eleven • Uninstaller')
        self.resize(900, 790)
        self.setMinimumSize(660, 580)
        outer = QWidget()
        self.setCentralWidget(outer)
        layout = QVBoxLayout(outer)
        layout.setContentsMargins(30, 24, 30, 24)
        layout.setSpacing(16)
        layout.addWidget(label('SEQUOIA ELEVEN', 11))
        self.steps = label('1  Choose what to remove     /     2  Review     /     3  Remove')
        layout.addWidget(self.steps)
        self.pages = QStackedWidget()
        layout.addWidget(self.pages, 1)
        self.make_choices()
        review_layout = self.page('Review removal', 'Only tracked files in the selected components will be removed. Saved originals will be restored.')
        self.review = QPlainTextEdit()
        self.review.setReadOnly(True)
        self.review.setAccessibleName('Removal summary')
        review_layout.addWidget(self.review, 1)
        progress_layout = self.page('Removing Sequoia Eleven', 'Keep this window open until removal finishes.')
        self.status = label('Preparing…', 14)
        progress_layout.addWidget(self.status)
        self.progress = QProgressBar()
        self.progress.setRange(0, 0)
        progress_layout.addWidget(self.progress)
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setAccessibleName('Removal log')
        progress_layout.addWidget(self.log, 1)
        save = QPushButton('Save log…')
        save.clicked.connect(self.save_log)
        progress_layout.addWidget(save, alignment=Qt.AlignLeft)
        footer = QHBoxLayout()
        self.back = QPushButton('Back')
        self.back.clicked.connect(self.go_back)
        footer.addWidget(self.back)
        footer.addStretch()
        self.next = QPushButton('Review removal')
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

    def make_choices(self):
        layout = self.page('Choose what to remove',
                           'Choose another theme and replace Sequoia panel widgets before removing components you currently use.')
        settings = QPushButton('Open Appearance Settings')
        settings.clicked.connect(self.open_settings)
        layout.addWidget(settings, alignment=Qt.AlignLeft)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)
        content = QWidget()
        choices = QVBoxLayout(content)
        self.all_theme = QCheckBox('Remove all theme components')
        self.all_theme.setChecked(True)
        self.all_theme.toggled.connect(self.sync)
        choices.addWidget(self.all_theme)
        theme = QGroupBox('Theme components')
        grid = QGridLayout(theme)
        for i, (key, (title, _)) in enumerate(COMPONENTS.items()):
            box = QCheckBox(title)
            box.toggled.connect(lambda checked, component=key: self.component_toggled(component, checked))
            self.component_boxes[key] = box
            grid.addWidget(box, i // 2, i % 2)
        choices.addWidget(theme)
        self.selection_note = label('')
        choices.addWidget(self.selection_note)
        choices.addWidget(label('Components that were never installed are skipped. Removing a preset asset also removes the Global Theme presets that require it.'))
        apps = QGroupBox('App makeovers')
        app_layout = QVBoxLayout(apps)
        for key, (title, _) in APPS.items():
            box = QCheckBox(title)
            box.toggled.connect(self.sync)
            self.app_boxes[key] = box
            app_layout.addWidget(box)
        choices.addWidget(apps)
        self.keep_system = QCheckBox('Keep shared system files for other accounts')
        self.keep_system.setToolTip('Retain native styles, decorations, native widgets and the login screen.')
        choices.addWidget(self.keep_system)
        self.force = QCheckBox('Also remove locally edited files in the selected components')
        choices.addWidget(self.force)
        choices.addWidget(label('Edited files normally stop removal so you can save your changes. This option discards those edits. Personal settings, separately installed widgets and distribution packages are retained.'))
        choices.addStretch()
        scroll.setWidget(content)
        layout.addWidget(scroll, 1)

    def component_toggled(self, key, checked):
        self.choices[key] = checked
        self.sync()

    def requested_components(self):
        return [c for c in COMPONENTS if self.all_theme.isChecked() or self.choices[c]]

    def selection(self):
        return resolve_removals(self.requested_components()), [a for a, box in self.app_boxes.items() if box.isChecked()]

    def sync(self):
        if not hasattr(self, 'next'):
            return
        components, apps = self.selection()
        required = set(components) - set(self.requested_components())
        for key, box in self.component_boxes.items():
            box.blockSignals(True)
            box.setChecked(key in components)
            box.blockSignals(False)
            box.setEnabled(not self.all_theme.isChecked() and key not in required)
        self.selection_note.setText('Global Theme presets will also be removed because they require a selected component.'
                                    if required else 'Select the components you want to remove.')
        self.back.setVisible(self.pages.currentIndex() == 1)
        self.next.setText('Remove selected components' if self.pages.currentIndex() == 1 else 'Review removal')
        self.next.setEnabled(bool(components or apps))

    def go_back(self):
        self.pages.setCurrentIndex(0)
        self.sync()

    def advance(self):
        if self.pages.currentIndex() == 2:
            self.close()
        elif self.pages.currentIndex() == 1:
            self.start_removal()
        else:
            components, apps = self.selection()
            shared = bool(set(components) & (NATIVE | {'widgets', 'login'}))
            lines = ['Theme components:'] + (['  • ' + COMPONENTS[c][0] for c in components] or ['  None'])
            lines += ['', 'App makeovers:'] + (['  • ' + APPS[a][0] for a in apps] or ['  None'])
            lines += ['', f'User data: {self.destinations.prefix.absolute()}',
                      f'Application launchers: {self.destinations.bin_prefix.absolute()}',
                      ('Shared system files will be kept.' if self.keep_system.isChecked() else
                       f'Selected shared system files will be removed from: {self.destinations.system_root.absolute()}' if shared else
                       'No shared system files are selected.'),
                      ('Locally edited selected files will be removed; their edits will be lost.' if self.force.isChecked() else
                       'Locally edited selected files will stop removal.'), '',
                      'Original files and launchers are restored where backups exist.',
                      'Components you keep retain their removal records and backups.',
                      'Personal settings, separately installed widgets and distribution packages are retained.',
                      'Choose another theme and replace Sequoia panel widgets before continuing.',
                      'Administrator permission is requested only for shared system files.']
            self.review.setPlainText('\n'.join(lines))
            if 'animation-kit' in components:
                self.review.appendPlainText('\nAnimation kit: restores its saved animation settings and files, then restarts Plasma. '
                                            'Later edits to managed animations stop restoration for review, even with force enabled.')
            self.pages.setCurrentIndex(1)
            self.sync()

    def start_removal(self):
        components, apps = self.selection()
        args = ['-u', str(ROOT / 'tools/uninstaller_run.py'), '--prefix', str(self.destinations.prefix),
                '--system-root', str(self.destinations.system_root), '--bin-prefix', str(self.destinations.bin_prefix),
                '--components', *components, '--apps', *apps]
        if self.keep_system.isChecked():
            args.append('--keep-system')
        if self.force.isChecked():
            args.append('--force')
        self.pages.setCurrentIndex(2)
        self.back.hide()
        self.next.setText('Removing…')
        self.next.setEnabled(False)
        self.steps.setText('Removal in progress')
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
        self.process.start('/usr/bin/python3', args)

    def read_output(self):
        text = self.decoder.decode(bytes(self.process.readAllStandardOutput()))
        cursor = self.log.textCursor()
        cursor.movePosition(cursor.MoveOperation.End)
        cursor.insertText(text)
        self.log.setTextCursor(cursor)
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
        self.progress.setValue(int(success))
        self.status.setText('Removal complete' if success else 'Removal stopped — see the details below')
        if success:
            self.log.appendPlainText('\nClose and reopen affected applications. Log out and back in to unload removed desktop plugins.'
                                     + (' Reboot to reload the login screen.' if 'login' in self.selection()[0] and not self.keep_system.isChecked() else ''))
        else:
            self.log.appendPlainText('\nCompleted: ' + (', '.join(self.completed) or 'No complete removal steps.')
                                     + '\nEarlier completed removals remain. Save any edits mentioned above and reopen the uninstaller to retry.')
        self.steps.setText('Finished' if success else 'Needs attention')
        self.next.setText('Close')
        self.next.setEnabled(True)

    def save_log(self):
        filename, _ = QFileDialog.getSaveFileName(self, 'Save removal log', 'sequoia-uninstall.log', 'Log files (*.log)')
        if filename:
            try:
                Path(filename).write_text(self.log.toPlainText())
            except OSError as error:
                QMessageBox.warning(self, 'Could not save log', str(error))

    def open_settings(self):
        started, _ = QProcess.startDetached('systemsettings', ['kcm_lookandfeel'])
        if not started:
            QMessageBox.warning(self, 'Could not open System Settings', 'Open System Settings → Colors & Themes and select another theme.')

    def closeEvent(self, event):
        if self.process and self.process.state() != QProcess.NotRunning:
            QMessageBox.information(self, 'Removal is running', 'Wait for removal to finish before closing this window.')
            event.ignore()
        else:
            event.accept()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    add_destinations(parser)
    args = parser.parse_args()
    app = QApplication([sys.argv[0]])
    app.setApplicationName('Sequoia Eleven Uninstaller')
    app.setStyle('Fusion')
    if os.geteuid() == 0:
        QMessageBox.critical(None, 'Open as your desktop user',
                             'Start the uninstaller without sudo. It requests administrator permission when needed.')
        sys.exit(1)
    window = UninstallerWindow(args)
    window.show()
    sys.exit(app.exec())
