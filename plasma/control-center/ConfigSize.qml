// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

KCMUtils.SimpleKCM {
    id: root
    objectName: "sequoiaSizeSettings"
    property int cfg_controlCenterWidth: 297
    property int cfg_controlCenterHeight: 451
    property int cfg_controlCenterWidthDefault: 297
    property int cfg_controlCenterHeightDefault: 451

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Set the size used when Control Center opens. Sound, Network, and other menus keep their own sizes.")
            wrapMode: Text.WordWrap
        }
        Kirigami.FormLayout {
            Layout.fillWidth: true
            QQC2.SpinBox {
                objectName: "controlCenterWidthInput"
                Kirigami.FormData.label: i18n("Width (px):")
                Accessible.name: i18n("Control Center width")
                from: 260
                to: 1600
                editable: true
                value: root.cfg_controlCenterWidth
                onValueModified: root.cfg_controlCenterWidth = value
            }
            QQC2.SpinBox {
                objectName: "controlCenterHeightInput"
                Kirigami.FormData.label: i18n("Height (px):")
                Accessible.name: i18n("Control Center height")
                from: 240
                to: 1600
                editable: true
                value: root.cfg_controlCenterHeight
                onValueModified: root.cfg_controlCenterHeight = value
            }
            QQC2.Button {
                text: i18n("Reset to Default Size")
                icon.name: "edit-undo"
                enabled: root.cfg_controlCenterWidth !== root.cfg_controlCenterWidthDefault
                    || root.cfg_controlCenterHeight !== root.cfg_controlCenterHeightDefault
                onClicked: {
                    root.cfg_controlCenterWidth = root.cfg_controlCenterWidthDefault;
                    root.cfg_controlCenterHeight = root.cfg_controlCenterHeightDefault;
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("The popup is kept within the current screen. Smaller heights let you scroll through the controls.")
            wrapMode: Text.WordWrap
            opacity: 0.7
        }
    }
}
