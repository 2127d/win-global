#!/bin/bash
# The right-click menu's actions on a file in the Stack:
#   file-actions.sh launch <desktop file id> <url>... open them with that app
#   file-actions.sh choose <url>                     KDE's "Choose Application" dialog
#   file-actions.sh show <url>...                    show them, selected, in the file manager
#   file-actions.sh rename <path> <new name>         prints "exists" if the new name is taken
#   file-actions.sh trash <url>...                   move to the Bin
#   file-actions.sh copy <text>                      put it on the clipboard
action=$1
shift

case $action in
    launch)
        app=${1%.desktop}
        shift
        for url in "$@"; do
            kstart --application "$app" --url "$url"
        done
        ;;
    choose)
        # KIO asks which app to use when nothing is registered for the type it's told the file is.
        # The dialog still shows (and can set the default for) the file's real type.
        kioclient --interactive exec "$1" application/x-stack-choose-application
        ;;
    show)
        # The standard "show this file" call Dolphin, Nautilus and most file managers answer;
        # otherwise just open the folder it's in
        # (dbus-send splits its list on commas, so a comma in a name is written the URL way)
        list=$(printf '%s,' "${@//,/%2C}")
        dbus-send --session --print-reply --dest=org.freedesktop.FileManager1 /org/freedesktop/FileManager1 \
            org.freedesktop.FileManager1.ShowItems array:string:"${list%,}" string:"" >/dev/null 2>&1 \
            || xdg-open "$(dirname "$1")"
        ;;
    rename)
        dest="$(dirname "$1")/$2"
        if [ -e "$dest" ]; then
            echo exists
        else
            mv -n -- "$1" "$dest"
        fi
        ;;
    trash)
        # Interactive: KDE explains if it can't (e.g. a drive with no Bin)
        kioclient --interactive move "$@" trash:/
        ;;
    copy)
        # Plasma's clipboard first: a panel widget can't always set the clipboard itself on Wayland
        dbus-send --session --print-reply --dest=org.kde.klipper /klipper \
            org.kde.klipper.klipper.setClipboardContents string:"$1" >/dev/null 2>&1 \
            || printf '%s' "$1" | wl-copy 2>/dev/null \
            || printf '%s' "$1" | xclip -selection clipboard 2>/dev/null
        ;;
esac
