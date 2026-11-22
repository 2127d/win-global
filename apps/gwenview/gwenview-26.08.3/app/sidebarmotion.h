// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <lib/sequoiadesign.h>
#include "buttonripple.h"
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QLayout>
#include <QIconEngine>
#include <QPainter>
#include <QPointer>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVariantAnimation>
#include <memory>

namespace Gwenview {
struct SidebarGlyphState { qreal reveal = 1; bool rtl = false; };

// Keep the sidebar outline still; the chevron gently folds through its centre.
// Copies share the same phase, including icons tinted by the floating toolbar.
class SidebarToggleIcon : public QIconEngine
{
public:
    explicit SidebarToggleIcon(std::shared_ptr<SidebarGlyphState> state) : mState(std::move(state)) {}
    QIconEngine *clone() const override { return new SidebarToggleIcon(mState); }
    bool isNull() override { return false; }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return scaledPixmap(size, mode, state, 1); }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal dpr) override
    {
        QPixmap result(size * dpr); result.setDevicePixelRatio(dpr); result.fill(Qt::transparent);
        QPainter painter(&result); paint(&painter, QRect(QPoint(), size), mode, state); return result;
    }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State) override
    {
        painter->save(); painter->setRenderHint(QPainter::Antialiasing);
        const qreal side = qMin(rect.width(), rect.height());
        painter->translate(rect.center().x() - side / 2.0, rect.center().y() - side / 2.0);
        painter->scale(side / 24, side / 24);
        if (mState->rtl) { painter->translate(24, 0); painter->scale(-1, 1); }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(qApp->palette().color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
                                                 QPalette::ButtonText), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRoundedRect(QRectF(3.5, 4.5, 17, 15), 2, 2);
        painter->drawLine(QPointF(8, 5), QPointF(8, 19));
        const qreal direction = 1 - 2 * mState->reveal;
        const QPolygonF arrow{QPointF(14.5 - 1.8 * direction, 9), QPointF(14.5 + 1.8 * direction, 12),
                              QPointF(14.5 - 1.8 * direction, 15)};
        painter->drawPolyline(arrow);
        painter->restore();
    }
private:
    std::shared_ptr<SidebarGlyphState> mState;
};

// Move the live sidebar contents at their full width while the splitter slides.
// Keeping their layout fixed avoids repeated thumbnail/text reflow and avoids
// the expensive first QWidget::grab() of a freshly populated sidebar.
class SidebarMotion : public QObject
{
public:
    SidebarMotion(QSplitter *splitter, QWidget *sidebar, QAction *action, const QList<QToolButton *> &buttons)
        : QObject(splitter), mSplitter(splitter), mSidebar(sidebar), mAction(action), mState(std::make_shared<SidebarGlyphState>())
    {
        mTarget = !sidebar->isHidden();
        mState->reveal = mTarget ? 1 : 0;
        mState->rtl = sidebar->isRightToLeft();
        const QIcon icon(new SidebarToggleIcon(mState));
        action->setIcon(icon);
        for (auto button : buttons) {
            mButtons.append(button); button->setIcon(icon);
            if (!button->findChild<QObject *>(QStringLiteral("sidebarButtonRipple"))) new SidebarButtonRipple(button);
        }
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("sidebarVisibilityAnimation"));
        mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) { setPhase(value.toReal()); });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        qApp->installEventFilter(this);
    }
    bool isRunning() const { return mAnimating; }
    void setVisible(bool visible, bool animate)
    {
        if (visible == mTarget && mAnimating && animate) return;
        if (!animate || !mSplitter->isVisible()
            || mSplitter->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mSplitter) == 0) {
            // Preserve manual splitter resizing when mode changes keep the
            // sidebar visible, or temporarily hide it without animation.
            if (!mAnimating && mSidebar->isVisible()) mFullWidth = mSidebar->width();
            mTarget = visible; finish();
            mSidebar->setVisible(visible);
            if (visible && mFullWidth > 0) {
                mSplitter->refresh();
                mSplitter->setSizes({mFullWidth, qMax(1, mSplitter->width() - mFullWidth - mSplitter->handleWidth())});
            }
            updateGlyph(visible ? 1 : 0); return;
        }
        if (!mAnimating && visible == !mSidebar->isHidden()) { mTarget = visible; updateGlyph(visible ? 1 : 0); return; }
        if (!mAnimating) {
            mMinimumWidth = mSidebar->minimumWidth();
            mCollapsible = mSplitter->childrenCollapsible() && mSplitter->isCollapsible(0);
            mHandleWidth = mSplitter->handleWidth();
            if (visible) {
                mSidebar->show();
                mSplitter->refresh();
                if (mFullWidth > 0) mSplitter->setSizes({mFullWidth, qMax(1, mSplitter->width() - mFullWidth - mHandleWidth)});
            }
            mFullWidth = mSidebar->width();
            mChildren.clear();
            if (auto layout = mSidebar->layout()) {
                layout->activate();
                mLayoutEnabled = layout->isEnabled();
                for (int i = 0; i < layout->count(); ++i)
                    if (auto child = layout->itemAt(i)->widget()) mChildren.append({child, child->geometry()});
                layout->setEnabled(false);
            }
            mSidebar->setProperty("sidebarSlideWidth", mFullWidth);
            mSidebar->setMinimumWidth(1);
            mSplitter->setCollapsible(0, true);
            mAnimating = true;
            setPhase(visible ? 0 : 1);
        }
        mTarget = visible;
        const qreal start = mState->reveal;
        mAnimation->stop();
        {
            const QSignalBlocker blocker(mAnimation);
            mAnimation->setStartValue(start); mAnimation->setEndValue(visible ? 1.0 : 0.0);
            mAnimation->setDuration(qMax(1, qRound(Sequoia::LongDuration * qAbs((visible ? 1.0 : 0.0) - start))));
            mAnimation->setCurrentTime(0);
        }
        mAnimation->start();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == mSplitter && (event->type() == QEvent::Resize || event->type() == QEvent::Hide
                                   || event->type() == QEvent::StyleChange)) finish();
        if (mAnimating && event->type() == QEvent::MouseButtonPress) {
            if (auto handle = qobject_cast<QSplitterHandle *>(object); handle && handle->splitter() == mSplitter) finish();
        }
        if (object == mSidebar && event->type() == QEvent::LayoutDirectionChange) {
            finish(); mState->rtl = mSidebar->isRightToLeft(); updateGlyph(mTarget ? 1 : 0);
        }
        if (mAnimating && (event->type() == QEvent::KeyPress || event->type() == QEvent::Wheel)) {
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && mSidebar->isAncestorOf(widget)) finish();
        }
        return QObject::eventFilter(object, event);
    }
