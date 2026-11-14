// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

Item {
    required property Item field
    visible: false
    // Both Plasma and Kirigami SearchField expose the magnifier as a direct
    // child. Their text padding already follows its width.
    readonly property var searchIcon: field.children.find(child => child.source !== undefined && String(child.source) === "search") ?? null
    readonly property real iconSize: [Kirigami.Units.iconSizes.small, Kirigami.Units.iconSizes.smallMedium,
        Kirigami.Units.iconSizes.medium, Kirigami.Units.iconSizes.large,
        Kirigami.Units.iconSizes.huge, Kirigami.Units.iconSizes.enormous]
        .filter(size => size > Kirigami.Units.iconSizes.sizeForLabels)[1] ?? Kirigami.Units.iconSizes.enormous
    Binding { target: searchIcon; property: "implicitWidth"; value: iconSize; when: searchIcon !== null }
    Binding { target: searchIcon; property: "implicitHeight"; value: iconSize; when: searchIcon !== null }
}
