// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "documentview.h"
#include <lib/document/document.h>
#include <QApplication>
#include <QGraphicsView>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>

namespace Gwenview {
// A sibling of the viewport, so capturing a frame never captures the overlay.
// Only screen-sized pixmaps move; document geometry and image decoding stay native.
class ImageSlideFrame : public QWidget
{
public:
    explicit ImageSlideFrame(QWidget *parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("imageSlideFrame"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setFocusPolicy(Qt::NoFocus);
        hide();
    }
    QPixmap from, to;
    qreal progress = 0;
    int direction = 1;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), palette().brush(QPalette::Base));
        const int shift = qRound(width() * progress);
        painter.drawPixmap(QPoint(-direction * shift, 0), from);
        if (!to.isNull()) painter.drawPixmap(QPoint(direction * (width() - shift), 0), to);
    }
};

class ImageSlide : public QObject
{
public:
    explicit ImageSlide(QGraphicsView *view) : QObject(view), mView(view)
    {
        mFrame = new ImageSlideFrame(view);
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("imageSlideAnimation"));
        mAnimation->setDuration(140);
        mAnimation->setEasingCurve(QEasingCurve::OutCubic);
        mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mFrame->progress = value.toReal(); mFrame->update();
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        mLoadingTimeout = new QTimer(this);
        mLoadingTimeout->setSingleShot(true); mLoadingTimeout->setInterval(600);
        // Slow/remote files get Gwenview's loading indicator, not a frozen slide.
        connect(mLoadingTimeout, &QTimer::timeout, this, [this] { finish(); });
        qApp->installEventFilter(this);
    }
    bool active() const { return mArmed || !mTarget.isNull(); }
    void prepare(int direction)
    {
        finish();
        if (!mView->isVisible() || mView->viewport()->inherits("QOpenGLWidget")
            || mView->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mView) == 0) return;
        mFrame->setGeometry(mView->viewport()->geometry());
        mFrame->setPalette(mView->palette());
        mFrame->from = mView->viewport()->grab();
        if (mFrame->from.isNull()) return;
        mFrame->direction = direction * (mView->isRightToLeft() ? -1 : 1);
        mFrame->setProperty("direction", mFrame->direction);
        mFrame->setProperty("framePixels", mFrame->from.size());
        mFrame->progress = 0;
        mArmed = true;
        mFrame->show(); // Keep floating controls above this viewport-only layer.
        mLoadingTimeout->start();
    }
    void watch(DocumentView *view)
    {
        if (!mArmed) return;
        mArmed = false; mTarget = view;
        connect(view, &DocumentView::completed, this, [this, view] {
            if (mTarget != view || mAnimation->state() == QAbstractAnimation::Running) return;
            // Let the adapter finish its layout before the one incoming capture.
            QTimer::singleShot(0, this, [this, target = QPointer<DocumentView>(view)] {
                if (!target || mTarget != target || mAnimation->state() == QAbstractAnimation::Running) return;
                auto doc = target->document();
                if (!mView->isVisible() || !doc || doc->loadingState() == Document::LoadingFailed
                    || (doc->kind() != MimeTypeUtils::KIND_RASTER_IMAGE && doc->kind() != MimeTypeUtils::KIND_SVG_IMAGE)) {
                    finish(); return;
                }
                mLoadingTimeout->stop();
                mFrame->to = mView->viewport()->grab();
                if (mFrame->to.isNull()) { finish(); return; }
                mAnimation->start();
            });
        });
        connect(view, &QObject::destroyed, this, [this] { if (mTarget.isNull()) finish(); });
    }
    void finish()
    {
        mAnimation->stop(); mLoadingTimeout->stop();
        mArmed = false; mTarget.clear();
        mFrame->hide(); mFrame->from = QPixmap(); mFrame->to = QPixmap();
        mView->viewport()->update();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!active()) return false;
        // Avoid painting the real (possibly very large) image under cached frames.
        if (object == mView->viewport() && event->type() == QEvent::Paint
            && mAnimation->state() == QAbstractAnimation::Running) return true;
        if ((object == mView || object == mView->viewport())
            && (event->type() == QEvent::Resize || event->type() == QEvent::Hide)) finish();
        if (event->type() == QEvent::ApplicationPaletteChange) finish();
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress
            || event->type() == QEvent::Wheel || event->type() == QEvent::TouchBegin) {
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && widget->window() == mView->window()) finish();
        }
        return false;
    }
private:
    QGraphicsView *mView;
    ImageSlideFrame *mFrame;
    QVariantAnimation *mAnimation;
    QTimer *mLoadingTimeout;
    QPointer<DocumentView> mTarget;
    bool mArmed = false;
};
}