private:
    void updateGlyph(qreal phase)
    {
        mState->reveal = phase;
        for (auto button : mButtons) if (button) button->update();
        for (auto object : mAction->associatedObjects()) if (auto widget = qobject_cast<QWidget *>(object)) widget->update();
    }
    void setPhase(qreal phase)
    {
        updateGlyph(phase);
        const int width = qRound(mFullWidth * phase);
        mSplitter->setHandleWidth(qRound(mHandleWidth * phase));
        mSplitter->setSizes({width, qMax(1, mSplitter->width() - width - mSplitter->handleWidth())});
        const int shift = mSidebar->isRightToLeft() ? 0 : mSidebar->width() - mFullWidth;
        for (const auto &child : mChildren) if (child.first) child.first->move(child.second.topLeft() + QPoint(shift, 0));
        mSidebar->update();
    }
    void finish()
    {
        if (!mAnimating) return;
        mAnimation->stop();
        setPhase(mTarget ? 1 : 0);
        mSidebar->setVisible(mTarget);
        mSidebar->setProperty("sidebarSlideWidth", 0);
        for (const auto &child : mChildren) if (child.first) child.first->setGeometry(child.second);
        mChildren.clear();
        if (auto layout = mSidebar->layout()) { layout->setEnabled(mLayoutEnabled); layout->activate(); }
        mSidebar->setMinimumWidth(mMinimumWidth);
        mSplitter->setCollapsible(0, mCollapsible);
        mSplitter->setHandleWidth(mHandleWidth);
        mSplitter->refresh();
        if (mTarget) mSplitter->setSizes({mFullWidth, qMax(1, mSplitter->width() - mFullWidth - mHandleWidth)});
        mAnimating = false;
        updateGlyph(mTarget ? 1 : 0);
    }
    QSplitter *mSplitter;
    QWidget *mSidebar;
    QAction *mAction;
    QList<QPointer<QToolButton>> mButtons;
    std::shared_ptr<SidebarGlyphState> mState;
    QList<QPair<QPointer<QWidget>, QRect>> mChildren;
    bool mLayoutEnabled = true;
    QVariantAnimation *mAnimation;
    int mFullWidth = 0, mMinimumWidth = 0, mHandleWidth = 0;
    bool mTarget = true, mAnimating = false, mCollapsible = false;
};
}
