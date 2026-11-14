#!/bin/bash
# Prints the name of the default file manager (e.g. "Dolphin", "COSMIC Files") for the "Open in …" buttons,
# then its desktop file on a second line. Prints nothing if it can't tell.
id=$(xdg-mime query default inode/directory 2>/dev/null)
[ -n "$id" ] || exit 0

IFS=: read -ra dirs <<< "${XDG_DATA_HOME:-$HOME/.local/share}:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
for dir in "${dirs[@]}"; do
    file="$dir/applications/$id"
    if [ -f "$file" ]; then
        # The plain Name= line of the [Desktop Entry] section
        sed -n '/^\[Desktop Entry\]/,/^\[/{s/^Name=//p}' "$file" | head -1
        echo "$id"
        exit 0
    fi
done
