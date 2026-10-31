// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <QApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>
#include <qpa/qplatformwindow_p.h>
#include <private/qhighdpiscaling_p.h>

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
inline void resetDropdownAnchor(QWidget *popup)
{
#if QT_CONFIG(wayland) && QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    auto window = popup->windowHandle();
    if (!window || !window->property("gwenviewDropdownAnchor").toBool()) return;
    for (auto property : {"_q_waylandPopupAnchorRect", "_q_waylandPopupAnchor", "_q_waylandPopupGravity", "_q_waylandPopupConstraintAdjustment"})
        window->setProperty(property, QVariant());
    if (auto native = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>()) native->setCustomMargins(QMargins());
    window->setProperty("gwenviewDropdownAnchor", false);
#endif
}
inline void positionDropdown(QWidget *popup, QWidget *field)
{
    // Anchor the visible material, rather than its invisible shadow margin.
    const QRect body = popupBodyRect(popup).toRect();
    const QRect anchor(field->mapToGlobal(QPoint()), field->size());
    const QRect available = field->screen()->availableGeometry();
    constexpr int gap = 6;
#if QT_CONFIG(wayland) && QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    // Qt Wayland anchors menus to their parent control, overriding QWidget's
    // guessed global position. Describe the visible surface and anchor in the
    // transient parent's coordinates instead of moving only the Qt widget.
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        if (!popup->windowHandle()) popup->winId();
        auto window = popup->windowHandle();
        if (auto native = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>()) {
            window->setProperty("gwenviewDropdownAnchor", true);
            native->setCustomMargins(QHighDpi::toNativePixels(QMargins(14, 14, 14, 18), window));
            auto parent = window->transientParent();
            if (!parent) parent = field->window()->windowHandle();
            const QPoint origin = parent->mapFromGlobal(field->mapToGlobal(QPoint()));
            window->setProperty("_q_waylandPopupAnchorRect", QHighDpi::toNativeLocalPosition(QRect(origin, field->size()).adjusted(0, -gap, 0, gap), parent));
            window->setProperty("_q_waylandPopupAnchor", QVariant::fromValue(Qt::Edges(Qt::BottomEdge | (field->isRightToLeft() ? Qt::RightEdge : Qt::LeftEdge))));
            window->setProperty("_q_waylandPopupGravity", QVariant::fromValue(Qt::Edges(Qt::BottomEdge | (field->isRightToLeft() ? Qt::LeftEdge : Qt::RightEdge))));
            // Let the compositor flip above the field or slide within the
            // actual output, including desktops whose outputs have offsets.
            window->setProperty("_q_waylandPopupConstraintAdjustment", 1u | 2u | 8u);
        }
    }
#endif
    int x = field->isRightToLeft() ? anchor.right() - body.right() : anchor.left() - body.left();
    int y = anchor.bottom() + 1 + gap - body.top();
    if (y + body.bottom() > available.bottom()
        && anchor.top() - gap - body.height() >= available.top())
        y = anchor.top() - gap - body.height() - body.top();
    x = qBound(available.left(), x, qMax(available.left(), available.right() - popup->width() + 1));
    y = qBound(available.top(), y, qMax(available.top(), available.bottom() - popup->height() + 1));
    popup->move(x, y);
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
