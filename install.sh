#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# The installer builds native menu toggles and stages Quick indicators plus Plasma switch artwork.
exec /usr/bin/python3 "$project_dir/tools/install-theme.py" install "$@"
