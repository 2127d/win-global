// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <QAction>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <KLocalizedString>

namespace Gwenview {
inline void paintControlPill(QPainter &painter, const QRectF &bounds, const QPalette &palette)
{
    const bool dark = palette.color(QPalette::Window).lightness() < 128;
    paintFloatingShadow(painter, bounds, 22, palette, true, dark ? 0.8 : 0.6);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient fill(bounds.topLeft(), bounds.bottomLeft());
    fill.setColorAt(0, palette.color(QPalette::Window).lighter(dark ? 113 : 102));
    fill.setColorAt(1, palette.color(QPalette::Window));
    painter.setBrush(fill);
    painter.setPen(QPen(QColor(0, 0, 0, dark ? 75 : 26), 1));
    painter.drawRoundedRect(bounds, 22, 22);
    QLinearGradient rim(bounds.topLeft(), bounds.bottomLeft());
    rim.setColorAt(0, QColor(255, 255, 255, dark ? 45 : 100));
    rim.setColorAt(1, QColor(255, 255, 255, dark ? 16 : 45));
    painter.setBrush(Qt::NoBrush); painter.setPen(QPen(QBrush(rim), 1));
    painter.drawRoundedRect(bounds.adjusted(1, 1, -1, -1), 21, 21);
}

// A 44px surface with space around it for an input-transparent shadow.
class FullScreenPill : public QWidget
{
public:
    explicit FullScreenPill(QWidget *parent = nullptr) : QWidget(parent)
    {
        setProperty("floatingControlPill", true);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        auto row = new QHBoxLayout(this);
        row->setContentsMargins(12, 10, 12, 16); row->setSpacing(3);
        row->setSizeConstraint(QLayout::SetMinimumSize);
        setMinimumHeight(60);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        paintControlPill(painter, QRectF(rect()).adjusted(6, 6, -6, -10), palette());
    }
};

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
        setFixedSize(34, 34);
        setIconSize(QSize(22, 22));
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
            QColor fill = isChecked() ? accent : palette().color(QPalette::WindowText);
            if (!isChecked()) fill.setAlpha(isDown() ? 40 : 22);
            painter.setPen(Qt::NoPen); painter.setBrush(fill);
            painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 16, 16);
        }
        if (hasFocus()) {
            painter.setPen(QPen(accent, 1.5)); painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(rect()).adjusted(2, 2, -2, -2), 15, 15);
        }
        QPixmap glyph = icon().pixmap(iconSize(), devicePixelRatioF());
        if (!glyph.isNull()) {
            QPainter tint(&glyph); tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(glyph.rect(), palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, isChecked() ? QPalette::HighlightedText : QPalette::WindowText));
            tint.end(); painter.drawPixmap(QRect(6, 6, 22, 22), glyph);
        }
    }
};

class FullScreenSurface : public QWidget
{
public:
    using QWidget::QWidget;
    void setFloating(bool floating) { mFloating = floating; update(); }
    void setFloatingParts(const QList<QWidget *> &parts) { mParts = parts; update(); }
    void setHorizontalInset(int inset) { mHorizontalInset = inset; update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!mFloating) return;
        QPainter painter(this);
        if (!mParts.isEmpty()) {
            for (auto part : mParts) {
                if (part->isVisible())
                    paintControlPill(painter, QRectF(part->mapTo(this, QPoint()), part->size()).adjusted(-6, -4, 6, 4), palette());
            }
            return;
        }
        const QRectF bounds = QRectF(rect()).adjusted(mHorizontalInset, 10, -mHorizontalInset, -18);
        paintFloatingShadow(painter, bounds, 16, palette(), true);
        paintFloatingPanel(painter, bounds, palette(), false);
    }
private:
    QList<QWidget *> mParts;
    bool mFloating = true;
    int mHorizontalInset = 14;
};

}
