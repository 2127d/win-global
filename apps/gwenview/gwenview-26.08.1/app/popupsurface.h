// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <QApplication>
#include <QWidget>

namespace Gwenview {
inline QPalette popupMaterialPalette(QPalette palette)
{
    // Match SequoiaEleven's native menu material: Transparent presets use 80%,
    // Solid presets 100%. Use the desktop palette, not an image-view or submenu
    // palette, so all popup kinds follow live desktop material changes.
    const int alpha = palette.color(QPalette::Window).alpha() < 255 ? 204 : 255;
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        QColor body = palette.color(group, QPalette::Base); body.setAlpha(alpha);
        for (auto role : {QPalette::Window, QPalette::Base, QPalette::Button})
            if (palette.color(group, role) != body) palette.setColor(group, role, body);
    }
    return palette;
}
inline QRectF popupBodyRect(const QWidget *popup)
{
    return QRectF(popup->rect()).adjusted(14, 14, -14, -18);
}
inline QRegion popupBlurRegion(const QWidget *popup, qreal offset = 0)
{
    if (popup->palette().color(QPalette::Window).alpha() < 255) {
        QPainterPath rounded;
        rounded.addRoundedRect(popupBodyRect(popup).translated(0, offset).adjusted(.5, .5, -.5, -.5), 11.5, 11.5);
        const QRegion region = QRegion(rounded.toFillPolygon().toPolygon()) & popup->rect();
        if (!region.isEmpty()) return region;
    }
    // Preserve KDE's surface-lifetime tracking even for solid/empty menus.
    // An empty region means whole-window blur; false hits the KDE 6.31 bug.
    return QRegion(QRect(-1, -1, 1, 1));
}
inline void paintPopupBody(QPainter &painter, const QWidget *popup)
{
    paintPillSurface(painter, popupBodyRect(popup), 12, popup->palette());
}
inline void paintPopupSurface(QPainter &painter, const QWidget *popup)
{
    paintFloatingShadow(painter, popupBodyRect(popup), 12, popup->palette(), true, 1.15);
    paintPopupBody(painter, popup);
}
}
