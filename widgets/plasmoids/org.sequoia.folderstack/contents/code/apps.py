#!/usr/bin/env python3
"""Apps that can open a file, for the "Open With" part of the right-click menu.

    apps.py <path>

Prints one line per app, the default first:  <desktop file id> TAB <name> TAB <icon>
For a folder the default file manager is left out: the menu already has "Open in <file manager>".
"""
import configparser
import os
import subprocess
import sys

MAX_APPS = 6

HOME = os.path.expanduser("~")
DATA_DIRS = [os.environ.get("XDG_DATA_HOME") or os.path.join(HOME, ".local/share")] + \
    (os.environ.get("XDG_DATA_DIRS") or "/usr/local/share:/usr/share").split(":")
CONFIG_DIRS = [os.environ.get("XDG_CONFIG_HOME") or os.path.join(HOME, ".config")] + \
    (os.environ.get("XDG_CONFIG_DIRS") or "/etc/xdg").split(":")


def mime_type(path):
    if os.path.isdir(path):
        return "inode/directory"
    try:
        out = subprocess.run(["xdg-mime", "query", "filetype", path], capture_output=True, text=True, timeout=5).stdout
        return out.strip().split(";")[0] or "application/octet-stream"
    except (OSError, subprocess.TimeoutExpired):
        return "application/octet-stream"


def parents(mime):
    """The type itself, then the types it's a kind of (text/markdown -> text/plain), nearest first."""
    subclasses = {}
    for d in DATA_DIRS:
        try:
            with open(os.path.join(d, "mime", "subclasses")) as f:
                for line in f:
                    child, _, parent = line.strip().partition(" ")
                    subclasses.setdefault(child, []).append(parent)
        except OSError:
            pass
    order, todo = [], [mime]
    while todo:
        m = todo.pop(0)
        if m not in order:
            order.append(m)
            todo += subclasses.get(m, [])
    # Everything is an octet-stream; apps for that aren't useful suggestions
    return [m for m in order if m != "application/octet-stream"]


def read_ini(path):
    ini = configparser.ConfigParser(interpolation=None, strict=False, delimiters=("=",))
    ini.optionxform = str
    try:
        ini.read(path, encoding="utf-8")
    except (configparser.Error, UnicodeDecodeError):
        pass
    return ini


def ids(value):
    return [i for i in value.split(";") if i]


def associations(mimes):
    """Desktop file ids for the types, in the order the system prefers them."""
    lists = [os.path.join(d, n) for d in CONFIG_DIRS for n in ("kde-mimeapps.list", "mimeapps.list")] + \
        [os.path.join(d, "applications", n) for d in DATA_DIRS for n in ("kde-mimeapps.list", "mimeapps.list")]
    configs = [read_ini(p) for p in lists if os.path.exists(p)]
    caches = [read_ini(os.path.join(d, "applications", "mimeinfo.cache")) for d in DATA_DIRS]

    found, removed = [], set()
    for mime in mimes:
        for ini in configs:
            if ini.has_option("Removed Associations", mime):
                removed.update(ids(ini.get("Removed Associations", mime)))
    # Defaults (for the type or anything it's a kind of) come before apps that merely can open it
    for section in ("Default Applications", "Added Associations"):
        for mime in mimes:
            for ini in configs:
                if ini.has_option(section, mime):
                    found += ids(ini.get(section, mime))
    for mime in mimes:
        for ini in caches:
            if ini.has_option("MIME Cache", mime):
                found += ids(ini.get("MIME Cache", mime))
    seen, result = set(), []
    for i in found:
        if i not in seen and i not in removed:
            seen.add(i)
            result.append(i)
    return result


def desktop_file(app_id):
    for d in DATA_DIRS:
        path = os.path.join(d, "applications", app_id)
        if os.path.exists(path):
            return path
        # "kde-foo.desktop" can also live at applications/kde/foo.desktop
        path = os.path.join(d, "applications", app_id.replace("-", "/", 1))
        if "-" in app_id and os.path.exists(path):
            return path
    return None


def default_file_manager():
    try:
        return subprocess.run(["xdg-mime", "query", "default", "inode/directory"],
                              capture_output=True, text=True, timeout=5).stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        return None


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    shown, names = 0, set()
    skip = default_file_manager() if os.path.isdir(sys.argv[1]) else None
    for app_id in associations(parents(mime_type(sys.argv[1]))):
        if app_id == skip:
            continue
        path = desktop_file(app_id)
        if not path:
            continue
        ini = read_ini(path)
        if not ini.has_section("Desktop Entry"):
            continue
        entry = ini["Desktop Entry"]
        name = entry.get("Name", app_id)
        # Some apps have one desktop file per type they open (Okular); list them once
        if entry.get("Hidden", "false") == "true" or not entry.get("Exec") or name in names:
            continue
        names.add(name)
        print(f"{app_id}\t{name}\t{entry.get('Icon', '')}", flush=True)
        shown += 1
        if shown == MAX_APPS:
            break


if __name__ == "__main__":
    main()
