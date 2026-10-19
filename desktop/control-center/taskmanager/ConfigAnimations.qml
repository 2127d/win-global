// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

KCMUtils.SimpleKCM {
    id: root
    objectName: "sequoiaAnimationSettings"
    property bool cfg_dockAnimations: true
    property bool cfg_taskHoverEffect: true
    property int cfg_dockHoverStyle: 2
    property int cfg_dockStrength: 12
    property int cfg_dockSpeed: 100
    property bool cfg_dockLaunchAnimation: true
    property bool cfg_dockPressAnimation: true
    property bool cfg_dockAttentionAnimation: true
    property bool cfg_dockReorderAnimation: true
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        SettingsCard {
            title: i18n("Motion")
            QQC2.Switch {
                text: i18n("Enable dock animations")
                checked: root.cfg_dockAnimations
                onToggled: root.cfg_dockAnimations = checked
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("Make the dock feel responsive with soft zoom, lift and launch feedback. Motion follows the desktop animation speed, including reduced motion.")
                wrapMode: Text.Wrap
                opacity: 0.7
            }
            Item {
                Layout.fillWidth: true
                implicitHeight: 92
                Rectangle {
                    anchors.centerIn: parent
                    width: 180; height: 70; radius: 24
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.04)
                    Kirigami.Icon {
                        anchors.centerIn: parent
                        width: 44; height: 44
                        source: "system-file-manager"
                        scale: previewMotion.zoom
                        transform: Translate { y: previewMotion.offset }
                    }
                    HoverHandler { id: hover }
                    TapHandler { id: press; onTapped: launchPreview.restart() }
                    Timer { id: launchPreview; interval: 1800 }
                    DockMotion {
                        id: previewMotion
                        animationsEnabled: root.cfg_dockAnimations
                        hovered: hover.hovered && root.cfg_taskHoverEffect
                        pressed: press.pressed
                        launching: launchPreview.running
                        launchEnabled: root.cfg_dockLaunchAnimation
                        pressEnabled: root.cfg_dockPressAnimation
                        hoverStyle: root.cfg_dockHoverStyle
                        strength: root.cfg_dockStrength
                        speed: root.cfg_dockSpeed
                    }
                    Accessible.name: i18n("Animation preview")
                }
            }
            QQC2.Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: i18n("Hover or click to preview"); opacity: 0.7 }
        }
        SettingsCard {
            title: i18n("Hover & feel")
            enabled: root.cfg_dockAnimations
            Kirigami.FormLayout {
                Layout.fillWidth: true
                QQC2.CheckBox {
                    text: i18n("Animate icons on hover")
                    checked: root.cfg_taskHoverEffect
                    onToggled: root.cfg_taskHoverEffect = checked
                }
                QQC2.ComboBox {
                    Kirigami.FormData.label: i18n("Hover style:")
                    enabled: root.cfg_taskHoverEffect
                    model: [i18n("Zoom"), i18n("Lift"), i18n("Zoom & lift")]
                    currentIndex: root.cfg_dockHoverStyle
                    onActivated: root.cfg_dockHoverStyle = currentIndex
                }
                QQC2.SpinBox {
                    Kirigami.FormData.label: i18n("Intensity (%):")
                    from: 0; to: 20; stepSize: 1
                    value: root.cfg_dockStrength
                    onValueModified: root.cfg_dockStrength = value
                }
                QQC2.SpinBox {
                    Kirigami.FormData.label: i18n("Speed (%):")
                    from: 50; to: 200; stepSize: 10
                    value: root.cfg_dockSpeed
                    onValueModified: root.cfg_dockSpeed = value
                }
            }
        }
        SettingsCard {
            title: i18n("Feedback")
            enabled: root.cfg_dockAnimations
            QQC2.CheckBox { text: i18n("Bounce while an app starts"); checked: root.cfg_dockLaunchAnimation; onToggled: root.cfg_dockLaunchAnimation = checked }
            QQC2.CheckBox { text: i18n("Press feedback"); checked: root.cfg_dockPressAnimation; onToggled: root.cfg_dockPressAnimation = checked }
            QQC2.CheckBox { text: i18n("Pulse the indicator when an app needs attention"); checked: root.cfg_dockAttentionAnimation; onToggled: root.cfg_dockAttentionAnimation = checked }
            QQC2.CheckBox { text: i18n("Animate task reordering"); checked: root.cfg_dockReorderAnimation; onToggled: root.cfg_dockReorderAnimation = checked }
        }
    }
}
