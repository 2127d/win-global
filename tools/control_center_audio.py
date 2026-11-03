# SPDX-License-Identifier: MIT
"""Output spotlight layout using the existing Plasma audio models and actions."""
import shutil


def style_audio(destination, source, replace):
    directory = destination / 'volume'
    for path in (source / 'audio').iterdir():
        if path.suffix in ('.qml', '.js') and path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = path.read_text()
    text = replace(text, '    id: main\n', """    id: main
    Plasmoid.title: i18n("Audio")
    readonly property bool sequoiaFooterActions: true
    readonly property bool audioSettingsAllowed: configureAction.visible
    function openAudioSettings() { configureAction.trigger(); }
    function retryAudioService() {
        if (!kdedContextReady) kded.reconnectContextAsync();
        if (!appletContextReady) Context.reconnectDaemon();
    }
""")
    start = text.index('    fullRepresentation: Details.DetailRepresentation {')
    end = text.index('\n    KDEDProxy {', start)
    text = text[:start] + """    fullRepresentation: AudioFullRepresentation {
        applet: main
        audioConfig: config
    }
""" + text[end:]
    path.write_text(text)

    config_path = directory / 'main.xml'
    config = config_path.read_text()
    if 'name="audioLayout"' not in config:
        config = replace(config, '<group name="General">', '\n'.join([
            '<group name="General">',
            '    <entry name="audioLayout" type="String">',
            '      <label>Audio popup layout</label>',
            '      <default>spotlight</default>',
            '    </entry>',
        ]))
    config_path.write_text(config)
