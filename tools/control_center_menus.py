# SPDX-License-Identifier: MIT
"""Tie all tray menus and selector popups to their owning representation."""
import os
import re
import shlex
import subprocess
import tempfile


def build_menu_helper(details):
    directory = details / "NativeMenus"
    flags = shlex.split(subprocess.check_output(["pkg-config", "--cflags", "--libs",
        "Qt6Quick", "Qt6Qml", "Qt6Widgets"], text=True))
    with tempfile.TemporaryDirectory(prefix="sequoia-menu-build-") as temporary:
        subprocess.run(["/usr/lib64/qt6/libexec/moc", *[f for f in flags if f.startswith(("-I", "-D"))], str(directory / "plugin.cpp"),
            "-o", temporary + "/plugin.moc"], check=True)
        subprocess.run(["c++", "-std=c++17", "-shared", "-fPIC",
            str(directory / "plugin.cpp"), "-I" + temporary, "-o",
            str(directory / "libsequoiatraymenus.so"), *flags], check=True)


def style_menu_lifetimes(destination):
    details = destination / 'details'
    changed = []
    for path in sorted(destination.rglob('*.qml')):
        relative = path.relative_to(destination)
        if relative.parts[0] == 'taskmanager' or 'tests' in relative.parts:
            continue
        if path.is_relative_to(details) and path.name != 'DetailExpandableBase.qml':
            continue
        text = path.read_text()
        updated = text
        # An explicit common scope works even when Plasma reparents a full
        # representation into the system tray's shared dialog.
        if path.name == 'main.qml' and 'property QtObject sequoiaPopupScope:' not in text:
            root = re.search(r'\b(PlasmoidItem|ContainmentItem)\s*\{\s*id:\s*(\w+)', updated)
            if root:
                expanded = 'systemTrayState.expanded' if root[1] == 'ContainmentItem' else root[2] + '.expanded'
                updated = updated[:root.end()] + ('\n    property QtObject sequoiaPopupScope: QtObject {\n'
                    f'        property bool expanded: {expanded}\n    }}') + updated[root.end():]
        prefix = '' if path.is_relative_to(details) else 'Details.'
        updated = re.sub(r'\b(?:PlasmaExtras|PE)\.Menu\s*\{', prefix + 'DetailNativeMenu {', updated)
        if re.search(r'^import org.kde.plasma.extras\s*$', updated, re.M):
            updated = re.sub(r'(?<![.\w])Menu\s*\{', prefix + 'DetailNativeMenu {', updated)
        # KDE can infer these menu anchors internally without exposing them
        # as visualParent. Name them explicitly so owner visibility is tracked.
        implicit_owners = {
            'BackgroundAppItem.qml': ('contextMenu', 'taskIcon'),
            'ControlTrayEntry.qml': ('backgroundMenu', 'card'),
            'EditContextMenu.qml': ('contextMenu', 'contextMenu.target'),
        }
        if path.name in implicit_owners:
            ident, owner = implicit_owners[path.name]
            anchor = re.search(r'\b' + re.escape(prefix) + r'DetailNativeMenu\s*\{\s*id:\s*' + ident + r'\b', updated)
            if anchor and not re.match(r'\s*visualParent:', updated[anchor.end():]):
                updated = updated[:anchor.end()] + '\n        visualParent: ' + owner + updated[anchor.end():]
        if path.name == 'DetailExpandableBase.qml':
            updated = updated.replace('DetailNativeMenu {\n                onStatusChanged:',
                'DetailNativeMenu {\n                visualParent: listItem\n                onStatusChanged:')
        native_pattern = r'(?<![.\w])(?:Notifications\.FileMenu|ListItemMenu)\s*\{\s*id:\s*(\w+)'
        for match in reversed(list(re.finditer(native_pattern, updated))):
            if re.match(r'\s*property QtObject sequoiaNativeLifetime:', updated[match.end():]):
                continue
            ident = match[1]
            addition = (f'\n    property QtObject sequoiaNativeLifetime: {prefix}DetailNativePopupLifetime {{\n'
                        f'        popup: {ident}\n        owner: {ident}.visualParent\n    }}')
            updated = updated[:match.end()] + addition + updated[match.end():]
        # Keep every selector's original style, model and actions. The helper
        # only closes its existing popup when the owning page goes away.
        pattern = r'\b(?:PC|PC3|PlasmaComponents3|PlasmaComponents|QQC|QQC2|T)\.ComboBox\s*\{'
        matches = list(re.finditer(pattern, updated))
        for number, match in reversed(list(enumerate(matches))):
            tail = updated[match.end():]
            existing = re.match(r'\s*(?:hoverEnabled:\s*true\s*)?id:\s*(\w+)', tail)
            if existing:
                ident = existing[1]
                insertion = match.end() + existing.end()
            else:
                ident = 'sequoiaMenuSelector' + str(number)
                insertion = match.end()
            # Idempotent for updates to installed packages.
            if re.match(r'\s*property QtObject sequoiaSelectorLifetime:', updated[insertion:]):
                continue
            addition = '' if existing else '\n    id: ' + ident
            addition += (f'\n    property QtObject sequoiaSelectorLifetime: {prefix}DetailPopupLifetime {{\n'
                         f'        popup: {ident}.popup\n        owner: {ident}\n    }}')
            updated = updated[:insertion] + addition + updated[insertion:]
        if not path.is_relative_to(details) and 'Details.Detail' in updated:
            relative_import = os.path.relpath(details, path.parent).replace(os.sep, '/')
            statement = 'import "' + relative_import + '" as Details'
            if re.search(r'^import .+ as Details\s*$', updated, re.M):
                updated = re.sub(r'^import .+ as Details[ \t]*$', statement, updated, flags=re.M)
            else:
                updated = re.sub(r'^(import .+)$', statement + '\n\\1', updated, count=1, flags=re.M)
        if updated == text:
            continue
        path.write_text(updated)
        changed.append(path)
    return changed
