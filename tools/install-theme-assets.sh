#!/usr/bin/env bash
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
data_dir="${XDG_DATA_HOME:-$HOME/.local/share}"

if [[ "${1:-}" == "--help" ]]; then
    printf '%s\n' 'Usage: bash tools/install-theme-assets.sh [--prefix DATA_DIRECTORY]' 'Internal artwork staging helper; no native plugins or uninstall receipts. Use ./install.sh for a complete installation.'
    exit 0
fi
if [[ "${1:-}" == "--prefix" && $# == 2 && -n "$2" ]]; then
    data_dir="$2"
elif [[ $# != 0 ]]; then
    printf '%s\n' 'Invalid arguments. See tools/install-theme-assets.sh --help.' >&2
    exit 2
fi

mkdir -p -- "$data_dir/plasma/desktoptheme" "$data_dir/plasma/look-and-feel" "$data_dir/color-schemes" "$data_dir/wallpapers" "$data_dir/icons"
cp -R -- "$source_dir"/icons/SequoiaTahoe* "$data_dir/icons/"
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    for theme in "$data_dir"/icons/SequoiaTahoe*; do
        gtk-update-icon-cache --force --ignore-theme-index "$theme"
    done
fi
cp -R -- "$source_dir/wallpapers/SequoiaEleven" "$data_dir/wallpapers/"
for variant in light dark; do
    cp -R -- "$source_dir/plasma/desktoptheme/sequoia-eleven-$variant" "$data_dir/plasma/desktoptheme/"
    # Remove the palette shipped by older Sequoia versions so accent colors work.
    rm -f -- "$data_dir/plasma/desktoptheme/sequoia-eleven-$variant/colors"
done
cp -- "$source_dir"/color-schemes/SequoiaEleven*.colors "$data_dir/color-schemes/"
cp -R -- "$source_dir"/plasma/look-and-feel/org.sequoia.eleven.* "$data_dir/plasma/look-and-feel/"
# Only supply missing layout widgets. This staging helper must not overwrite a
# separately installed/customized widget; the main installer tracks ownership.
mkdir -p -- "$data_dir/plasma/plasmoids"
for widget in org.sequoia.startmenu org.sequoia.downloads org.sequoia.dockseparator org.kpple.kppleMenu org.kde.windowtitle.Fork; do
    if [[ ! -e "$data_dir/plasma/plasmoids/$widget" && ! -L "$data_dir/plasma/plasmoids/$widget" ]]; then
        cp -R -- "$source_dir/plasma/plasmoids/$widget" "$data_dir/plasma/plasmoids/"
    fi
done
printf 'Installed Sequoia Eleven to %s\nChoose Light/Dark Solid or Transparent in System Settings → Colors & Themes → Global Theme.\nChoose Sequoia Eleven in Wallpaper settings for automatic light/dark switching based on the color scheme.\n' "$data_dir"
