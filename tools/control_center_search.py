# SPDX-License-Identifier: MIT
"""Keep all tray-widget search magnifiers two standard icon sizes larger."""
import os
import re


def style_search_fields(destination):
    details = destination / 'details'
    for path in destination.rglob('*.qml'):
        if path.is_relative_to(details):
            continue
        text = path.read_text()
        updated = re.sub(r'\b(?:PlasmaExtras|PE)\.SearchField\s*\{', 'Details.DetailSearchField {', text)
        updated = re.sub(r'\bKirigami\.SearchField\s*\{', 'Details.DetailKirigamiSearchField {', updated)
        if updated == text:
            continue
        if not re.search(r'^import .+ as Details\s*$', updated, re.M):
            relative = os.path.relpath(details, path.parent).replace(os.sep, '/')
            updated = re.sub(r'^(import .+)$', r'import "' + relative + '" as Details\n\1', updated, count=1, flags=re.M)
        path.write_text(updated)
