// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>
#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
class HubTitleBar : public QWidget {
public:
    explicit HubTitleBar(QWidget *parent = nullptr) : QWidget(parent) { setFixedHeight(36); setAccessibleName(i18n("Window title bar")); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path; path.moveTo(0,0); path.lineTo(width()-20,0); path.quadTo(width(),0,width(),20); path.lineTo(width(),height()); path.lineTo(0,height()); path.closeSubpath();
        QColor background = palette().window().color(); KConfig globals(QStringLiteral("kdeglobals"), KConfig::SimpleConfig);
        const auto parts = globals.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal", QStringLiteral("242,243,245")).split(QLatin1Char(','));
        background.setAlphaF(parts.size() > 3 && parts.at(3).toInt() < 255 ? .60 : 1.0); painter.save(); if (isRightToLeft()) { painter.translate(width(), 0); painter.scale(-1, 1); } painter.fillPath(path, background); painter.restore();
        painter.setPen(palette().windowText().color()); painter.drawText(rect().adjusted(20,0,-20,0), Qt::AlignCenter, i18n("Settings — Appearance Hub Preview"));
    }
    void mousePressEvent(QMouseEvent *event) override { if (event->button() == Qt::LeftButton && window()->windowHandle()) window()->windowHandle()->startSystemMove(); }
    void mouseDoubleClickEvent(QMouseEvent *event) override { if (event->button() == Qt::LeftButton) { if (window()->isMaximized()) window()->showNormal(); else window()->showMaximized(); } }
};
