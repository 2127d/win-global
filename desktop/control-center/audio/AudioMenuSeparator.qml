// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami

QQC.MenuSeparator {
    topPadding: 4; bottomPadding: 4; leftPadding: 12; rightPadding: 12
    implicitHeight: 9
    contentItem: Rectangle { implicitHeight: 1; color: Qt.alpha(Kirigami.Theme.textColor,0.12) }
}
