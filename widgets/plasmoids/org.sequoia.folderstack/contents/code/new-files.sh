#!/bin/bash
# Names of the things that arrived in a folder after a time:  new-files.sh <folder> <seconds since 1970>
# Uses the time a file got into the folder (its inode change time), not its date: a copied
# file keeps its old date, and a moved one keeps both its dates, but either way this changes.
find "$1" -mindepth 1 -maxdepth 1 -newerct "@$2" -printf '%f\n' 2>/dev/null
