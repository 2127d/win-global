"""Style app-local copies of native settings forms without replacing their logic."""
import hashlib
from pathlib import Path


def build(module, fingerprints, replace):
    from PySide6.QtCore import QLibraryInfo
    imports = Path(QLibraryInfo.path(QLibraryInfo.QmlImportsPath))
    qml = module / 'qml'
    shared = 'import org.sequoia.components as S\n'
    forms = 'org/kde/kirigamiaddons/formcard/'

    def copy(source, name, transform=lambda text: text):
        path = imports / source
        fingerprints[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        text = path.read_text().replace('import QtQuick\n', 'import QtQuick\n' + shared, 1)
        (qml / (name + '.qml')).write_text(transform(text))
        with (module / 'qmldir').open('a') as stream:
            stream.write(f'{name} 1.0 qml/{name}.qml\n')

    def form_imports(text):
        text = text.replace('import "private" as Private', '').replace("import 'private' as Private", '')
        text = text.replace("import './private' as Private", '').replace('import "./private" as Private', '')
        text = text.replace('Private.FormInlineMessage', 'SequoiaSettingsInlineMessage')
        text = text.replace('Private.SpinButton', 'SequoiaSettingsSpinButton')
        text = text.replace('Private.FormCardUnits.verticalSpacing', 'Kirigami.Units.smallSpacing')
        text = text.replace('Private.FormCardUnits.horizontalSpacing', 'Kirigami.Units.largeSpacing')
        return text.replace('import QtQuick\n', 'import QtQuick\nimport org.kde.kirigamiaddons.formcard\n', 1)

    if (imports / forms / 'private/FormInlineMessage.qml').is_file():
        copy(forms + 'private/FormInlineMessage.qml', 'SequoiaSettingsInlineMessage')
    else:
        # New Kirigami Addons uses Kirigami.InlineMessage directly. Do not
        # require the retired private helper unless a delegate still needs it.
        for name in ('FormComboBoxDelegate', 'FormSpinBoxDelegate',
                     'FormTextFieldDelegate', 'FormPasswordFieldDelegate'):
            if 'Private.FormInlineMessage' in (imports / forms / (name + '.qml')).read_text():
                raise RuntimeError('Kirigami Addons references a missing FormInlineMessage helper')

    if (imports / forms / 'private/SpinButton.qml').is_file():
        copy(forms + 'private/SpinButton.qml', 'SequoiaSettingsSpinButton')

    def card(text):
        text = text.replace('import "private" as Private', 'import org.kde.kirigamiaddons.formcard')
        text = replace(text, 'property real padding: 0', 'property real padding: 6')
        if 'Kirigami.ShadowedRectangle {' in text:
            # New Addons inlines the old private background. Retain its layout
            # and native delegates while using the shared Sequoia panel.
            text = replace(text, 'Kirigami.ShadowedRectangle {', 'S.Panel {')
            start = text.index('        readonly property real borderWidth: 1')
            end = text.index('        anchors {', start)
            text = text[:start] + '''        objectName: "sequoiaSettingsCard"
        readonly property real borderWidth: border.width
        radius: S.Tokens.panelRadius
        color: Qt.alpha(theme.windowColor, theme.glass ? .48 : theme.translucent ? .76 : 1)

''' + text[end:]
            start = text.index('        border {', start)
            end = text.index('        ColumnLayout {', start)
            text = text[:start] + text[end:]
            return text.replace(': -1', ': 0')
        text = replace(text, 'Private.FormCardBackground {', 'S.Panel {\n        objectName: "sequoiaSettingsCard"\n        readonly property real borderWidth: border.width\n        color: Qt.alpha(theme.windowColor, theme.glass ? .48 : theme.translucent ? .76 : 1)')
        text = replace(text, 'rounded: root.cardWidthRestricted', 'radius: S.Tokens.panelRadius')
        text = text.replace(': -1', ': 0')
        text = replace(text, 'root.cardWidthRestricted ? Kirigami.Units.cornerRadius : 0', 'S.Tokens.panelRadius - root.padding')
        return text
    copy(forms + 'FormCard.qml', 'SequoiaSettingsCard', card)

    def combo(text):
        text = form_imports(text)
        text = text.replace('FormComboBoxDelegate.', 'SequoiaSettingsCombo.')
        text = replace(text, 'QQC2.ComboBox {\n                id: combobox', 'S.ThemeSelector {\n                id: combobox\n                objectName: "sequoiaSettingsSelector"\n                Accessible.name: controlRoot.text\n                backdropItem: controlRoot.QQC2.ApplicationWindow.window.contentItem')
        text = replace(text, '                popup.contentItem.clip: true', '''                popup.contentItem.clip: true
                property SequoiaGlassScrollBar glassScrollBar: SequoiaGlassScrollBar {}
                Component.onCompleted: popup.contentItem.QQC2.ScrollBar.vertical = glassScrollBar''')
        return text
    copy(forms + 'FormComboBoxDelegate.qml', 'SequoiaSettingsCombo', combo)
    copy(forms + 'FormSpinBoxDelegate.qml', 'SequoiaSettingsSpin',
         lambda t: form_imports(t).replace('QQC2.SpinBox {', 'S.SpinPill {'))
    copy(forms + 'FormTextFieldDelegate.qml', 'SequoiaSettingsField',
         lambda t: replace(form_imports(t), 'TextField {', 'S.TextFieldPill {'))
    copy(forms + 'FormPasswordFieldDelegate.qml', 'SequoiaSettingsPassword',
         lambda t: replace(form_imports(t), 'Kirigami.PasswordField {', '''Kirigami.PasswordField {
                background: S.Panel {
                    radius: height / 2
                    emphasized: textField.activeFocus
                    color: Qt.alpha(theme.buttonColor, theme.glass ? .5 : 1)
                }'''))

    def configuration(text):
        text = text.replace('import QtQml\n', 'import QtQml\nimport QtCore\nimport org.kde.kirigamiaddons.settings\n')
        text = replace(text, '    id: root', '''    id: root
    property Settings preferences: Settings {
        location: StandardPaths.writableLocation(StandardPaths.GenericConfigLocation) + "/sequoia-merkuro.ini"
        category: "SettingsWindow"
    }''')
        text = replace(text, "Qt.createComponent('org.kde.kirigamiaddons.settings.private', 'ConfigWindow')", 'Qt.createComponent("SequoiaSettingsWindow.qml")')
        text = replace(text, 'root.configViewItem = component.createObject(root.window, {\n                defaultModule: defaultModule,', '''root.configViewItem = component.createObject(root.window, {
                statePreferences: root.preferences,
                defaultModule: defaultModule || root.preferences.value("lastModule", "appearance"),''')
        return text
    copy('org/kde/kirigamiaddons/settings/ConfigurationView.qml', 'SequoiaSettingsView', configuration)

    def window(text):
        text = text.replace('import QtQuick\n', 'import QtQuick\nimport org.kde.config as WindowConfig\n', 1)
        text = replace(text, '    id: root', '''    id: root
    objectName: "sequoiaMerkuroSettingsWindow"
    required property var statePreferences
    WindowConfig.WindowStateSaver { configGroupName: "SequoiaMerkuroSettingsWindow" }
    // A stable, app-specific caption lets KWin remember this dialog on Wayland.
    Component.onCompleted: title = "Merkuro Settings"
''')
        text = replace(text, '    function pageForModule(module: ConfigurationModule): var {', '''    function pageForModule(module: ConfigurationModule): var {
        statePreferences.setValue("lastModule", module.moduleId);
        statePreferences.sync();''')
        text = replace(text, '        id: drawer', '        id: drawer\n        objectName: "sequoiaMerkuroSettingsSidebar"\n        background: S.Panel { anchors.fill: parent; anchors.margins: 12; color: Qt.alpha(theme.windowColor, theme.glass ? .48 : 1) }')
        for edge in ('left', 'right', 'top', 'bottom'):
            text = replace(text, f'        {edge}Padding: 0', f'        {edge}Padding: 24')
        text = replace(text, '            spacing: 0', '            spacing: 12')
        text = replace(text, '                visible: !Kirigami.Settings.isMobile', '                background: Item {}\n                visible: !Kirigami.Settings.isMobile')
        text = replace(text, '            QQC2.ToolBar {', '            QQC2.ToolBar {\n                padding: 0')
        text = replace(text, '                Kirigami.SearchField {', '                S.SearchField {\n                    objectName: "sequoiaSettingsSearch"\n                    placeholderText: i18n("Search settings…")')
        text = replace(text, '                    id: listview', '                    id: listview\n                    spacing: 8\n                    clip: true')
        text = replace(text, '            QQC2.ScrollView {', '''            QQC2.ScrollView {
                QQC2.ScrollBar.vertical: SequoiaGlassScrollBar {}''')
        text = replace(text, 'delegate: Delegates.RoundedItemDelegate {', 'delegate: S.NavigationItem {')
        text = replace(text, '                        id: settingDelegate', '                        id: settingDelegate\n                        objectName: "sequoiaSettingsNav-" + modelData.moduleId\n                        width: ListView.view.width\n                        highlighted: checked')
        text = replace(text, '                        text: modelData?.text', '''                        contentItem: RowLayout {
                            spacing: 12
                            layoutDirection: settingDelegate.mirrored ? Qt.RightToLeft : Qt.LeftToRight
                            Kirigami.Icon {
                                objectName: "sequoiaSettingsIcon-" + settingDelegate.modelData.moduleId
                                source: settingDelegate.icon.name
                                implicitWidth: 20
                                implicitHeight: 20
                                isMask: true
                                color: Kirigami.Theme.textColor
                                Accessible.ignored: true
                            }
                            QQC2.Label {
                                Layout.fillWidth: true
                                text: settingDelegate.text
                                elide: Text.ElideRight
                                font.weight: settingDelegate.checked ? Font.DemiBold : Font.Normal
                            }
                        }
                        text: modelData?.text''')
        return text
    copy('org/kde/kirigamiaddons/settings/private/ConfigWindow.qml', 'SequoiaSettingsWindow', window)

    def account(text):
        text = text.replace('FormCard.FormCard {', 'SequoiaSettingsCard {')
        text = text.replace('QQC2.Button {', 'S.ActionPill {')
        if 'Components.MessageDialog {' in text:
            text = replace(text, 'onClicked: (root.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack.pushDialogLayer(addAccountPage)', 'onClicked: addAccountDialog.open()')
            start = text.index('    data: Component {\n        id: addAccountPage')
            text = text[:start] + '''    SequoiaAddAccountDialog {
        id: addAccountDialog
        configuration: root._configuration
        title: root.addPageTitle
        backdropItem: root.QQC2.ApplicationWindow.window.contentItem
    }
}
'''
            text = replace(text, 'import QtQuick\n', 'import QtQuick\nimport QtQuick as Quick\n')
            text = replace(text, 'Components.MessageDialog {', 'S.SurfaceDialog {')
            text = replace(text, '        id: dialog', '''        id: dialog
        objectName: "sequoiaAccountDialog"
        backdropItem: root.QQC2.ApplicationWindow.window.contentItem
        width: Math.min(520, parent ? Math.max(0, parent.width - 32) : 520)
        dim: true''')
            text = replace(text, '        standardButtons: Kirigami.Dialog.NoButton\n        iconName: \'\'', '')
            text = replace(text, '        QQC2.Label {', '''        contentItem: QQC2.Label {
            color: dialog.theme.textColor''')
            text = replace(text, '        footer: QQC2.DialogButtonBox {', '''        footer: QQC2.DialogButtonBox {
            spacing: 8
            alignment: Qt.AlignRight
            // Merkuro's application-wide Escape action must not steal this key.
            Keys.onShortcutOverride: event => { if (event.key === Qt.Key_Escape) event.accepted = true; }
            Keys.onEscapePressed: event => { event.accepted = true; dialog.reject(); }
            background: Quick.Item {}''')
            text = replace(text, '            standardButtons: QQC2.Dialog.Cancel', '''            standardButtons: QQC2.Dialog.NoButton
            S.ActionPill {
                objectName: "sequoiaAccountCancel"
                theme: dialog.theme
                text: i18ndc("libakonadi6", "@action:button", "Cancel")
                icon.name: "dialog-cancel"
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
                onClicked: dialog.reject()
            }''')
            for label in ('Modify', 'Delete'):
                text = replace(text, '            S.ActionPill {\n                text: i18ndc("libakonadi6", "@action:button", "' + label + '")',
                               '            S.ActionPill {\n                objectName: "sequoiaAccount' + label + '"\n                theme: dialog.theme\n                text: i18ndc("libakonadi6", "@action:button", "' + label + '")')
        if '            id: overlay' in text:
            text = replace(text, '            id: overlay', '''            id: overlay
            property SequoiaSettingsScrollEdges scrollEdges: SequoiaSettingsScrollEdges { page: overlay }''')
        return text
    copy('org/kde/akonadi/AgentConfigurationForm.qml', 'SequoiaSettingsAccounts', account)
    copy('org/kde/merkuro/components/SystemAccountsForm.qml', 'SequoiaSettingsSystemAccounts', account)

    path = qml / 'Settings/ConfigurationsView.qml'
    text = path.read_text().replace('import QtQuick\n', 'import QtQuick\nimport org.kde.merkuro.calendar as Calendar\n')
    text = replace(text, 'icon.name: "preferences-desktop-theme-global"', 'icon.name: "draw-brush"')
    text = replace(text, 'icon.name: "preferences-system-users"', 'icon.name: "user-identity"')
    path.write_text(replace(text, 'KirigamiSettings.ConfigurationView {', 'Calendar.SequoiaSettingsView {'))
    for name in ('ViewSettingsPage', 'SourceSettingsPage', 'FreeBusySettingsPage'):
        path = qml / 'Settings' / (name + '.qml')
        text = path.read_text().replace('import QtQuick\n', 'import QtQuick\nimport QtQuick as Quick\n' + shared + 'import org.kde.merkuro.calendar as GlassSettings\n')
        if 'import QtQuick.Controls as QQC2' not in text:
            text = text.replace('import QtQuick\n', 'import QtQuick\nimport QtQuick.Controls as QQC2\n', 1)
        # All native models, bindings, ranges and save/action handlers remain.
        text = replace(text, 'FormCard.FormCardPage {', '''FormCard.FormCardPage {
    objectName: "sequoiaSettingsPage-''' + name + '''"
    background: Quick.Item {}
    topPadding: 12
    bottomPadding: 20
    leftPadding: 16
    rightPadding: 16''')
        text = replace(text, '    background: Quick.Item {}', '''    background: Quick.Item {}
    property GlassSettings.SequoiaSettingsScrollEdges scrollEdges: GlassSettings.SequoiaSettingsScrollEdges {
        page: __SEQUOIA_SETTINGS_PAGE__
    }
    property GlassSettings.SequoiaSettingsPageMemory scrollMemory: GlassSettings.SequoiaSettingsPageMemory {
        page: __SEQUOIA_SETTINGS_PAGE__
        positionKey: "__SEQUOIA_SETTINGS_KEY__"
    }
    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None
    header: QQC2.Control {
        objectName: "sequoiaSettingsHeading"
        implicitHeight: 72
        leftPadding: Math.max(16, (width - Kirigami.Units.gridUnit * 30) / 2)
        rightPadding: leftPadding
        topPadding: 20
        bottomPadding: 12
        background: Quick.Item {}
        contentItem: Kirigami.Heading {
            text: __SEQUOIA_SETTINGS_TITLE__
            level: 1
            font.pixelSize: 24
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }'''.replace('__SEQUOIA_SETTINGS_TITLE__', 'freeBusySettingsPage.title' if name == 'FreeBusySettingsPage' else 'root.title')
           .replace('__SEQUOIA_SETTINGS_PAGE__', 'freeBusySettingsPage' if name == 'FreeBusySettingsPage' else 'root'))
        text = text.replace('__SEQUOIA_SETTINGS_KEY__', {'ViewSettingsPage': 'appearance', 'SourceSettingsPage': 'users', 'FreeBusySettingsPage': 'freebusy'}[name])
        for old, new in (('FormCard.FormCard', 'GlassSettings.SequoiaSettingsCard'),
                         ('FormCard.FormComboBoxDelegate', 'GlassSettings.SequoiaSettingsCombo'),
                         ('FormCard.FormSpinBoxDelegate', 'GlassSettings.SequoiaSettingsSpin'),
                         ('FormCard.FormTextFieldDelegate', 'GlassSettings.SequoiaSettingsField'),
                         ('FormCard.FormPasswordFieldDelegate', 'GlassSettings.SequoiaSettingsPassword'),
                         ('AgentConfigurationForm', 'GlassSettings.SequoiaSettingsAccounts'),
                         ('SystemAccountsForm', 'GlassSettings.SequoiaSettingsSystemAccounts'),
                         ('QQC2.Button', 'S.ActionPill'), ('QQC2.SpinBox', 'S.SpinPill'),
                         ('QQC2.Slider', 'S.SliderPill'), ('QQC2.TextField', 'S.TextFieldPill')):
            text = text.replace(old + ' {', new + ' {')
        if name == 'ViewSettingsPage':
            # RoundedItemDelegate is checkable, but has no CheckDelegate-only
            # tristate/checkState properties. Keep the native toggle handler.
            text = replace(text, 'checkState: delegate.checkState', 'checkState: delegate.checked ? Qt.Checked : Qt.Unchecked')
            text = replace(text, 'nextCheckState: delegate.nextCheckState', 'nextCheckState: () => checked ? Qt.Unchecked : Qt.Checked')
            text = replace(text, 'tristate: delegate.tristate', 'tristate: false')
        path.write_text(text)


def install_position_memory(config_dir):
    """Let KWin remember this dialog's position, including on Wayland."""
    import subprocess
    path = Path(config_dir) / 'kwinrulesrc'
    rule = 'sequoia-merkuro-settings-position'
    original = path.read_text() if path.exists() else ''
    if '[' + rule + ']' in original:
        return False
    # KWin Rule::Remember = 4; RegExpMatch = 3. No initial position is
    # forced; KWin learns it when the settings window is moved/closed.
    entries = {'Description': 'Merkuro Settings position',
               'wmclass': r'^(org\.kde\.merkuro(?:\.calendar)?|merkuro-calendar)$',
               'wmclassmatch': '3', 'wmclasscomplete': 'false',
               'title': r'^Merkuro Settings(?: .*Merkuro Calendar)?$',
               'titlematch': '3', 'positionrule': '4'}
    for key, value in entries.items():
        subprocess.run(['kwriteconfig6', '--file', str(path), '--group', rule, '--key', key, value], check=True)
    order = subprocess.check_output(['kreadconfig6', '--file', str(path), '--group', 'General', '--key', 'Order'], text=True).strip()
    subprocess.run(['kwriteconfig6', '--file', str(path), '--group', 'General', '--key', 'Order', ','.join(filter(None, [order, rule]))], check=True)
    return True
