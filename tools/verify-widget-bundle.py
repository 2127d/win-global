#!/usr/bin/env python3
"""Compatibility entry point for the current archive-based widget checks."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--bundle', type=Path, default=ROOT / '.staging/widget-archives')
parser.add_argument('--native', action='store_true')
parser.add_argument('--functional', action='store_true')
args = parser.parse_args()
command = [sys.executable, str(ROOT / 'tools/verify-widget-archives.py'), '--folder', str(args.bundle)]
if args.native: command.append('--native')
if args.functional: command.append('--functional')
raise SystemExit(subprocess.call(command))
