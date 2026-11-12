#!/usr/bin/env python3
"""Compatibility entry point; the app package lives in sandbox/apps."""
import runpy
import sys
from pathlib import Path
entry=Path(__file__).resolve().parents[2]/'apps'/'sequoia-component-library'/'manage.py'
if not entry.is_file(): raise SystemExit('App package missing: '+str(entry))
sys.path.insert(0,str(entry.parent))
sys.argv[0]=str(entry)
runpy.run_path(str(entry),run_name='__main__')
