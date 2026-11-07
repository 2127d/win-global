// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QQuickPaintedItem>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
// Render the bundled vectors with the requested palette ink on both the
// software and GPU scene graphs. Icon-theme masks are not reliable for URLs.
class HubGlyph : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QString symbol READ symbol WRITE setSymbol NOTIFY symbolChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    explicit HubGlyph(QQuickItem *parent=nullptr) : QQuickPaintedItem(parent) { setAntialiasing(true); }
    QString symbol() const { return m_symbol; }
    QColor color() const { return m_color; }
    void setSymbol(const QString &value) { if(value==m_symbol)return; m_symbol=value; update(); Q_EMIT symbolChanged(); }
    void setColor(const QColor &value) { if(value==m_color)return; m_color=value; update(); Q_EMIT colorChanged(); }
    void paint(QPainter *painter) override {
        if (m_symbol.contains(QLatin1Char('/')) || m_symbol.contains(QLatin1Char('.'))) return;
        QFile file(QStringLiteral(":/qt/qml/org/kde/systemsettings/")+m_symbol+QStringLiteral(".svg"));
        if(!file.open(QIODevice::ReadOnly))return;
        QByteArray svg=file.readAll(); svg.replace("currentColor",m_color.name().toUtf8());
        QSvgRenderer renderer(svg);
        painter->setRenderHint(QPainter::Antialiasing);painter->setOpacity(m_color.alphaF());
        const qreal side=qMin(width(),height());
        renderer.render(painter,QRectF((width()-side)/2,(height()-side)/2,side,side));
    }
Q_SIGNALS:
    void symbolChanged();
    void colorChanged();
private:
    QString m_symbol=QStringLiteral("sliders");
    QColor m_color=QColor(35,43,50);
};
