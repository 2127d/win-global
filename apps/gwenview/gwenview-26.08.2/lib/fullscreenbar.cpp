// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "fullscreenbar.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QScreen>
#include <QTimeLine>
#include <QTimer>
#include <QToolButton>

// KF
#include <KLocalizedString>
#include <chrono>

using namespace std::chrono_literals;

// Local

namespace Gwenview
{
static const int SLIDE_DURATION = 150;
static const int AUTO_HIDE_CURSOR_TIMEOUT = 1000;

// How long before the bar slide out after switching to fullscreen
static const int INITIAL_HIDE_TIMEOUT = 2000;

// Do not slide bar out if mouse is less than this amount of pixels below bar, to
// prevent accidental slide outs
static const int EXTRA_BAR_HEIGHT = 20;

struct FullScreenBarPrivate {
    FullScreenBar *q = nullptr;
    QTimeLine *mTimeLine = nullptr;
    QTimer *mAutoHideCursorTimer = nullptr;
    bool mAutoHidingEnabled;
    bool mEdgeTriggerEnabled;
    bool mActivated = false;
    bool mOwnsCursor = false;
    bool mKeyboardInteraction = false;
    QPoint mPointer;
    qreal mReveal = 1;
    QTimer *mInitialHideTimer = nullptr;

    void startTimeLine()
    {
        if (mTimeLine->state() != QTimeLine::Running) {
            mTimeLine->start();
        }
    }

    void hideCursor()
    {
        if (!mOwnsCursor) {
            QApplication::setOverrideCursor(Qt::BlankCursor);
            mOwnsCursor = true;
        }
    }

    void restoreCursor()
    {
        if (mOwnsCursor) {
            QApplication::restoreOverrideCursor();
            mOwnsCursor = false;
        }
    }

    /**
     * Returns the rectangle in which the mouse must enter to trigger bar
     * sliding. The rectangle is in global coords.
     */
    QRect slideInTriggerRect() const
    {
        // Use the image pane's global bounds, including sidebar/save-bar offsets.
        QRect rect(q->parentWidget()->mapToGlobal(QPoint()), q->parentWidget()->size());
        rect.setHeight(mReveal > 0 ? qMax(12, q->y() + q->height() + EXTRA_BAR_HEIGHT) : 12);
        return rect;
    }

