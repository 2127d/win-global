# SPDX-License-Identifier: MIT
"""Presentation for KScreen's separate layout/presentation-mode tray applet."""
import shutil


def style_displayconfiguration(destination, source, plugin_dir, extract, replace):
    plugin = plugin_dir / 'org.kde.kscreen.so'
    if not plugin.exists():
        return []
    directory = destination / 'kscreen'
    extract(plugin, ':/qt/qml/plasma/applet/org/kde/kscreen/', directory)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '''    id: root
    readonly property bool sequoiaFooterActions: true
    Plasmoid.title: i18n("Display Configuration")
    property bool presentationBusy: false
    property string presentationError: ""
    function setPresentationMode(enabled) {
        if (presentationBusy) return;
        const service = pmSource.serviceForSource("PowerDevil");
        if (!service) {
            presentationError = i18n("Power management is currently unavailable.");
            return;
        }
        const previous = presentationModeEnabled;
        presentationModeEnabled = enabled;
        presentationBusy = true;
        presentationError = "";
        const op = service.operationDescription(enabled
            ? "beginSuppressingScreenPowerManagement" : "stopSuppressingScreenPowerManagement");
        if (enabled) op.reason = i18n("User enabled presentation mode");
        const job = service.startOperationCall(op);
        job.finished.connect(job => {
            if (job.error) {
                root.presentationModeEnabled = previous;
                root.presentationError = job.errorString || i18n("Could not change presentation mode.");
            }
            root.presentationBusy = false;
        });
    }
    QtObject {
        id: configurationController
        readonly property var layouts: Plasmoid.availableActions
        readonly property int connectedCount: Plasmoid.connectedOutputCount
        readonly property bool presentationEnabled: root.presentationModeEnabled
        readonly property bool presentationBusy: root.presentationBusy
        readonly property bool powerAvailable: !!pmSource.data.PowerDevil
        readonly property string errorMessage: root.presentationError
        readonly property var inhibitions: pmSource.inhibitions
        readonly property bool settingsAllowed: root.kcmAllowed
        function applyLayout(action) { if (connectedCount > 1) Plasmoid.applyLayoutPreset(action); }
        function setPresentationMode(enabled) { root.setPresentationMode(enabled); }
        function settings() { if (settingsAllowed) configureAction.trigger(); }
    }
''')
    start = text.index('    fullRepresentation: ColumnLayout {')
    text = text[:start] + '''    fullRepresentation: FullRepresentation { controller: configurationController }
}
'''
    path.write_text(text)
    for path in (source / 'displayconfiguration').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    return [plugin]
