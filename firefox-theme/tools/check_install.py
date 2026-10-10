#!/usr/bin/env python3
"""Exercise real install/update/restore behavior in disposable profiles."""
import importlib.util
from pathlib import Path
import tempfile

spec = importlib.util.spec_from_file_location('installer', Path(__file__).resolve().parents[1] / 'install.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory(prefix='sequoia-installer-test-') as temp:
    root = Path(temp)
    old = root / 'old-theme'
    old.mkdir()
    (old / 'userChrome.css').write_text('/* Original theme */\n')
    profile = root / 'profile'
    profile.mkdir()
    (profile / 'chrome').symlink_to(old, target_is_directory=True)
    original = b'// My settings\nuser_pref("example.untouched", true);'
    (profile / 'user.js').write_bytes(original)
    module.install(profile, dry_run=True)
    assert (profile / 'chrome').is_symlink()
    module.install(profile)
    assert not (profile / 'chrome').is_symlink()
    assert (old / 'userChrome.css').read_text() == '/* Original theme */\n'
    module.install(profile)
    assert len(list(profile.glob('.sequoia-backup-*'))) == 1
    assert (profile / 'user.js').read_text().count(module.BEGIN) == 1
    module.install(profile, restore=True)
    assert (profile / 'chrome').is_symlink()
    assert (profile / 'user.js').read_bytes() == original
    module.install(profile)
    with (profile / 'user.js').open('a') as f:
        f.write('// Later user setting\n')
    installed_css = profile / 'chrome/userChrome.css'
    contents = installed_css.read_text()
    installed_css.write_text(contents + '/* Edited */')
    try:
        module.install(profile, restore=True)
    except ValueError:
        pass
    else:
        raise AssertionError('Restore discarded user edits')
    installed_css.write_text(contents)
    module.install(profile, restore=True)
    assert '// Later user setting' in (profile / 'user.js').read_text()
    assert module.BEGIN not in (profile / 'user.js').read_text()
    clean = root / 'clean'
    clean.mkdir()
    module.install(clean)
    module.install(clean, restore=True)
    assert not (clean / 'chrome').exists()
    assert not (clean / 'user.js').exists()
print('PASS: dry run, symlink preservation, update, restore, edit protection, clean profile')
