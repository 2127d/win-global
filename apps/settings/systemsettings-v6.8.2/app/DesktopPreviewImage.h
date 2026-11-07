// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>

class DesktopPreviewImage : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
public:
    explicit DesktopPreviewImage(QQuickItem *parent=nullptr) : QQuickPaintedItem(parent) { setAntialiasing(true); }
    QUrl source() const { return m_source; }
    qreal radius() const { return m_radius; }
    void setSource(const QUrl &value) {
        if(value==m_source) return;
        m_source=value; m_image=value.isLocalFile() ? QImage(value.toLocalFile()) : QImage();
        update(); Q_EMIT sourceChanged();
    }
    void setRadius(qreal value) { if(value==m_radius)return; m_radius=value; update(); Q_EMIT radiusChanged(); }
    void paint(QPainter *painter) override {
        if(m_image.isNull()) return;
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip; clip.addRoundedRect(boundingRect(),m_radius,m_radius);
        const QSizeF size=m_image.size().scaled(boundingRect().size().toSize(),Qt::KeepAspectRatio);
        const QRectF destination(QPointF((width()-size.width())/2,(height()-size.height())/2),size);
        painter->fillPath(clip,QColor(28,30,34));
        QPainterPath imageBounds; imageBounds.addRect(destination);
        QBrush imageBrush(m_image);
        QTransform transform;
        transform.translate(destination.x(),destination.y());
        transform.scale(destination.width()/m_image.width(),destination.height()/m_image.height());
        imageBrush.setTransform(transform);
        painter->fillPath(clip.intersected(imageBounds),imageBrush);
    }
Q_SIGNALS:
    void sourceChanged();
    void radiusChanged();
private:
    QUrl m_source;
    QImage m_image;
    qreal m_radius=14;
};
