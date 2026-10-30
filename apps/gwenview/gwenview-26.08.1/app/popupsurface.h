// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <QApplication>
#include <QWidget>

namespace Gwenview {
inline QPalette popupMaterialPalette(QPalette palette)
{
    // Weather uses the desktop Window material unchanged, including its alpha.
    // All popup kinds follow that same live palette rather than inventing a
    // second opacity or taking the image viewer's local background color.
    const int alpha = palette.color(QPalette::Window).alpha();
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        QColor body = palette.color(group, QPalette::Window); body.setAlpha(alpha);
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
        rounded.addRoundedRect(popupBodyRect(popup).translated(0, offset).adjusted(.5, .5, -.5, -.5), Sequoia::SurfaceRadius - .5, Sequoia::SurfaceRadius - .5);
        const QRegion region = QRegion(rounded.toFillPolygon().toPolygon()) & popup->rect();
        if (!region.isEmpty()) return region;
    }
    // Preserve KDE's surface-lifetime tracking even for solid/empty menus.
    // An empty region means whole-window blur; false hits the KDE 6.31 bug.
    return QRegion(QRect(-1, -1, 1, 1));
}
inline void paintPopupBody(QPainter &painter, const QWidget *popup)
{
    paintPillSurface(painter, popupBodyRect(popup), Sequoia::SurfaceRadius, popup->palette());
}
inline void paintPopupSurface(QPainter &painter, const QWidget *popup)
{
    paintFloatingShadow(painter, popupBodyRect(popup), Sequoia::SurfaceRadius, popup->palette(), true, 1.15);
    paintPopupBody(painter, popup);
}
}
