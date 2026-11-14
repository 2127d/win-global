#!/bin/bash
# Puts files dropped on the Stack into its folder: usage  drop.sh move|copy <folder> <url>...
mode=$1
dest=$2
shift 2

for url in "$@"; do
    case $url in
        file://*)
            path=${url#file://}
            path=$(printf '%b' "${path//%/\\x}")    # undo %20 etc.
            # Dropped from this same folder (e.g. dragged out of the fan and back again): nothing to do
            [ "$(dirname "$path")" -ef "$dest" ] && continue
            kioclient "$mode" "$url" "file://$dest/"
            ;;
        *)
            # Links from a browser and the like can only be downloaded
            kioclient copy "$url" "file://$dest/"
            ;;
    esac
done
