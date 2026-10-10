"""Start test services with private XDG paths before creating their D-Bus daemon."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def ensure_private_session():
    if os.environ.get('SEQUOIA_PRIVATE_PREVIEW'):
        return
    # Setting XDG paths only inside the test is too late: D-Bus activated
    # services inherit the daemon's environment and could overwrite live state.
    with tempfile.TemporaryDirectory(prefix='sequoia-preview-session-', ignore_cleanup_errors=True) as temporary:
        env = dict(os.environ, SEQUOIA_PRIVATE_PREVIEW=temporary)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE'):
            directory = Path(temporary) / kind.lower()
            directory.mkdir()
            env[f'XDG_{kind}_HOME'] = str(directory)
        result = subprocess.call(['dbus-run-session', '--', sys.executable, *sys.argv], env=env)
    raise SystemExit(result)
