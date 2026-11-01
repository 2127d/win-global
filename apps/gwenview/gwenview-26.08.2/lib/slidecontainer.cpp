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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
// Self
#include "slidecontainer.h"
#include <lib/sequoiadesign.h>

// KF

// Qt
#include <QEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStyle>
#include <QLayout>

namespace Gwenview
{
static const int SLIDE_DURATION = Sequoia::LongDuration;

SlideContainer::SlideContainer(QWidget *parent)
    : QFrame(parent)
    , mContent(nullptr)
{
    mSlidingOut = false;
    setFixedHeight(0);
}

QWidget *SlideContainer::content() const
{
    return mContent;
}

void SlideContainer::setContent(QWidget *content)
{
    if (mContent) {
        mContent->setParent(nullptr);
        mContent->removeEventFilter(this);
    }
    mContent = content;
    if (mContent) {
        mContent->setParent(this);
        mContent->setAutoFillBackground(false);
        mContent->installEventFilter(this);
        mContent->hide();
    }
}

void SlideContainer::animTo(int newHeight)
{
    delete mAnim.data();
    auto anim = new QPropertyAnimation(this, "slideHeight", this);
    anim->setObjectName(QStringLiteral("editingToolSlideAnimation"));
    anim->setDuration(style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) == 0 ? 0 : SLIDE_DURATION);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->setStartValue(slideHeight());
    anim->setEndValue(newHeight);
    connect(anim, &QPropertyAnimation::finished, this, &SlideContainer::slotAnimFinished);
    mAnim = anim;
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SlideContainer::slideIn()
{
    mSlidingOut = false;
    mContent->show();
    mContent->resize(qMax(1, width() - 32), mContent->sizeHint().height());
    delete mAnim.data();
    if (height() == mContent->height() + 32) {
        return;
    }
    animTo(mContent->height() + 32);
}

void SlideContainer::slideOut()
{
    if (height() == 0) {
        return;
    }
    mSlidingOut = true;
    animTo(0);
}

QSize SlideContainer::sizeHint() const
{
    if (mContent) {
        return mContent->sizeHint() + QSize(32, 32);
    } else {
        return {};
    }
}

QSize SlideContainer::minimumSizeHint() const
{
    if (mContent) {
        return mContent->minimumSizeHint() + QSize(32, 32);
    } else {
        return {};
    }
}

void SlideContainer::resizeEvent(QResizeEvent *event)
{
    if (mContent) {
        if (event->oldSize().width() != width()) {
            adjustContentGeometry();
        }
    }
}

void SlideContainer::adjustContentGeometry()
{
    if (mContent) {
        const int contentHeight = mContent->hasHeightForWidth() ? mContent->heightForWidth(qMax(1, width() - 32)) : mContent->height();
        mContent->setGeometry(16, height() - contentHeight - 12, qMax(1, width() - 32), contentHeight);
    }
}

bool SlideContainer::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Resize:
        if (!mSlidingOut && height() != 0 && height() != mContent->height() + 32) {
            animTo(mContent->height() + 32);
        }
        break;
    case QEvent::LayoutRequest:
        mContent->resize(qMax(1, width() - 32), mContent->sizeHint().height());
        break;
    default:
        break;
    }
    return false;
}

int SlideContainer::slideHeight() const
{
    return isVisible() ? height() : 0;
}

void SlideContainer::setSlideHeight(int value)
{
    setFixedHeight(value);
    adjustContentGeometry();
}

void SlideContainer::slotAnimFinished()
{
    if (height() == 0) {
        mSlidingOut = false;
        Q_EMIT slidedOut();
    } else {
        Q_EMIT slidedIn();
    }
}

} // namespace

#include "moc_slidecontainer.cpp"