    bool shouldHide() const
    {
        Q_ASSERT(q->parentWidget());

        if (!mActivated || !mAutoHidingEnabled) {
            return false;
        }
        if (slideInTriggerRect().contains(mPointer)) {
            return false;
        }
        QWidget *focus = QApplication::focusWidget();
        if (mKeyboardInteraction && focus && (focus == q || q->isAncestorOf(focus))) return false;
        if (QApplication::activePopupWidget() || QApplication::activeModalWidget()) {
            return false;
        }
        // Do not hide if a button is down, which can happen when we are
        // using a scroll bar.
        if (QApplication::mouseButtons() != Qt::NoButton) {
            return false;
        }
        return true;
    }
};

FullScreenBar::FullScreenBar(QWidget *parent)
    : QFrame(parent)
    , d(new FullScreenBarPrivate)
{
    d->q = this;
    d->mAutoHidingEnabled = true;
    d->mEdgeTriggerEnabled = true;
    setObjectName(QStringLiteral("fullScreenBar"));

    d->mTimeLine = new QTimeLine(SLIDE_DURATION, this);
    connect(d->mTimeLine, &QTimeLine::valueChanged, this, &FullScreenBar::moveBar);

    d->mAutoHideCursorTimer = new QTimer(this);
    d->mAutoHideCursorTimer->setInterval(AUTO_HIDE_CURSOR_TIMEOUT);
    d->mAutoHideCursorTimer->setSingleShot(true);
    connect(d->mAutoHideCursorTimer, &QTimer::timeout, this, &FullScreenBar::slotAutoHideCursorTimeout);

    d->mInitialHideTimer = new QTimer(this);
    d->mInitialHideTimer->setInterval(INITIAL_HIDE_TIMEOUT);
    d->mInitialHideTimer->setSingleShot(true);
    connect(d->mInitialHideTimer, &QTimer::timeout, this, &FullScreenBar::slideOut);

    hide();
}

FullScreenBar::~FullScreenBar()
{
    d->restoreCursor();
    delete d;
}

QSize FullScreenBar::sizeHint() const
{
    QSize sh = QFrame::sizeHint();
    if (!layout()) {
        return sh;
    }

    if (layout()->expandingDirections() & Qt::Horizontal) {
        sh.setWidth(parentWidget()->width());
    }
    return sh;
}

void FullScreenBar::moveBar(qreal value)
{
    d->mReveal = value;
    move(0, -height() + int(value * height()));

    // For some reason, if Gwenview is started with command line options to
    // start a slideshow, the bar might end up below the view. Calling raise()
    // here fixes it.
    raise();
}

void FullScreenBar::setActivated(bool activated)
{
    if (d->mActivated == activated) return;
    d->mActivated = activated;
    d->mPointer = QCursor::pos();
    d->mTimeLine->stop();
    d->mInitialHideTimer->stop();
    if (activated) {
        // Delay installation of event filter because switching to fullscreen
        // cause a few window adjustments, which seems to generate unwanted
        // mouse events, which cause the bar to slide in.
        QTimer::singleShot(500ms, this, &FullScreenBar::delayedInstallEventFilter);

        adjustSize();

        // Make sure the widget is visible on start
        moveBar(1);
        raise();
        show();
    } else {
        qApp->removeEventFilter(this);
        hide();
        d->mAutoHideCursorTimer->stop();
        d->restoreCursor();
    }
}

void FullScreenBar::delayedInstallEventFilter()
{
    if (!d->mActivated) return;
    qApp->installEventFilter(this);
    if (d->shouldHide()) {
        d->mInitialHideTimer->start();
        d->mAutoHideCursorTimer->start();
    }
}

void FullScreenBar::slotAutoHideCursorTimeout()
{
    if (d->shouldHide()) {
        d->hideCursor();
    } else {
        d->mAutoHideCursorTimer->start();
    }
}

void FullScreenBar::slideOut()
{
    if (!d->shouldHide()) return;
    d->mInitialHideTimer->stop();
    d->mTimeLine->setDirection(QTimeLine::Backward);
    d->startTimeLine();
}

void FullScreenBar::slideIn()
{
    if (!d->mActivated) return;
    d->mInitialHideTimer->stop();
    d->restoreCursor();
    d->mTimeLine->setDirection(QTimeLine::Forward);
    d->startTimeLine();
}

bool FullScreenBar::eventFilter(QObject *object, QEvent *event)
{
    if (!d->mActivated) return false;
    auto widget = qobject_cast<QWidget *>(object);
    if (!widget || widget->window() != window()) return false;
    if (event->type() == QEvent::FocusIn) {
        auto focus = static_cast<QFocusEvent *>(event);
        if (isAncestorOf(widget) && (focus->reason() == Qt::TabFocusReason || focus->reason() == Qt::BacktabFocusReason || focus->reason() == Qt::ShortcutFocusReason)) {
            d->mKeyboardInteraction = true;
            slideIn();
        }
    }
    if (event->type() == QEvent::KeyPress) d->restoreCursor();
    if (event->type() == QEvent::MouseButtonPress) d->mKeyboardInteraction = false;
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
        auto mouse = static_cast<QMouseEvent *>(event);
        d->mPointer = mouse->globalPosition().toPoint();
        d->restoreCursor();
        d->mAutoHideCursorTimer->start();
        if (y() == 0) {
            if (d->shouldHide()) slideOut();
        } else if (d->mEdgeTriggerEnabled && mouse->buttons() == Qt::NoButton && d->slideInTriggerRect().contains(d->mPointer)) {
            slideIn();
        }
        return false;
    }

    return false;
}

void FullScreenBar::setAutoHidingEnabled(bool value)
{
    d->mAutoHidingEnabled = value;
    if (!value) slideIn();
    else if (d->mActivated && d->shouldHide()) d->mInitialHideTimer->start();
}

void FullScreenBar::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    moveBar(d->mReveal);
}

void FullScreenBar::setEdgeTriggerEnabled(bool value)
{
    d->mEdgeTriggerEnabled = value;
}

} // namespace

#include "moc_fullscreenbar.cpp"
