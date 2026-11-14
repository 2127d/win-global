#!/usr/bin/env python3
# The files used most recently, from any folder, newest first: the same list as KDE's
# recentlyused:/files and Kickoff's Recent Files, plus the one GNOME and GTK apps keep.
#   recent-files.py <how many>
# Prints one "<seconds since 1970>\t<size>\t<path>" line per file that still exists. Folders are left out.
import os
import stat
import sqlite3
import sys
import threading
import time
import xml.etree.ElementTree as ET
from datetime import datetime
from urllib.parse import unquote, urlparse

limit = int(sys.argv[1]) if len(sys.argv) > 1 else 50
data = os.environ.get("XDG_DATA_HOME") or os.path.expanduser("~/.local/share")
used = {}   # path -> when last used


def note(path, when):
    if path.startswith("/") and when > used.get(path, 0):
        used[path] = when


# KDE: written by the activity manager whenever a KDE app (or the file dialog) opens or saves a file.
# Empty when "Remember recent files" is off in System Settings.
try:
    db = sqlite3.connect(f"file:{data}/kactivitymanagerd/resources/database?mode=ro", uri=True, timeout=2)
    for resource, when in db.execute("SELECT targettedResource, MAX(lastUpdate) FROM ResourceScoreCache "
                                     "GROUP BY targettedResource"):
        if resource.startswith("file://"):
            resource = unquote(urlparse(resource).path)
        note(resource, when or 0)
    db.close()
except sqlite3.Error:
    pass

# GNOME, GTK and many other apps (KDE apps write here too)
try:
    for bookmark in ET.parse(f"{data}/recently-used.xbel").getroot().iter("bookmark"):
        href = bookmark.get("href", "")
        if not href.startswith("file://"):
            continue
        stamp = bookmark.get("visited") or bookmark.get("modified") or ""
        try:
            when = datetime.fromisoformat(stamp.replace("Z", "+00:00")).timestamp()
        except ValueError:
            continue
        note(unquote(urlparse(href).path), int(when))
except (OSError, ET.ParseError):
    pass



# Still there, and a file? A network drive that's out of reach can hang for a long time on this,
# so each one is looked at in its own thread and anything that hasn't answered in time is left out.
def check(path, results):
    try:
        st = os.stat(path)
        if stat.S_ISREG(st.st_mode):
            results[path] = st.st_size
    except OSError:
        pass   # deleted or moved since


newest = [(path, when) for path, when in sorted(used.items(), key=lambda item: -item[1]) if "\n" not in path]
shown = 0
start = 0
deadline = time.monotonic() + 2
while shown < limit and start < len(newest) and time.monotonic() < deadline:
    batch = newest[start:start + limit * 2]
    start += len(batch)
    results = {}
    threads = [threading.Thread(target=check, args=(path, results), daemon=True) for path, _ in batch]
    for thread in threads:
        thread.start()
    # A file on this computer answers at once; half a second is plenty for a network drive that's up
    waited = min(deadline, time.monotonic() + 0.5)
    for thread in threads:
        thread.join(max(0, waited - time.monotonic()))
    for path, when in batch:
        if path in results and shown < limit:
            print(f"{int(when)}\t{results[path]}\t{path}")
            shown += 1

sys.stdout.flush()
os._exit(0)   # don't wait for checks still stuck on an unreachable drive
