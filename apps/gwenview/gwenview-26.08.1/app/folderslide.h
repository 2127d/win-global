// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QAbstractItemView>
#include <QApplication>
#include <QGraphicsEffect>
#include <QPainter>
#include <QStyle>
#include <QUrl>
#include <QVariantAnimation>

namespace Gwenview {
// Translate only the viewport's painting. Its geometry, scrollbars, selection
// and hit targets stay native. Retain one viewport-sized frame for the brief
// transition while listing/thumbnail delivery continue. There are no folder-wide
// snapshots or per-frame item paints, and the effect is disabled when idle.
class FolderSlideEffect : public QGraphicsEffect
{
public:
    explicit FolderSlideEffect(QWidget *viewport) : QGraphicsEffect(viewport), mViewport(viewport) { setEnabled(false); }
    void setOffset(qreal offset) { mOffset = offset; update(); }
    void clearFrame() { mFrame = QPixmap(); }
protected:
    QRectF boundingRectFor(const QRectF &rect) const override { return rect; }
    void draw(QPainter *painter) override
    {
        if (mFrame.isNull()) mFrame = sourcePixmap(Qt::LogicalCoordinates, &mOrigin, NoPad);
        painter->save();
        painter->setClipRect(mViewport->rect(), Qt::IntersectClip);
        painter->fillRect(mViewport->rect(), mViewport->palette().brush(QPalette::Base));
        painter->drawPixmap(QPointF(mOrigin) + QPointF(mOffset, 0), mFrame);
        painter->restore();
    }
private:
    QWidget *mViewport;
    QPixmap mFrame;
    QPoint mOrigin;
    qreal mOffset = 0;
};

class FolderSlide : public QObject
{
public:
    FolderSlide(QAbstractItemView *view, QWidget *page) : QObject(page), mView(view), mPage(page)
    {
        mEffect = new FolderSlideEffect(view->viewport());
        mEffect->setObjectName(QStringLiteral("folderSlideEffect"));
        view->viewport()->setGraphicsEffect(mEffect);
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("folderSlideAnimation"));
        mAnimation->setDuration(140); mAnimation->setEasingCurve(QEasingCurve::OutCubic);
        mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            const qreal offset = mDirection * 28 * (1 - value.toReal());
            mEffect->setOffset(offset);
            mView->setProperty("folderSlideOffset", offset);
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        qApp->installEventFilter(this);
    }
    void navigate(const QUrl &url, int historyIndex)
    {
        const bool changed = !mUrl.isEmpty() && url != mUrl;
        const bool back = url.isParentOf(mUrl) || historyIndex > mHistoryIndex;
        mUrl = url; mHistoryIndex = historyIndex;
        finish();
        if (!changed || !url.isValid() || !mPage->isVisible() || !mView->isVisible()
            || mView->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mView) == 0) return;
        mDirection = (back ? -1 : 1) * (mView->isRightToLeft() ? -1 : 1);
        mEffect->setOffset(mDirection * 28);
        mView->setProperty("folderSlideOffset", mDirection * 28);
        mEffect->setEnabled(true);
        mAnimation->start();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (mAnimation->state() != QAbstractAnimation::Running) return false;
        if (object == mPage && (event->type() == QEvent::Resize || event->type() == QEvent::Hide)) finish();
        if (event->type() == QEvent::ApplicationPaletteChange) finish();
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress
            || event->type() == QEvent::Wheel || event->type() == QEvent::TouchBegin) {
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && widget->window() == mPage->window()) finish();
        }
        return false;
    }
private:
    void finish()
    {
        mAnimation->stop();
        mEffect->setEnabled(false);
        mEffect->clearFrame();
        mEffect->setOffset(0);
        mView->setProperty("folderSlideOffset", 0.0);
    }
    QAbstractItemView *mView;
    QWidget *mPage;
    FolderSlideEffect *mEffect;
    QVariantAnimation *mAnimation;
    QUrl mUrl;
    int mHistoryIndex = 0, mDirection = 1;
};
}
