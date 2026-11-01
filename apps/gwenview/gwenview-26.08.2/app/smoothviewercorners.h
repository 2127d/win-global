// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <lib/sequoiadesign.h>
#include <QGraphicsEffect>
#include <QImage>
#include <QPainter>
#include <QPixmapCache>
#include <QtMath>

namespace Gwenview {
// Qt caches the viewport-sized source. Preserve its pixels at native resolution
// and alpha-mask only four small corner tiles, never supersample the photograph.
class SmoothViewerCorners : public QGraphicsEffect
{
public:
    explicit SmoothViewerCorners(QObject *parent) : QGraphicsEffect(parent)
    {
        setObjectName(QStringLiteral("smoothViewerCorners"));
    }
protected:
    QRectF boundingRectFor(const QRectF &rect) const override { return rect; }
    void draw(QPainter *painter) override
    {
        QPoint origin;
        const QPixmap source = sourcePixmap(Qt::LogicalCoordinates, &origin, NoPad);
        if (source.isNull()) return;
        const qreal dpr = source.devicePixelRatioF();
        const int side = qMin(qCeil((Sequoia::SurfaceRadius + 1) * dpr), qMin(source.width(), source.height()) / 2);
        if (side < 1) { drawSource(painter); return; }
        const qreal logicalSide = side / dpr;
        const QSizeF size = source.deviceIndependentSize();
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        // Straight rectangular strips have no curved clipping or resampling.
        painter->drawPixmap(QRectF(origin.x() + logicalSide, origin.y(), size.width() - 2 * logicalSide, size.height()),
                            source, QRectF(side, 0, source.width() - 2 * side, source.height()));
        for (bool right : {false, true}) {
            const int x = right ? source.width() - side : 0;
            painter->drawPixmap(QRectF(origin.x() + x / dpr, origin.y() + logicalSide, logicalSide, size.height() - 2 * logicalSide),
                                source, QRectF(x, side, side, source.height() - 2 * side));
        }
        for (int corner = 0; corner < 4; ++corner) {
            const bool right = corner & 1, bottom = corner & 2;
            const QPoint pos(right ? source.width() - side : 0, bottom ? source.height() - side : 0);
            const QString key = QStringLiteral("gwenview-viewer-corner/%1/%2/%3").arg(dpr).arg(side).arg(corner);
            QPixmap cached;
            if (!QPixmapCache::find(key, &cached)) {
                QImage mask(QSize(side, side) * 2, QImage::Format_ARGB32_Premultiplied);
                mask.setDevicePixelRatio(dpr * 2); mask.fill(Qt::transparent);
                {
                    QPainter alpha(&mask); alpha.setRenderHint(QPainter::Antialiasing);
                    alpha.setPen(Qt::NoPen); alpha.setBrush(Qt::white);
                    alpha.drawRoundedRect(QRectF(0, 0, logicalSide * 3, logicalSide * 3), Sequoia::SurfaceRadius, Sequoia::SurfaceRadius);
                }
                mask = mask.scaled(side, side, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).mirrored(right, bottom);
                mask.setDevicePixelRatio(dpr);
                cached = QPixmap::fromImage(mask); QPixmapCache::insert(key, cached);
            }
            QImage tile = source.copy(QRect(pos, QSize(side, side))).toImage();
            {
                QPainter alpha(&tile); alpha.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                alpha.drawPixmap(QPoint(), cached);
            }
            painter->drawImage(QPointF(origin) + QPointF(pos) / dpr, tile);
        }
        painter->restore();
    }
};
}
