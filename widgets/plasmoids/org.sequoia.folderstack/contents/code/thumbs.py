#!/usr/bin/env python3
"""Previews (thumbnails) for files in the Stack.

    thumbs.py lookup <url>...   previews that already exist (Dolphin's shared cache or ours) - instant
    thumbs.py make <url>...     make the missing ones (PDF page, video frame, album art, office documents)

Prints one line per file that has a preview:  <url as given> TAB <preview file url>
"""
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.parse

SIZE = 256
SHARED = os.path.join(os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache")), "thumbnails")
OURS = os.path.join(os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache")), "stack-widget", "thumbnails")
KEEP_DAYS = 60

VIDEO = {"mp4", "mkv", "mov", "webm", "avi", "m4v", "wmv", "mpg", "mpeg"}
AUDIO = {"mp3", "flac", "m4a", "ogg", "opus", "aac", "wav"}
OFFICE = {"odt", "ods", "odp", "odg", "ott", "ots", "otp",
          "doc", "docx", "xls", "xlsx", "ppt", "pptx"}


def path_of(url):
    return urllib.parse.unquote(urllib.parse.urlparse(url).path)


def cache_name(path):
    # Same naming as Dolphin/KDE and GNOME: md5 of the file's URI, encoded the way Qt does it
    uri = "file://" + urllib.parse.quote(path, safe="/!$&'()*+,;=:@~")
    return hashlib.md5(uri.encode()).hexdigest() + ".png"


def png_mtime(png):
    """Thumb::MTime saved inside a shared-cache preview, or None."""
    try:
        with open(png, "rb") as f:
            if f.read(8) != b"\x89PNG\r\n\x1a\n":
                return None
            while True:
                head = f.read(8)
                if len(head) < 8:
                    return None
                length, kind = int.from_bytes(head[:4], "big"), head[4:]
                data = f.read(length)
                f.read(4)  # CRC
                if kind == b"tEXt" and data.startswith(b"Thumb::MTime\0"):
                    return int(data.split(b"\0", 1)[1])
                if kind == b"IDAT":  # text chunks come before the image data
                    return None
    except (OSError, ValueError):
        return None


def existing(path):
    try:
        st = os.stat(path)
    except OSError:
        return None
    mtime = int(st.st_mtime)
    name = cache_name(path)
    for size in ("large", "x-large", "xx-large", "normal"):
        png = os.path.join(SHARED, size, name)
        if os.path.exists(png) and png_mtime(png) == mtime:
            return png
    # Ours is good if it was made after the file last changed. The change time counts too: a file
    # replaced by a copy with an older date still has to get a new preview.
    png = os.path.join(OURS, name)
    if os.path.exists(png) and os.stat(png).st_mtime >= max(st.st_mtime, st.st_ctime):
        return png
    return None


def run(cmd):
    try:
        return subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15).returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False


def make(path, out):
    suffix = path.rsplit(".", 1)[-1].lower() if "." in path else ""
    scale = f"scale='min({SIZE},iw)':-2"
    if suffix == "pdf" and shutil.which("pdftoppm"):
        return run(["pdftoppm", "-png", "-singlefile", "-f", "1", "-l", "1", "-scale-to", str(SIZE), path, out[:-4]])
    if suffix in VIDEO and shutil.which("ffmpeg"):
        # A frame a second in (skips black first frames); very short clips fall back to the first frame
        return (run(["ffmpeg", "-y", "-loglevel", "error", "-ss", "1", "-i", path, "-frames:v", "1", "-vf", scale, out])
                or run(["ffmpeg", "-y", "-loglevel", "error", "-i", path, "-frames:v", "1", "-vf", scale, out]))
    if suffix in AUDIO and shutil.which("ffmpeg"):
        # Album art, if the file has any
        return run(["ffmpeg", "-y", "-loglevel", "error", "-i", path, "-an", "-frames:v", "1", "-vf", scale, out])
    if suffix in OFFICE and shutil.which("gsf-office-thumbnailer"):
        # The preview saved inside the document, if it has one
        return run(["gsf-office-thumbnailer", "-i", path, "-o", out, "-s", str(SIZE)])
    return False


def tidy():
    """Forget our previews nobody has needed for a while."""
    cutoff = time.time() - KEEP_DAYS * 86400
    for name in os.listdir(OURS):
        png = os.path.join(OURS, name)
        try:
            if os.stat(png).st_atime < cutoff and os.stat(png).st_mtime < cutoff:
                os.remove(png)
        except OSError:
            pass


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("lookup", "make"):
        sys.exit(__doc__)
    mode, urls = sys.argv[1], sys.argv[2:]
    os.makedirs(OURS, exist_ok=True)
    for url in urls:
        path = path_of(url)
        png = existing(path)
        if png is None and mode == "make":
            final = os.path.join(OURS, cache_name(path))
            fd, tmp = tempfile.mkstemp(suffix=".png", dir=OURS)
            os.close(fd)
            if make(path, tmp) and os.path.getsize(tmp) > 0:
                os.replace(tmp, final)
                png = final
            else:
                os.remove(tmp)
        if png:
            print(f"{url}\t{'file://' + urllib.parse.quote(png)}", flush=True)
    if mode == "make":
        tidy()


if __name__ == "__main__":
    main()
