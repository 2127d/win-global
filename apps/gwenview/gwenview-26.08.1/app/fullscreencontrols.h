// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <QAction>
#include <QToolButton>
#include <KLocalizedString>

namespace Gwenview {
// Paint local button glyphs in the fullscreen palette without changing the
// shared QActions used by the normal toolbar and menus.
class FullScreenButton : public QToolButton
{
public:
    explicit FullScreenButton(QAction *action, QWidget *parent) : QToolButton(parent)
    {
        setDefaultAction(action);
        setObjectName(QStringLiteral("fullscreen_") + action->objectName());
        setAutoRaise(true);
        setFocusPolicy(Qt::TabFocus);
        setFixedSize(38, 38);
        setIconSize(QSize(20, 20));
        auto refresh = [this, action] {
            QString text = KLocalizedString::removeAcceleratorMarker(action->text());
            setAccessibleName(text);
            if (!action->shortcut().isEmpty()) text += QStringLiteral(" (%1)").arg(action->shortcut().toString(QKeySequence::NativeText));
            setToolTip(text);
        };
        connect(action, &QAction::changed, this, refresh);
        refresh();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        QColor accent = palette().color(QPalette::Highlight);
        if (isChecked() || underMouse() || isDown() || hasFocus()) {
            QColor fill = accent; fill.setAlpha(isDown() ? 85 : isChecked() ? 60 : 30);
            painter.setPen(Qt::NoPen); painter.setBrush(fill);
            painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 11, 11);
        }
        if (hasFocus()) {
            painter.setPen(QPen(accent, 1.5)); painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(rect()).adjusted(2, 2, -2, -2), 10, 10);
        }
        QPixmap glyph = icon().pixmap(iconSize(), devicePixelRatioF());
        if (!glyph.isNull()) {
            QPainter tint(&glyph); tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(glyph.rect(), palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
            tint.end(); painter.drawPixmap(QRect(9, 9, 20, 20), glyph);
        }
    }
};

class FullScreenSurface : public QWidget
{
public:
    using QWidget::QWidget;
    void setFloating(bool floating) { mFloating = floating; update(); }
    void setHorizontalInset(int inset) { mHorizontalInset = inset; update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!mFloating) return;
        QPainter painter(this);
        const QRectF bounds = QRectF(rect()).adjusted(mHorizontalInset, 10, -mHorizontalInset, -18);
        paintFloatingShadow(painter, bounds, 16, palette(), true);
        paintFloatingPanel(painter, bounds, palette(), false);
    }
private:
    bool mFloating = true;
    int mHorizontalInset = 14;
};

}
