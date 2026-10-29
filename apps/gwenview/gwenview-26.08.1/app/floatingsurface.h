// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QImage>
#include <QPixmapCache>
#include <QtMath>

namespace Gwenview {
inline constexpr int FloatingPanelInset = 7;
inline constexpr int FloatingPanelPadding = FloatingPanelInset + 8;
// Render decorative surfaces at twice the device resolution, then resolve once.
// Keep text/icons native and cache the small surface, so hovering and animation
// reuse it rather than allocating a high-resolution layer on every frame.
template<typename Paint>
inline void paintSmoothSurface(QPainter &painter, const QRectF &area, const QString &key, Paint paint)
{
    if (area.isEmpty()) return;
    const QRect box = area.toAlignedRect();
    const qreal dpr = painter.device()->devicePixelRatioF();
    const QString cacheKey = QStringLiteral("gwenview-surface-v2/%1/%2/%3/%4/%5/%6")
        .arg(key).arg(dpr).arg(box.x()).arg(box.y()).arg(box.width()).arg(box.height());
    QPixmap cached;
    if (!QPixmapCache::find(cacheKey, &cached)) {
        const QSize pixels(qCeil(box.width() * dpr), qCeil(box.height() * dpr));
        QImage layer(pixels * 2, QImage::Format_ARGB32_Premultiplied);
        layer.setDevicePixelRatio(dpr * 2);
        layer.fill(Qt::transparent);
        {
            QPainter smooth(&layer);
            smooth.setRenderHint(QPainter::Antialiasing);
            smooth.setRenderHint(QPainter::SmoothPixmapTransform);
            smooth.translate(-box.topLeft());
            paint(smooth);
        }
        layer = layer.scaled(pixels, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        layer.setDevicePixelRatio(dpr);
        cached = QPixmap::fromImage(layer);
        QPixmapCache::insert(cacheKey, cached);
    }
    painter.drawPixmap(box.topLeft(), cached);
}

inline QString surfaceGeometryKey(const QRectF &bounds, qreal radius)
{
    return QStringLiteral("%1,%2,%3,%4,%5").arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height()).arg(radius);
}

// Shared glass/solid material for normal and fullscreen controls. Only the
// backdrop is clipped; labels, icons and previews are never rasterized here.
inline void paintPillSurface(QPainter &painter, const QRectF &bounds, qreal radius, const QPalette &palette,
                             const QImage &backdrop = QImage(), const QRectF &backdropRect = QRectF(), bool translucent = false)
{
    if (bounds.isEmpty()) return;
    radius = qMin(radius, qMin(bounds.width(), bounds.height()) / 2);
    const bool dark = palette.color(QPalette::Window).lightness() < 128;
    const QString key = QStringLiteral("pill/%1/%2/%3/%4/%5")
        .arg(surfaceGeometryKey(bounds, radius)).arg(palette.cacheKey()).arg(backdrop.cacheKey())
        .arg(surfaceGeometryKey(backdropRect, 0)).arg(translucent);
    paintSmoothSurface(painter, bounds.adjusted(-1, -1, 1, 1), key, [&](QPainter &smooth) {
        if (!backdrop.isNull()) {
            QPainterPath clip; clip.addRoundedRect(bounds, radius, radius);
            smooth.save(); smooth.setClipPath(clip);
            smooth.drawImage(backdropRect, backdrop); smooth.restore();
        }
        QLinearGradient fill(bounds.topLeft(), bounds.bottomLeft());
        QColor top = palette.color(QPalette::Window).lighter(dark ? 113 : 102);
        QColor bottom = palette.color(QPalette::Window);
        if (translucent || !backdrop.isNull()) {
            top.setAlpha(dark ? 205 : 195); bottom.setAlpha(dark ? 185 : 175);
        }
        fill.setColorAt(0, top); fill.setColorAt(1, bottom);
        smooth.setPen(Qt::NoPen); smooth.setBrush(fill);
        smooth.drawRoundedRect(bounds, radius, radius);
        // Inset strokes stay inside the capsule and retain even coverage at
        // fractional desktop scales instead of straddling a clipped boundary.
        smooth.setBrush(Qt::NoBrush);
        smooth.setPen(QPen(QColor(0, 0, 0, dark ? 65 : 24), 1));
        smooth.drawRoundedRect(bounds.adjusted(.5, .5, -.5, -.5), qMax(0.0, radius - .5), qMax(0.0, radius - .5));
        QLinearGradient rim(bounds.topLeft(), bounds.bottomLeft());
        rim.setColorAt(0, QColor(255, 255, 255, dark ? 40 : 90));
        rim.setColorAt(1, QColor(255, 255, 255, dark ? 12 : 35));
        smooth.setPen(QPen(QBrush(rim), 1));
        smooth.drawRoundedRect(bounds.adjusted(1.5, 1.5, -1.5, -1.5), qMax(0.0, radius - 1.5), qMax(0.0, radius - 1.5));
    });
}
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
    const QString key = QStringLiteral("shadow/%1/%2/%3/%4")
        .arg(surfaceGeometryKey(bounds, radius)).arg(dark).arg(elevated).arg(opacity);
    paintSmoothSurface(painter, bounds.adjusted(-16, -16, 16, 16), key, [&](QPainter &smooth) {
        smooth.setPen(Qt::NoPen);
        for (int i = spread; i >= 0; --i) {
            const qreal alpha = strength * 0.18 * qExp(-qreal(i * i) / (2 * sigma * sigma));
            smooth.setBrush(QColor::fromRgbF(0, 0, 0, alpha));
            smooth.drawRoundedRect(bounds.translated(0, offset).adjusted(-i, -i, i, i), radius + i, radius + i);
        }
        // Erase with antialiased coverage, not a binary QPainter clip region.
        // This keeps shadow ink outside glass without a jagged inner contour.
        smooth.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        smooth.setBrush(Qt::black);
        smooth.drawRoundedRect(bounds, radius, radius);
    });
}

inline void paintFloatingPanel(QPainter &painter, const QRectF &bounds, const QPalette &palette, bool withShadow = true)
{
    constexpr qreal radius = 16;
    if (withShadow) paintFloatingShadow(painter, bounds, radius, palette);
    QColor surface = palette.color(QPalette::Button);
    surface.setAlpha(255);
    const bool dark = surface.lightness() < 128;
    const QString key = QStringLiteral("panel/%1/%2").arg(surfaceGeometryKey(bounds, radius)).arg(surface.rgba());
    paintSmoothSurface(painter, bounds, key, [&](QPainter &smooth) {
        smooth.setPen(QPen(QColor(255, 255, 255, dark ? 20 : 110), 1));
        smooth.setBrush(surface);
        smooth.drawRoundedRect(bounds.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    });
}
}
