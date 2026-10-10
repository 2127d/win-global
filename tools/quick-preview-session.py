#!/usr/bin/env python3
"""Run a preview and stop only its isolated Akonadi instance before bus exit."""
import os
import subprocess
import sys

try:
    result = subprocess.run(sys.argv[1:])
finally:
    instance = os.environ.get('AKONADI_INSTANCE', '')
    if instance.startswith('sequoia-preview-'):
        subprocess.run(['akonadictl', '--instance', instance, '--wait', 'stop'], timeout=25)
raise SystemExit(result.returncode)
