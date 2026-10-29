// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "popupsurface.h"
#include <KWindowEffects>
#include <QApplication>
#include <QCursor>
#include <QGraphicsEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>
#include <QWindow>

namespace Gwenview {
class PopupEntranceEffect : public QGraphicsEffect
{
public:
    explicit PopupEntranceEffect(QWidget *parent) : QGraphicsEffect(parent) { setEnabled(false); }
    void reset() { mFrame = QPixmap(); }
    void setProgress(qreal progress) { mProgress = progress; update(); }
    qreal slideOffset() const { return isEnabled() ? 6 * (1 - mProgress) : 0; }
protected:
    QRectF boundingRectFor(const QRectF &rect) const override { return rect; }
    void draw(QPainter *painter) override
    {
        if (mFrame.isNull()) mFrame = sourcePixmap(Qt::LogicalCoordinates, &mOrigin, NoPad);
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        // Keep the theme's material opacity throughout the quick slide; only
        // geometry animates, and its native blur region follows the same offset.
        painter->drawPixmap(QPointF(mOrigin) + QPointF(0, slideOffset()), mFrame);
        painter->restore();
    }
private:
    QPixmap mFrame;
    QPoint mOrigin;
    qreal mProgress = 0;
};

// Popup geometry and hit targets are final from the outset. Only one cached
// frame moves, and any interaction reveals the live native menu immediately.
class PopupMotion : public QObject
{
public:
    explicit PopupMotion(QWidget *popup) : QObject(popup), mPopup(popup)
    {
        setObjectName(QStringLiteral("popupMotion"));
        mEffect = new PopupEntranceEffect(popup);
        mEffect->setObjectName(QStringLiteral("popupEntranceEffect"));
        popup->setGraphicsEffect(mEffect);
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("popupEntranceAnimation"));
        mAnimation->setDuration(120); mAnimation->setEasingCurve(QEasingCurve::OutCubic);
        mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mEffect->setProgress(value.toReal()); refreshBlurRegion();
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        mStart = new QTimer(this); mStart->setSingleShot(true);
        connect(mStart, &QTimer::timeout, this, [this] {
            if (!mPopup->isVisible()) { finish(); return; }
            mAnimation->start();
        });
        mBlur = new QTimer(this); mBlur->setSingleShot(true);
        connect(mBlur, &QTimer::timeout, this, [this] {
            // The style also queues blur updates on Show/native surface
            // creation. Run after those callbacks, independent of filter order.
            QTimer::singleShot(0, this, [this] { refreshBlurRegion(); });
        });
        qApp->installEventFilter(this);
    }
    void open(bool reducedMotion = false)
    {
        finish(); refreshBlurRegion(); mBlur->start();
        if (reducedMotion || mPopup->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mPopup) == 0) return;
        mPointer = QCursor::pos();
        mEffect->setProgress(0); mEffect->setEnabled(true); refreshBlurRegion(); mStart->start();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == mPopup || object == mPopup->windowHandle()) {
            switch (event->type()) {
            case QEvent::Show:
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::WinIdChange:
            case QEvent::Expose:
            case QEvent::StyleChange:
            case QEvent::PaletteChange:
            case QEvent::PlatformSurface:
                if (event->type() != QEvent::PlatformSurface
                    || static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated)
                    mBlur->start();
                break;
            default: break;
            }
        }
        if (object == mPopup) {
            if (event->type() == QEvent::Hide) finish();
            // The style lazily creates switch widgets for checkable actions
            // after Show, which can also resize the menu at fractional scaling.
            // Refresh its snapshot without cancelling or restarting the entrance.
            if (mEffect->isEnabled() && (event->type() == QEvent::Resize || event->type() == QEvent::ActionChanged)) {
                mEffect->reset(); mEffect->update();
            }
            if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange
                || event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved) finish();
        }
        if (!mEffect->isEnabled()) return false;
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget || widget->window() != mPopup) return false;
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease
            || event->type() == QEvent::KeyPress || event->type() == QEvent::Wheel || event->type() == QEvent::TouchBegin) finish();
        if (event->type() == QEvent::MouseMove
            && (static_cast<QMouseEvent *>(event)->globalPosition().toPoint() - mPointer).manhattanLength() > 2) finish();
        return false;
    }
private:
    void refreshBlurRegion()
    {
        if (!mPopup->isVisible() || !mPopup->windowHandle() || !mPopup->windowHandle()->handle()) return;
        // Let the compositor's Blur effect/settings decide how (or whether)
        // to blur. Restrict its region to the actual rounded material, excluding
        // transparent shadow margins, including while the opening frame slides.
        // Keep tracking enabled: KDE 6.31's false path untracks the surface then
        // retains a new effect for it, crashing when a popup is opened again.
        KWindowEffects::enableBlurBehind(mPopup->windowHandle(), true, popupBlurRegion(mPopup, mEffect->slideOffset()));
    }
    void finish()
    {
        mStart->stop(); mAnimation->stop(); mEffect->setEnabled(false); mEffect->reset(); refreshBlurRegion();
    }
    QWidget *mPopup;
    PopupEntranceEffect *mEffect;
    QVariantAnimation *mAnimation;
    QTimer *mStart, *mBlur;
    QPoint mPointer;
};
}
