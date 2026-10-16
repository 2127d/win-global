// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page
    implicitWidth: Kirigami.Units.gridUnit * 36
    implicitHeight: Kirigami.Units.gridUnit * 38
    readonly property var settings: KCM.ConfigModule.settings
    readonly property var values: settings.values
    Connections {
        target: KCM.ConfigModule
        function onErrorOccurred(message) { error.text = message; error.visible = true; }
    }
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            text: i18n("These settings apply to all Sequoia Taskbar panels. Add one from the desktop’s Add Panel menu. Use Edit Mode to position the panel, change its height, or add widgets.")
        }
        Kirigami.InlineMessage { id: error; Layout.fillWidth: true; type: Kirigami.MessageType.Error; visible: false }
        Kirigami.FormLayout {
            Layout.fillWidth: true
            QQC2.ComboBox {
                id: mode
                implicitContentWidthPolicy: QQC2.ComboBox.WidestText
                objectName: "sizeModeControl"
                Kirigami.FormData.label: i18n("Icon sizing:")
                textRole: "text"
                valueRole: "value"
                model: [
                    {text: i18n("Keep icons the same size"), value: "fixed"},
                    {text: i18n("Fill available space"), value: "fill"},
                    {text: i18n("Resize with panel"), value: "panel"}
                ]
                currentIndex: Math.max(0, indexOfValue(page.values.sizeMode))
                onActivated: page.settings.setValue("sizeMode", currentValue)
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 25
                wrapMode: Text.WordWrap
                text: page.values.sizeMode === "fill"
                    ? i18n("Buttons spread across the available task area; icons keep your chosen size.")
                    : page.values.sizeMode === "panel"
                    ? i18n("Icons grow and shrink with panel thickness, after subtracting padding.")
                    : i18n("Icons keep your chosen size. When space runs out, the task area scrolls.")
            }
            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Icon size (px):")
                from: 16; to: 128; stepSize: 2; editable: true
                enabled: page.values.sizeMode !== "panel"
                value: page.values.iconSize
                onValueModified: page.settings.setValue("iconSize", value)
            }
            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Space between buttons (px):")
                from: 0; to: 64; editable: true
                value: page.values.spacing
                onValueModified: page.settings.setValue("spacing", value)
            }
            QQC2.SpinBox {
                Kirigami.FormData.label: i18n("Padding around icons (px):")
                from: 0; to: 32; editable: true
                value: page.values.padding
                onValueModified: page.settings.setValue("padding", value)
            }
            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Task alignment:")
                model: [i18n("Start"), i18n("Center"), i18n("End")]
                currentIndex: ["left", "center", "right"].indexOf(page.values.alignment)
                enabled: page.values.sizeMode !== "fill"
                onActivated: page.settings.setValue("alignment", ["left", "center", "right"][currentIndex])
            }
            QQC2.CheckBox {
                text: i18n("Show application menu")
                checked: page.values.showLauncher
                onToggled: page.settings.setValue("showLauncher", checked)
            }
            QQC2.CheckBox {
                text: i18n("Show running application indicators")
                checked: page.values.showIndicators
                onToggled: page.settings.setValue("showIndicators", checked)
            }
            QQC2.CheckBox {
                text: i18n("Only show windows on this screen")
                checked: page.values.currentScreenOnly
                onToggled: page.settings.setValue("currentScreenOnly", checked)
            }
            QQC2.CheckBox {
                text: i18n("Only show windows on the current desktop")
                checked: page.values.currentDesktopOnly
                onToggled: page.settings.setValue("currentDesktopOnly", checked)
            }
        }
        Kirigami.Heading { text: i18n("Pinned applications"); level: 2 }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Right-click a task button to pin or unpin it. You can also reorder this list or add an application URL, such as applications:org.kde.dolphin.desktop or preferred://browser.")
        }
        ColumnLayout {
            objectName: "pinnedApplicationsControl"
            Layout.fillWidth: true
            Repeater {
                model: page.values.launchers
                delegate: RowLayout {
                    id: pinRow
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    function move(offset) {
                        const entries = page.values.launchers.slice();
                        const item = entries.splice(index, 1)[0];
                        entries.splice(index + offset, 0, item);
                        page.settings.setValue("launchers", entries);
                    }
                    QQC2.Label { Layout.fillWidth: true; text: pinRow.modelData; elide: Text.ElideMiddle }
                    QQC2.ToolButton {
                        text: i18n("Up")
                        icon.name: "go-up"; enabled: pinRow.index > 0
                        Accessible.name: i18n("Move launcher earlier")
                        onClicked: pinRow.move(-1)
                    }
                    QQC2.ToolButton {
                        text: i18n("Down")
                        icon.name: "go-down"; enabled: pinRow.index < page.values.launchers.length - 1
                        Accessible.name: i18n("Move launcher later")
                        onClicked: pinRow.move(1)
                    }
                    QQC2.ToolButton {
                        text: i18n("Remove")
                        icon.name: "list-remove"
                        Accessible.name: i18n("Remove launcher")
                        onClicked: {
                            const entries = page.values.launchers.slice();
                            entries.splice(pinRow.index, 1);
                            page.settings.setValue("launchers", entries);
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                QQC2.TextField {
                    id: newLauncher
                    Layout.fillWidth: true
                    placeholderText: "applications:org.kde.kate.desktop"
                    Accessible.name: i18n("Application URL to pin")
                    onAccepted: if (addLauncher.enabled) addLauncher.clicked()
                }
                QQC2.Button {
                    id: addLauncher
                    text: i18n("Add")
                    enabled: /^(applications:.+\.desktop|preferred:\/\/[^/ ]+|file:\/\/.+\.desktop)$/.test(newLauncher.text.trim())
                    onClicked: {
                        page.settings.setValue("launchers", page.values.launchers.concat([newLauncher.text.trim()]));
                        newLauncher.clear();
                    }
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Sizes use logical pixels and follow display scaling. An icon is reduced only if the panel is too thin to contain it. Added widgets retain their own sizing settings.")
        }
    }
}
