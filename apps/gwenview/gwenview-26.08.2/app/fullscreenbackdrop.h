// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "backdropblur.h"
#include <QGraphicsView>
#include <QPainter>
#include <QPointer>
#include <QTimer>

namespace Gwenview {
// One scene-only capture shared by all fullscreen islands. Never grab the
// controls themselves, and never blur labels, icons or thumbnail contents.
class FullScreenBackdrop : public QObject
{
public:
    FullScreenBackdrop(QWidget *surface, QGraphicsView *view, QWidget *bar)
        : QObject(surface), mSurface(surface), mView(view), mBar(bar), mTimer(new QTimer(this))
    {
        setObjectName(QStringLiteral("fullscreenBackdrop"));
        mTimer->setSingleShot(true); mTimer->setInterval(32);
        connect(mTimer, &QTimer::timeout, this, [this] { capture(); });
        for (QWidget *widget : {surface, bar, view->viewport()}) widget->installEventFilter(this);
        mTimer->start();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!mView) return false;
        if (event->type() == QEvent::Hide && (object == mSurface || object == mBar)) mTimer->stop();
        else if (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::Move
                 || (object == mView->viewport() && event->type() == QEvent::Paint)) {
            if (mSurface->isVisible() && mView->isVisible() && !mTimer->isActive()) mTimer->start();
        }
        return false;
    }
private:
    void capture()
    {
        if (!mView || !mSurface->isVisible() || !mView->isVisible()) return;
        auto viewport = mView->viewport();
        const QPoint origin = viewport->mapFromGlobal(mSurface->mapToGlobal(QPoint()));
        const QRect region = QRect(origin, mSurface->size()).intersected(viewport->rect());
        if (region.isEmpty()) return;
        QImage raw(QSize(qMax(1, region.width() / 4), qMax(1, region.height() / 4)), QImage::Format_RGB32);
        raw.fill(mView->palette().color(QPalette::Base));
        { QPainter painter(&raw); mView->render(&painter, QRectF(raw.rect()), region, Qt::IgnoreAspectRatio); }
        const QRect local = region.translated(-origin);
        if (raw == mRaw && local == mRect) return;
        mRaw = raw; mRect = local;
        mSurface->setProperty("floatingBackdropImage", blurBackdrop(raw));
        mSurface->setProperty("floatingBackdropRect", local);
        mSurface->update();
        for (auto widget : mSurface->findChildren<QWidget *>())
            if (widget->property("floatingControlPill").toBool() || widget->property("floatingControlSurface").toBool()) widget->update();
    }
    QWidget *mSurface;
    QPointer<QGraphicsView> mView;
    QWidget *mBar;
    QTimer *mTimer;
    QImage mRaw;
    QRect mRect;
};
}
