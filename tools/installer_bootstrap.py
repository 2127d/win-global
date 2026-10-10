"""Bootstrap the shared Qt dependency for the installer and uninstaller."""
import shutil
import subprocess
import sys

def launch(module, entry, title):
    try:
        from PySide6 import QtWidgets
    except ImportError:
        from installer_platform import DISTROS, GUI_PACKAGES, detected_distro, package_command
        distro = detected_distro()
        if distro not in DISTROS or not shutil.which('kdialog') or not shutil.which('pkexec'):
            message = (f'{title} needs Python 3 with PySide6 QtWidgets.\n'
                       'Arch: sudo pacman -S pyside6\nFedora: sudo dnf install python3-pyside6\n'
                       'Ubuntu / Debian: sudo apt-get install python3-pyside6.qtwidgets\n'
                       'Then open this window again.')
            if shutil.which('kdialog'):
                subprocess.run(['kdialog', '--error', message])
            elif shutil.which('zenity'):
                subprocess.run(['zenity', '--error', '--text', message])
            sys.exit(message)
        accepted = subprocess.run(['kdialog', '--title', title, '--yesno',
            f'Install the graphical interface dependency for {DISTROS[distro]}?\n'
            + ', '.join(GUI_PACKAGES[distro]) + '\n\nYour desktop will ask for administrator permission.'])
        if accepted.returncode:
            return
        command = package_command(distro, GUI_PACKAGES[distro])[1:]
        result = subprocess.run(['pkexec', *command])
        if result.returncode:
            subprocess.run(['kdialog', '--error', 'Could not install the graphical interface dependency. '
                            'Check that your distribution release provides PySide6 QtWidgets.'])
            return
        # Start a fresh interpreter after the package transaction.
        import os
        os.execv('/usr/bin/python3', ['/usr/bin/python3', str(entry), *sys.argv[1:]])
    import importlib
    importlib.import_module(module).main()
