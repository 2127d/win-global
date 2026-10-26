// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QtMath>

namespace Gwenview {
inline constexpr int FloatingPanelInset = 7;
inline constexpr int FloatingPanelPadding = FloatingPanelInset + 8;
// Paint only the surface's shadow, never blur its contents. The compact card
// shadow fits the compact panel gutter; floating controls use a softer 16px halo.
inline void paintFloatingShadow(QPainter &painter, const QRectF &bounds, qreal radius,
                                const QPalette &palette, bool elevated = false, qreal opacity = 1)
{
    if (bounds.isEmpty() || opacity <= 0) return;
    const int spread = elevated ? 12 : 6;
    const qreal offset = elevated ? 4 : 2;
    const qreal sigma = elevated ? 4.5 : 2.5;
    const bool dark = palette.color(QPalette::Window).lightness() < 128;
    const qreal strength = (dark ? 0.32 : 0.20) * opacity;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    // Keep glass clear: shadow ink belongs outside the actual surface.
    QPainterPath outside;
    outside.setFillRule(Qt::OddEvenFill);
    outside.addRect(bounds.adjusted(-16, -16, 16, 16));
    outside.addRoundedRect(bounds, radius, radius);
    painter.setClipPath(outside, Qt::IntersectClip);
    painter.setPen(Qt::NoPen);
    for (int i = spread; i >= 0; --i) {
        const qreal alpha = strength * 0.18 * qExp(-qreal(i * i) / (2 * sigma * sigma));
        painter.setBrush(QColor::fromRgbF(0, 0, 0, alpha));
        painter.drawRoundedRect(bounds.translated(0, offset).adjusted(-i, -i, i, i), radius + i, radius + i);
    }
    painter.restore();
}

inline void paintFloatingPanel(QPainter &painter, const QRectF &bounds, const QPalette &palette, bool withShadow = true)
{
    constexpr qreal radius = 16;
    if (withShadow) paintFloatingShadow(painter, bounds, radius, palette);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    QColor surface = palette.color(QPalette::Button);
    surface.setAlpha(255);
    const bool dark = surface.lightness() < 128;
    painter.setPen(QPen(QColor(255, 255, 255, dark ? 20 : 110), 1));
    painter.setBrush(surface);
    painter.drawRoundedRect(bounds.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    painter.restore();
}
}
