// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "../../../../desktop/glass-material/glassrim.h"
#include <QPainter>

// The Settings islands have a wider bevel, using the existing shared intensity.
// This does not change the rim geometry of other applications.
class HubIslandRim : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(qreal radius MEMBER m_radius NOTIFY geometryChanged)
    Q_PROPERTY(qreal thickness MEMBER m_thickness NOTIFY geometryChanged)
public:
    explicit HubIslandRim(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent) {
        setAntialiasing(true);
        connect(this, &HubIslandRim::geometryChanged, this, [this] { update(); });
        connect(SequoiaGlass::Settings::instance(), &SequoiaGlass::Settings::intensityChanged, this, [this] { update(); });
    }
    void paint(QPainter *painter) override {
        const qreal intensity = SequoiaGlass::Settings::instance()->intensity();
        if (intensity <= 0) return;
        const qreal thickness = qBound(1., m_thickness, 5.);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(Qt::NoBrush);
        QLinearGradient edge(0, 0, width() * .7, height());
        edge.setColorAt(0, QColor(255, 255, 255, qMin(255, qRound(70 * intensity))));
        edge.setColorAt(.45, QColor(255, 255, 255, qMin(255, qRound(24 * intensity))));
        edge.setColorAt(1, QColor(0, 0, 0, qMin(255, qRound(32 * intensity))));
        painter->setPen(QPen(edge, thickness));
        const qreal inset = thickness / 2;
        painter->drawRoundedRect(boundingRect().adjusted(inset, inset, -inset, -inset), qMax(0., m_radius-inset), qMax(0., m_radius-inset));
        SequoiaGlass::paintRim(painter, boundingRect(), m_radius, intensity);
    }
Q_SIGNALS:
    void geometryChanged();
private:
    qreal m_radius = 18;
    qreal m_thickness = 3;
};
