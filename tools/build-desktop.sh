#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
source_dir="$project_dir/themes/application-style/SequoiaEleven"
build_dir="$project_dir/build/desktop"
jobs=$(/usr/bin/python3 -c 'import sys; sys.path.insert(0, sys.argv[1]); from installer_platform import build_jobs; print(build_jobs())' "$project_dir/tools")
printf 'Building desktop theme with %s parallel jobs.\n' "$jobs"

# CMake caches absolute source/build paths. A copied checkout must configure
# from scratch, even when it includes another account's successful build.
if [[ -f "$build_dir/CMakeCache.txt" ]]; then
    cached_source=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$build_dir/CMakeCache.txt")
    cached_build=$(sed -n 's/^CMAKE_CACHEFILE_DIR:INTERNAL=//p' "$build_dir/CMakeCache.txt")
    if [[ "$cached_source" != "$source_dir" || "$cached_build" != "$build_dir" ]]; then
        saved_dir=$(mktemp -d "$project_dir/build/desktop-relocated-XXXXXXXX")
        mv -- "$build_dir" "$saved_dir/desktop"
        printf 'Checkout moved; saved the old CMake build in %s and configuring a fresh build.\n' "$saved_dir/desktop"
    fi
fi

read -r library_dir plugin_dir < <(/usr/bin/python3 -c 'import sys; sys.path.insert(0, sys.argv[1]); from installer_platform import native_layout; print(*native_layout())' "$project_dir/tools")
cmake -S "$source_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_QT5=OFF -DBUILD_QT6=ON -DBUILD_TESTING=OFF \
    -DWITH_DECORATIONS="${SEQUOIA_WITH_DECORATIONS:-ON}" \
    -DCMAKE_INSTALL_PREFIX=/usr -DKDE_INSTALL_LIBDIR="$library_dir" \
    -DKDE_INSTALL_QTPLUGINDIR="$plugin_dir"
cmake --build "$build_dir" --parallel "$jobs"
DESTDIR="${SEQUOIA_STAGE_DIR:-$project_dir/.staging/desktop}" cmake --install "$build_dir"
