// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import "qrc:/sequoia-weather/ActionIcons.js" as ActionIcons
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.coreaddons as Core
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kweather

Basic.ScrollView {
    id: root
    objectName: "weatherAboutView"
    readonly property var aboutData: Core.AboutData
    implicitWidth: 540
    implicitHeight: 500
    contentWidth: availableWidth
    clip: true
    background: null
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    component Card: Pane {
        Layout.fillWidth: true
        padding: 20
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        background: Kirigami.ShadowedRectangle {
            radius: 22
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.13)
            shadow.size: Kirigami.Units.largeSpacing
            shadow.color: Qt.alpha(Kirigami.Theme.textColor, 0.10)
        }
    }
    component SectionTitle: Label {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.topMargin: 6
        font.bold: true
        wrapMode: Text.WordWrap
    }
    component Body: Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        textFormat: Text.PlainText
    }
    component ActionButton: Button {
        id: actionButton
        Layout.fillWidth: true
        implicitHeight: Math.max(40, actionContent.implicitHeight + 20)
        padding: 10
        contentItem: RowLayout {
            id: actionContent
            spacing: 10
            Kirigami.Icon {
                source: actionButton.icon.source
                visible: source.toString().length > 0
                Layout.preferredWidth: 18; Layout.preferredHeight: 18
                isMask: true
            }
            Label {
                text: actionButton.text
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        background: Rectangle {
            radius: 14
            color: Qt.alpha(Kirigami.Theme.highlightColor, parent.down ? 0.20 : parent.hovered ? 0.12 : 0.06)
            border.width: parent.visualFocus ? 2 : 0
            border.color: Kirigami.Theme.highlightColor
            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
        }
    }
    component LicenseEntry: ColumnLayout {
        id: entry
        required property string licenseName
        required property string licenseText
        Layout.fillWidth: true
        spacing: 10
        ActionButton {
            objectName: "weatherAboutLicense"
            text: entry.licenseName
            checkable: true
            icon.source: checked ? ActionIcons.source('go-up-symbolic') : ActionIcons.source('go-down-symbolic')
            onToggled: licenseBody.visible = checked
        }
        TextArea {
            id: licenseBody
            visible: false
            Layout.fillWidth: true
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.PlainText
            text: entry.licenseText
            background: null
        }
    }
    component People: ColumnLayout {
        required property var people
        Layout.fillWidth: true
        spacing: 16
        Repeater {
            model: people
            ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 4
                Body { text: modelData.name; font.bold: true }
                Body { text: modelData.task; visible: text.length > 0; color: Kirigami.Theme.disabledTextColor }
                ActionButton {
                    visible: !!modelData.webAddress
                    text: i18n("Website")
                    icon.source: ActionIcons.source('globe-symbolic')
                    onClicked: Qt.openUrlExternally(modelData.webAddress)
                }
                ActionButton {
                    visible: !!modelData.emailAddress
                    text: modelData.emailAddress || ""
                    icon.source: ActionIcons.source('mail-sent-symbolic')
                    onClicked: Qt.openUrlExternally("mailto:" + modelData.emailAddress)
                }
            }
        }
    }

    ColumnLayout {
        width: Math.min(540, root.availableWidth - 32)
        x: (root.availableWidth - width) / 2
        spacing: 10
        Item { implicitHeight: 12 }
        Card {
            contentItem: ColumnLayout {
                spacing: 12
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 72; Layout.preferredHeight: 72
                    radius: 20
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#42b8f2" }
                        GradientStop { position: 1; color: "#1762c4" }
                    }
                    WeatherGlyph {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: 4
                        width: 68; height: 60
                        condition: "weather-few-clouds"
                    }
                }
                Body {
                    text: root.aboutData.displayName
                    font.pixelSize: 26; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
                Body {
                    text: i18n("Sequoia appearance · KDE Weather")
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                }
                Body {
                    text: i18n("Version %1", root.aboutData.version)
                    horizontalAlignment: Text.AlignHCenter
                }
                Body {
                    text: root.aboutData.shortDescription
                    horizontalAlignment: Text.AlignHCenter
                }
                Body {
                    text: root.aboutData.copyrightStatement
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        Card {
            visible: root.aboutData.otherText.length > 0
            contentItem: Body { text: root.aboutData.otherText }
        }
        SectionTitle { text: i18n("Project & support") }
        Card {
            contentItem: ColumnLayout {
                spacing: 8
                ActionButton {
                    visible: root.aboutData.homepage.length > 0
                    text: i18n("Homepage"); icon.source: ActionIcons.source('globe-symbolic')
                    onClicked: Qt.openUrlExternally(root.aboutData.homepage)
                }
                ActionButton {
                    text: i18n("Report a bug"); icon.source: ActionIcons.source('tools-report-bug-symbolic')
                    visible: root.aboutData.bugAddress.length > 0
                    onClicked: Qt.openUrlExternally(root.aboutData.bugAddress)
                }
                ActionButton {
                    text: i18n("Get involved"); icon.source: ActionIcons.source('applications-development-symbolic')
                    onClicked: Qt.openUrlExternally("https://community.kde.org/Get_Involved")
                }
                ActionButton {
                    text: i18n("Support KDE"); icon.source: ActionIcons.source('donate-symbolic')
                    onClicked: Qt.openUrlExternally("https://kde.org/donate/?app=" + encodeURIComponent(root.aboutData.componentName))
                }
            }
        }
        SectionTitle { text: i18n("License"); visible: root.aboutData.licenses.length > 0 }
        Card {
            visible: root.aboutData.licenses.length > 0
            contentItem: ColumnLayout {
                Repeater {
                    model: root.aboutData.licenses
                    LicenseEntry {
                        required property var modelData
                        licenseName: modelData.name
                        licenseText: modelData.text
                    }
                }
            }
        }
        SectionTitle { text: i18n("Authors"); visible: root.aboutData.authors.length > 0 }
        Card {
            visible: root.aboutData.authors.length > 0
            contentItem: People { people: root.aboutData.authors }
        }
        SectionTitle { text: i18n("Credits"); visible: root.aboutData.credits.length > 0 }
        Card {
            visible: root.aboutData.credits.length > 0
            contentItem: People { people: root.aboutData.credits }
        }
        SectionTitle { text: i18n("Translators"); visible: root.aboutData.translators.length > 0 }
        Card {
            visible: root.aboutData.translators.length > 0
            contentItem: People { people: root.aboutData.translators }
        }
        SectionTitle { text: i18n("Libraries in use") }
        Card {
            contentItem: ColumnLayout {
                spacing: 12
                ActionButton {
                    text: i18n("Copy system information"); icon.source: ActionIcons.source('edit-copy-symbolic')
                    onClicked: {
                        FormCard.AboutComponent.copyToClipboard();
                        text = i18n("Copied");
                    }
                }
                Repeater {
                    model: FormCard.AboutComponent.components
                    ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 6
                        Body { text: modelData.name + " " + modelData.version; font.bold: true }
                        Body { text: modelData.description; visible: text.length > 0; color: Kirigami.Theme.disabledTextColor }
                        LicenseEntry {
                            visible: modelData.licenses.key !== 0
                            licenseName: modelData.licenses.name
                            licenseText: modelData.licenses.text
                        }
                        ActionButton {
                            visible: !!modelData.webAddress
                            text: i18n("Website"); icon.source: ActionIcons.source('globe-symbolic')
                            onClicked: Qt.openUrlExternally(modelData.webAddress)
                        }
                    }
                }
            }
        }
        Item { implicitHeight: 16 }
    }
}
