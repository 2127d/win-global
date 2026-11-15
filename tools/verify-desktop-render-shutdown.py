#!/usr/bin/env python3
"""Exercise real preview completion/cancellation without touching the live shell."""
import argparse
import datetime
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--helper', type=Path, default=ROOT/'apps/settings/appearance-desktop-render.py')
args = parser.parse_args()
spec = importlib.util.spec_from_file_location('renderer', args.helper)
renderer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(renderer)
config = renderer.config(Path.home()/'.config/kdeglobals')
rgba = config.get('Colors:Window', 'BackgroundNormal', fallback='242,243,245').split(',')
kwin = renderer.config(Path.home()/'.config/kwinrc')
material = 'solid' if len(rgba) < 4 or int(rgba[3]) == 255 else 'glass' if kwin.getboolean('Plugins', 'sequoia_liquid_glass_v6Enabled', fallback=False) else 'transparent'
started = datetime.datetime.now().astimezone().isoformat(timespec='seconds')
all_pids = set()


def processes():
    result = {}
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            fields = (entry/'stat').read_text().rsplit(')', 1)[1].split()
            command = (entry/'cmdline').read_bytes().split(b'\0')[0].decode(errors='replace')
            result[int(entry.name)] = (int(fields[1]), fields[0], fields[19], Path(command).name)
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            pass
    return result


with tempfile.TemporaryDirectory(prefix='preview-shutdown-check-') as temporary:
    directory = Path(temporary)
    for name, trigger in [('complete', None), ('startup', 'dbus-daemon'),
                          ('shell', 'plasmashell'), ('applications', 'kweather'),
                          ('refresh-again', 'plasmashell'), ('hung-worker', 'plasmashell')]:
        output = directory/(name+'.png')
        tracked = {}
        sent = False
        with (directory/(name+'.supervisor.log')).open('w+') as log:
            worker = subprocess.Popen(['python3', str(args.helper), '--material', material,
                '--mode', 'current', '--width', '640', '--height', '480', '--output', str(output)],
                stdout=log, stderr=subprocess.STDOUT)
            tracked[worker.pid] = None
            try:
                deadline = time.monotonic() + 95
                while worker.poll() is None and time.monotonic() < deadline:
                    current = processes()
                    # Include descendants even after their parents exit.
                    changed = True
                    while changed:
                        changed = False
                        for pid, (parent, state, birth, command) in current.items():
                            if pid not in tracked and parent in tracked:
                                tracked[pid] = birth
                                changed = True
                    if trigger and not sent and any(current.get(pid, (None, None, None, None))[3] == trigger for pid in tracked):
                        if name == 'hung-worker':
                            # Simulate a stuck render call; the supervisor must
                            # kill clients before taking away their private bus.
                            for pid, (parent, state, birth, command) in current.items():
                                if pid in tracked and command.startswith('python') and current.get(parent, (None, None, None, None))[3] == 'kwin_wayland':
                                    os.kill(pid, signal.SIGSTOP)
                        worker.send_signal(signal.SIGTERM)
                        # Refresh followed immediately by close must not interrupt cleanup.
                        time.sleep(.05)
                        if worker.poll() is None:
                            worker.send_signal(signal.SIGTERM)
                        sent = True
                    time.sleep(.05)
                assert worker.poll() is not None, name + ': supervisor hung'
                assert worker.returncode == (143 if trigger else 0), (name, worker.returncode)
                assert not trigger or sent, name + ': trigger never reached'
                current = processes()
                leftovers = {pid: current[pid] for pid, birth in tracked.items()
                             if pid in current and current[pid][2] == birth and current[pid][1] != 'Z'}
                assert not leftovers, (name, leftovers)
                log.seek(0)
                supervisor = log.read()
                inner = Path(str(output)+'.log').read_text()
                for text in (inner, supervisor):
                    assert 'exit=-6' not in text and '_dbus_abort' not in text, text[-3000:]
                stopped = re.findall(r'preview-stop: (\S+) pid=\d+ exit=', inner)
                expected = ['kweather', 'gwenview', 'plasmashell', 'kactivitymanagerd']
                assert stopped == [process for process in expected if process in stopped], stopped
                outer = re.findall(r'preview-stop: (\S+) pid=\d+ exit=', supervisor)
                assert outer[-1:] == ['dbus-daemon'], outer
                if name == 'hung-worker':
                    assert 'worker grace period expired' in supervisor, supervisor
                if not trigger:
                    assert stopped == expected, stopped
                    assert output.is_file() and output.stat().st_size > 10000
                    metadata = json.loads(Path(str(output)+'.json').read_text())
                    assert set(metadata['applications']) == {'weather', 'gwenview'}
                print(f'PASS {name}: exit {worker.returncode}, no remaining preview processes; stopped {stopped}', flush=True)
            except BaseException:
                log.seek(0)
                print(log.read())
                if Path(str(output)+'.log').exists():
                    print(Path(str(output)+'.log').read_text()[-4000:])
                raise
            finally:
                all_pids.update(tracked)
                if worker.poll() is None:
                    worker.terminate()
                    worker.wait(timeout=30)
    # systemd-coredump records can arrive just after process exit.
    time.sleep(2)
    journal = subprocess.run(['journalctl', '--quiet', '-t', 'systemd-coredump', '--since', started,
                              '-o', 'json', '--no-pager'], check=True, text=True, capture_output=True)
    crashes = [record for line in journal.stdout.splitlines() if (record := json.loads(line)).get('COREDUMP_PID')
               and int(record['COREDUMP_PID']) in all_pids]
    assert not crashes, [(record['COREDUMP_PID'], record.get('COREDUMP_COMM')) for record in crashes]
    print(f'PASS: no core dumps from {len(all_pids)} observed preview processes', flush=True)
