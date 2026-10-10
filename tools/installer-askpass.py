#!/usr/bin/python3
"""sudo askpass: the password goes only to sudo's private pipe, never the log."""
import sys
from PySide6.QtWidgets import QApplication, QInputDialog, QLineEdit

app = QApplication([])
password, accepted = QInputDialog.getText(
    None, 'Sequoia Eleven — administrator permission',
    'Enter your password to install dependencies or install/remove shared theme files.', QLineEdit.Password)
if not accepted:
    sys.exit(1)
print(password)
