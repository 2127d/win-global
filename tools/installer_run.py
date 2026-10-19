#!/usr/bin/env python3
"""Execute a reviewed GUI selection using the existing reversible installers."""
import argparse
from pathlib import Path
import subprocess
import sys

from installer_platform import (APPS, COMPONENTS, DISTROS, app_problem,
                                install_dependencies, login_available, packages_for, resolve_components, run)

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--distro', required=True, choices=DISTROS)
    parser.add_argument('--components', nargs='*', choices=COMPONENTS, default=[])
    parser.add_argument('--apps', nargs='*', choices=APPS, default=[])
    parser.add_argument('--plan', action='store_true')
    args = parser.parse_args()
    if not args.components and not args.apps:
        parser.error('Select at least one theme component or app makeover.')
    args.components = resolve_components(args.components)
    if args.plan:
        print('Distribution:', DISTROS[args.distro])
        print('Components:', ', '.join(args.components) or 'None')
        print('App makeovers:', ', '.join(args.apps) or 'None')
        print('Dependencies:', ' '.join(packages_for(args.distro, args.components, args.apps)) or 'None')
        print('Final step: Restart Plasma shell')
        return
    # Reject incompatible forks before any packages or theme files are changed.
    if 'login' in args.components and not login_available():
        raise RuntimeError('The login theme requires Plasma Login Manager. Deselect the login screen on systems using SDDM.')
    for app in args.apps:
        problem = app_problem(app)
        if problem:
            raise RuntimeError(problem)
    print('STEP: Installing dependencies', flush=True)
    install_dependencies(args.distro, args.components, args.apps)
    for app in args.apps:
        problem = app_problem(app)
        if problem:
            raise RuntimeError(problem)
    if args.components:
        print('STEP: Building and installing theme components', flush=True)
        run([sys.executable, '-u', str(ROOT / 'tools/install-theme.py'), 'install',
             '--skip-deps', '--skip-restart', '--distro', args.distro, '--components', *args.components])
        print('COMPLETED: Selected theme components', flush=True)
    for app in args.apps:
        print('STEP: Installing ' + APPS[app][0], flush=True)
        script = {'settings': 'install-settings-navigation.py', 'gwenview': 'install-gwenview.py', 'kcalc': 'install-kcalc.py'}[app]
        run([sys.executable, '-u', str(ROOT / 'tools' / script)])
        print('COMPLETED: ' + APPS[app][0], flush=True)
    print('STEP: Restarting Plasma shell', flush=True)
    run(['systemctl', '--user', 'restart', 'plasma-plasmashell.service'])
    print('COMPLETED: Plasma shell restarted', flush=True)

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        sys.exit(f'Installation stopped: {error}')
