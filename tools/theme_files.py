"""File ownership receipts for reversible theme installation."""
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import tempfile


def exists(path):
    return path.exists() or path.is_symlink()


def fingerprint(path):
    if path.is_symlink():
        return 'link:' + os.readlink(path)
    if not path.exists():
        return None
    if not path.is_file():
        raise RuntimeError(f'Expected a file: {path}')
    return 'sha256:' + hashlib.sha256(path.read_bytes()).hexdigest()


def copy_file(source, target):
    # Replacing the directory entry also avoids writing through existing hardlinks.
    descriptor, name = tempfile.mkstemp(prefix='.sequoia-copy-', dir=target.parent)
    os.close(descriptor)
    temporary = Path(name)
    try:
        if source.is_symlink():
            temporary.unlink()
        shutil.copy2(source, temporary, follow_symlinks=False)
        temporary.replace(target)
    finally:
        temporary.unlink(missing_ok=True)


def payload_files(folder):
    result = {}
    for base, directories, files in os.walk(folder, followlinks=False):
        for name in list(directories):
            if (Path(base) / name).is_symlink():
                files.append(name)
                directories.remove(name)
        for name in files:
            path = Path(base) / name
            result[path.relative_to(folder).as_posix()] = path
    return result


class Receipt:
    def __init__(self, root, state, allowed):
        self.root = Path(root).absolute()
        self.state = self.root / state
        self.allowed = allowed
        self.manifest = self.state / 'manifest.json'
        self.data = {'version': 1, 'root': str(self.root), 'files': {}, 'directories': []}
        self.check_parents(self.manifest)
        if self.manifest.is_symlink():
            raise RuntimeError(f'Receipt must not be a symlink: {self.manifest}')
        if self.manifest.exists():
            self.data = json.loads(self.manifest.read_text())
            if self.data.get('version') != 1 or self.data.get('root') != str(self.root):
                raise RuntimeError('Installation receipt belongs to a different destination.')
            for name, entry in self.data['files'].items():
                self.target(name)
                if entry['original'] not in (None, hashlib.sha256(name.encode()).hexdigest()):
                    raise RuntimeError('Invalid backup filename in installation receipt.')
            for name in self.data['directories']:
                self.relative(name)

    @staticmethod
    def relative(name):
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or not path.parts or str(path) != name:
            raise RuntimeError(f'Invalid installation path: {name}')

    def target(self, name):
        self.relative(name)
        if not self.allowed(name):
            raise RuntimeError(f'Path is outside the theme installation: {name}')
        target = self.root / name
        self.check_parents(target)
        return target

    @staticmethod
    def check_parents(path):
        for parent in path.parents:
            if parent.is_symlink():
                raise RuntimeError(f'Refusing to write through a directory symlink: {parent}')

    def save(self):
        self.state.mkdir(parents=True, exist_ok=True)
        self.state.chmod(0o700)
        temporary = self.manifest.with_suffix('.tmp')
        if temporary.is_symlink():
            raise RuntimeError(f'Unexpected receipt symlink: {temporary}')
        temporary.write_text(json.dumps(self.data, indent=2) + '\n')
        temporary.replace(self.manifest)

    def validate(self, incoming=None, force=False, scope=None):
        for name, entry in self.data['files'].items():
            if scope is not None and not scope(name):
                continue
            current = fingerprint(self.target(name))
            # A standalone component refresh may leave an older receipt. An
            # exact match to the incoming payload loses no local changes.
            matches_incoming = (incoming is not None and name in incoming
                                and current == fingerprint(incoming[name]))
            if not force and current not in (None, entry['installed']) and not matches_incoming:
                raise RuntimeError(f'Installed file was edited: {self.root / name}. '
                                   'Save your changes, then use --force to replace/remove it.')
            if entry['original'] is not None and not exists(self.state / 'originals' / entry['original']):
                raise RuntimeError(f'Missing original backup for {name}')
        for name in incoming or {}:
            fingerprint(self.target(name))

    def mkdirs(self, parent):
        missing = []
        while parent != self.root and not parent.exists():
            missing.append(parent)
            parent = parent.parent
        for directory in reversed(missing):
            name = directory.relative_to(self.root).as_posix()
            if name not in self.data['directories']:
                self.data['directories'].append(name)
            directory.mkdir(exist_ok=True)

    def backup_modified(self, incoming, scope=None):
        """Keep replaced local edits independently of uninstall/rollback backups."""
        changed = {}
        for name, entry in self.data['files'].items():
            if scope is not None and name not in incoming and not scope(name):
                continue
            current = fingerprint(self.target(name))
            if current in (None, entry['installed']):
                continue
            if name in incoming and current == fingerprint(incoming[name]):
                continue
            changed[name] = current
        if not changed:
            return
        backups = self.state.with_name(self.state.name + '-modified-backups')
        self.check_parents(backups / 'snapshot')
        backups.mkdir(parents=True, exist_ok=True, mode=0o700)
        backup = Path(tempfile.mkdtemp(prefix='snapshot-', dir=backups))
        for name in changed:
            target = backup / 'files' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            copy_file(self.target(name), target)
        (backup / 'manifest.json').write_text(json.dumps({
            'root': str(self.root), 'files': changed,
        }, indent=2) + '\n')
        print(f'Saved modified installed files: {backup}', flush=True)
        return backup

    def prune(self):
        for name in sorted(self.data['directories'], key=lambda s: len(PurePosixPath(s).parts), reverse=True):
            path = self.root / name
            self.check_parents(path)
            if not path.is_symlink():
                try:
                    path.rmdir()
                except OSError:
                    pass  # Shared directories and user-added files are retained.

    def install(self, folder, force=False, original_overrides=None, obsolete=(), scope=None):
        incoming = payload_files(Path(folder))
        original_overrides = original_overrides or {}
        if not incoming:
            raise RuntimeError('Refusing to install an empty theme payload.')
        # Check everything this transaction can replace or retire, while leaving
        # edits in unselected components alone. Incoming files always count,
        # even when the caller's retirement scope does not include them.
        self.validate(incoming, force, scope=None if scope is None else
                      lambda name: name in incoming or scope(name))
        if force:
            self.backup_modified(incoming, scope)
        old = json.loads(json.dumps(self.data))
        for name in obsolete:
            target = self.target(name)
            if name not in incoming and name not in self.data['files'] and exists(target):
                self.data['files'][name] = {'original': None, 'installed': fingerprint(target)}
        self.save()
        originals = self.state / 'originals'
        if originals.is_symlink():
            raise RuntimeError('Backup directory must not be a symlink.')
        originals.mkdir(exist_ok=True)
        # Separate the previous invocation from originals retained across upgrades.
        with tempfile.TemporaryDirectory(prefix='transaction-', dir=self.state) as temporary:
            previous, touched = {}, []
            # A component refresh retires stale files only in that component.
            # Other installed components retain their ownership and backups.
            names = set(incoming) | {name for name in self.data['files']
                                     if scope is None or scope(name)}
            for index, name in enumerate(sorted(names)):
                target = self.target(name)
                backup = Path(temporary) / str(index)
                previous[name] = backup if exists(target) else None
                if exists(target):
                    copy_file(target, backup)
            try:
                for name, source in incoming.items():
                    target = self.target(name)
                    if name not in self.data['files']:
                        original = None
                        prior = original_overrides.get(name, target)
                        if prior is not None and exists(prior):
                            original = hashlib.sha256(name.encode()).hexdigest()
                            copy_file(prior, originals / original)
                        self.data['files'][name] = {'original': original, 'installed': None}
                    self.data['files'][name]['installed'] = fingerprint(source)
                    self.mkdirs(target.parent)
                self.save()  # Persist ownership before copying live files.
                for name in sorted(names):
                    touched.append(name)
                    if name in incoming:
                        copy_file(incoming[name], self.target(name))
                    else:
                        self.restore(name)
                self.prune()
                self.save()
            except BaseException:
                for name in reversed(touched):
                    target = self.target(name)
                    target.unlink(missing_ok=True)
                    if previous[name] is not None:
                        copy_file(previous[name], target)
                self.prune()
                self.data = old
                self.save()
                raise
        used = {e['original'] for e in self.data['files'].values()}
        for path in originals.iterdir():
            if path.name not in used:
                path.unlink()

    def restore(self, name):
        entry = self.data['files'][name]
        target = self.target(name)
        target.unlink(missing_ok=True)
        if entry['original'] is not None:
            target.parent.mkdir(parents=True, exist_ok=True)
            copy_file(self.state / 'originals' / entry['original'], target)
        del self.data['files'][name]

    def uninstall(self, force=False, scope=None):
        if not self.manifest.exists():
            print(f'No tracked theme installation under {self.root}')
            return
        self.validate(force=force, scope=scope)
        try:
            for name in list(self.data['files']):
                if scope is None or scope(name):
                    self.restore(name)
        except BaseException:
            self.save()
            raise
        if self.data['files']:
            self.prune()
            self.save()
            used = {entry['original'] for entry in self.data['files'].values()}
            for path in (self.state / 'originals').iterdir():
                if path.name not in used:
                    path.unlink()
            return
        shutil.rmtree(self.state)
        self.prune()
        try:
            self.state.parent.rmdir()
        except OSError:
            pass
