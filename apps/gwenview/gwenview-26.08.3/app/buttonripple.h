// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPainter>
#include <QEvent>
#include <QPointer>
#include <QStyle>
#include <QToolButton>
#include <QVariantAnimation>

namespace Gwenview {
class ButtonRippleWave : public QWidget
{
public:
    explicit ButtonRippleWave(QWidget *window) : QWidget(window)
    {
        setObjectName(QStringLiteral("sidebarButtonRippleWave"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::NoFocus); hide();
    }
    qreal progress = 0;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        QColor accent = palette().color(QPalette::Highlight);
        accent.setAlphaF(.85 * (1 - progress * progress));
        painter.setPen(QPen(accent, 1.8)); painter.setBrush(Qt::NoBrush);
        const qreal radius = 5 + progress * (width() / 2.0 - 7);
        painter.drawEllipse(QRectF(rect()).center(), radius, radius);
    }
};

class SidebarButtonRipple : public QObject
{
public:
    explicit SidebarButtonRipple(QToolButton *button) : QObject(button), mButton(button)
    {
        setObjectName(QStringLiteral("sidebarButtonRipple"));
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("sidebarButtonRippleAnimation"));
        mAnimation->setDuration(320); mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        connect(button, &QAbstractButton::pressed, this, [this] {
            finish();
            if (!mButton->isVisible() || mButton->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mButton) == 0) return;
            if (!mWave || mWave->parentWidget() != mButton->window()) {
                delete mWave.data(); mWave = new ButtonRippleWave(mButton->window());
            }
            mWave->setPalette(mButton->palette()); mWave->progress = 0;
            position(); mWave->show(); mWave->raise(); mAnimation->start();
        });
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            if (!mWave || !mButton->isVisible()) { finish(); return; }
            position(); mWave->progress = value.toReal(); mWave->update();
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        button->installEventFilter(this);
    }
    ~SidebarButtonRipple() override { delete mWave.data(); }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Hide || event->type() == QEvent::ParentChange) finish();
        return false;
    }
private:
    void finish() { mAnimation->stop(); if (mWave) mWave->hide(); }
    void position()
    {
        const QPoint center = mButton->mapTo(mButton->window(), mButton->rect().center());
        const int side = qMax(mButton->width(), mButton->height()) + 28;
        mWave->setGeometry(center.x() - side / 2, center.y() - side / 2, side, side);
    }
    QToolButton *mButton;
    QPointer<ButtonRippleWave> mWave;
    QVariantAnimation *mAnimation;
};
}
