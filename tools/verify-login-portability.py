#!/usr/bin/env python3
"""Stage login files for different account records without installing services."""
import importlib.util
import json
import os
from pathlib import Path
from types import SimpleNamespace
import tempfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('login_installer', Path(__file__).with_name('install-login.py'))
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)

with tempfile.TemporaryDirectory(prefix='sequoia-login-portability-') as temporary:
    root = Path(temporary)
    records = {
        'guest-one': SimpleNamespace(pw_name='guest-one', pw_dir=str(root/'guest one'), pw_uid=12001),
        'guest-two': SimpleNamespace(pw_name='guest-two', pw_dir=str(root/'guest two'), pw_uid=12002),
        'plasmalogin': SimpleNamespace(pw_name='plasmalogin', pw_dir=str(root/'greeter'), pw_uid=12003),
    }
    for index, (user, sudo, external) in enumerate([
            ('guest-one', False, False), ('guest-two', True, False),
            ('guest-one', False, True), ('guest-two', True, True)]):
        config = root/f'external settings {index}' if external else Path(records[user].pw_dir)/'.config'
        config.mkdir(parents=True, exist_ok=True)
        mode = 'light' if index % 2 == 0 else 'dark'
        (config/'kdeglobals').write_text(f'[General]\nColorScheme=SequoiaEleven{mode.title()}Solid\n')
        stage = root/f'staged installation {index}'
        argv = ['install-login.py', '--prefix', str(stage)]
        if sudo and external:
            argv += ['--config-dir', str(config)]
        # Emulate sudo clearing XDG variables and preserving the selected user;
        # non-sudo installation discovers the current user's passwd record.
        env = {'XDG_CONFIG_HOME': '' if sudo or not external else str(config),
               'SUDO_USER': user if sudo else ''}
        current = SimpleNamespace(pw_name='root', pw_uid=0) if sudo else records[user]
        with patch.dict(os.environ, env), patch('sys.argv', argv), \
                patch.object(installer.pwd, 'getpwnam', side_effect=records.__getitem__), \
                patch.object(installer.pwd, 'getpwuid', return_value=current), \
                patch.object(installer.os, 'getuid', return_value=current.pw_uid):
            installer.main()
        settings = json.loads((stage/'etc/sequoia-eleven-login.json').read_text())
        assert settings['source_config'] == str(config)
        watcher = (stage/'etc/systemd/system/sequoia-eleven-login-sync.path').read_text()
        assert f'PathChanged={config}/kdeglobals\n' in watcher
        greeter = stage/records['plasmalogin'].pw_dir.lstrip('/')/'.config/kdeglobals'
        assert f'ColorScheme=SequoiaEleven{mode.title()}\n' in greeter.read_text()
        print(f'PASS: {user}, sudo={sudo}, external XDG={external}, staged account selection and login appearance')
print('Account records were simulated; no user accounts or system services were changed.')
