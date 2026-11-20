// SPDX-License-Identifier: GPL-2.0-or-later
#include "sequoiaelevenmenutransition.h"
#include "sequoiaeleven.h"
#include "sequoiaelevenhelper.h"
#include <QActionEvent>
#include <QApplication>
#include <QBackingStore>
#include <QMenu>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <private/qhighdpiscaling_p.h>
#endif

namespace SequoiaEleven {
static void positionSubmenu(QMenu *menu, bool initialShow = false)
{
    if (auto handle = menu->windowHandle(); initialShow && handle && handle->property("sequoiaSubmenuAnchor").toBool()) {
        for (const auto key : {"_q_waylandPopupAnchorRect", "_q_waylandPopupAnchor", "_q_waylandPopupGravity", "_q_waylandPopupConstraintAdjustment"})
            handle->setProperty(key, QVariant());
        handle->setProperty("sequoiaSubmenuAnchor", false);
    }
    // Qt's screen-edge fallback aligns to the parent action rectangle
    // without the submenu offset. Restore the visible-surface gap
    // before the popup is mapped, keeping Qt's chosen side and y.
    // KActionMenu can expose a different QAction from menuAction().
    // Resolve the active menu relationship rather than QAction ownership.
    for (auto owner : QApplication::topLevelWidgets()) {
        auto parentMenu = qobject_cast<QMenu *>(owner);
        if (!parentMenu || !parentMenu->isVisible()
            || !parentMenu->activeAction() || parentMenu->activeAction()->menu() != menu) continue;
        const QRect parent = parentMenu->geometry();
        const QRect screen = parentMenu->screen()->availableGeometry();
        const int right = parent.right() + 1 + Metrics::Menu_SubmenuGap;
        const int left = parent.left() - menu->width() - Metrics::Menu_SubmenuGap;
        const bool fitsRight = right + menu->width() <= screen.right() + 1;
        const bool fitsLeft = left >= screen.left();
        bool opensRight = menu->geometry().center().x() >= parent.center().x();
        // Continue an already-flipped cascade in the same direction;
        // otherwise a third level can land back over its grandparent.
        for (auto ancestorOwner : QApplication::topLevelWidgets()) {
            auto ancestor = qobject_cast<QMenu *>(ancestorOwner);
            if (ancestor && ancestor->isVisible() && ancestor->activeAction()
                && ancestor->activeAction()->menu() == parentMenu) {
                opensRight = parent.center().x() >= ancestor->geometry().center().x();
                break;
            }
        }
        int targetX = menu->x();
        if (opensRight ? fitsRight : fitsLeft)
            targetX = opensRight ? right : left;
        else if (opensRight ? fitsLeft : fitsRight)
            targetX = opensRight ? left : right;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        if (Helper::isWayland()) {
            // Wayland positions popups from the action anchor, ignoring
            // QWidget::move(). Expand that anchor to the parent surface
            // plus the gap, and let KWin flip/slide it at output edges.
            if (!menu->windowHandle()) menu->winId();
            auto handle = menu->windowHandle();
            // Qt Wayland attaches nested popups to the current popup
            // even when their QWidget owner is the application window.
            // The anchor must be local to that actual popup parent.
            auto transient = parentMenu->windowHandle();
            if (transient) {
                const QRect anchor(QPoint(-Metrics::Menu_SubmenuGap, menu->y() - parent.y()),
                                   QSize(parent.width() + 2 * Metrics::Menu_SubmenuGap, 1));
                const bool rightSide = targetX + menu->width() / 2 >= parent.center().x();
                const Qt::Edges side = rightSide ? Qt::RightEdge : Qt::LeftEdge;
                handle->setProperty("_q_waylandPopupAnchorRect", QHighDpi::toNativeLocalPosition(anchor, transient));
                handle->setProperty("_q_waylandPopupAnchor", QVariant::fromValue(Qt::Edges(Qt::TopEdge | side)));
                handle->setProperty("_q_waylandPopupGravity", QVariant::fromValue(Qt::Edges(Qt::BottomEdge | side)));
                handle->setProperty("_q_waylandPopupConstraintAdjustment", 1u | 2u | 4u | 8u);
                handle->setProperty("sequoiaSubmenuAnchor", true);
            }
        }
#endif
        // Moving a mapped Wayland popup immediately snapshots these properties
        // into a new positioner, so install its anchor before changing position.
        menu->move(targetX, menu->y());
        break;
    }
}

void MenuTransition::install(QMenu *menu)
{
    if (!menu->findChild<MenuTransition *>(QString(), Qt::FindDirectChildrenOnly)) new MenuTransition(menu);
}

void MenuTransition::remove(QMenu *menu)
{
    if (auto transition = menu->findChild<MenuTransition *>(QString(), Qt::FindDirectChildrenOnly)) {
        transition->m_committing = true;
        transition->restore();
        delete transition;
    }
}

MenuTransition::MenuTransition(QMenu *menu) : QObject(menu), m_menu(menu)
{
    m_commit.setSingleShot(true);
    connect(&m_commit, &QTimer::timeout, this, &MenuTransition::commit);
    menu->installEventFilter(this);
}

void MenuTransition::restore()
{
    m_commit.stop();
    if (!m_frozen) return;
    m_frozen = false;
    // Preserve an explicit constraint change made by the application mid-batch.
    if (m_menu->minimumSize() == m_frozenSize) m_menu->setMinimumSize(m_minimum);
    if (m_menu->maximumSize() == m_frozenSize) m_menu->setMaximumSize(m_maximum);
    m_menu->setUpdatesEnabled(true);
}

bool MenuTransition::eventFilter(QObject *, QEvent *event)
{
    if (m_committing) return false;
    switch (event->type()) {
    case QEvent::Show:
        positionSubmenu(m_menu, true);
        break;
    case QEvent::Hide:
        m_committing = true;
        restore();
        m_committing = false;
        m_painted = false;
        break;
    case QEvent::Paint:
        m_painted = true;
        break;
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
        // The appmenu applet already performs its own synchronous transaction.
        // Respect any caller that has explicitly suspended widget updates.
        if (!m_menu->isVisible() || (!m_frozen && !m_menu->updatesEnabled())) break;
        if (!m_frozen) {
            m_minimum = m_menu->minimumSize();
            m_maximum = m_menu->maximumSize();
            m_frozenSize = m_menu->size();
            m_frozen = true;
            m_menu->setUpdatesEnabled(false);
            m_menu->setFixedSize(m_frozenSize);
        }
        m_commit.start(0);
        break;
    case QEvent::Resize:
        if (m_menu->isVisible() && m_menu->updatesEnabled()) m_commit.start(0);
        break;
    default:
        break;
    }
    return false; // QMenu still owns action/widget lifetimes and native input.
}

void MenuTransition::commit()
{
    m_committing = true;
    const bool relayout = m_frozen;
    restore();
    if (m_menu->isVisible()) {
        if (relayout) {
            // QMenu cached action rectangles while its size was frozen.
            if (!m_menu->actions().isEmpty()) {
                QActionEvent changed(QEvent::ActionChanged, m_menu->actions().first());
                QCoreApplication::sendEvent(m_menu, &changed);
            }
            // adjustSize() additionally caps top-level widgets at 2/3 screen.
            m_menu->resize(m_menu->sizeHint());
        }
        positionSubmenu(m_menu);
        if (auto action = m_menu->activeAction(); action && action->menu() && action->menu()->isVisible())
            positionSubmenu(action->menu());
        if (m_painted && m_menu->updatesEnabled() && m_menu->windowHandle()) {
            // QWidget repaint() is deferred during Wayland popup repositioning.
            // Flush the final-size buffer before the compositor can stretch the
            // previous contents into the new geometry. Never remap the popup.
            if (auto store = m_menu->backingStore()) {
                const QRegion region(m_menu->rect());
                store->resize(m_menu->size());
                store->beginPaint(region);
                QPainter painter(store->paintDevice());
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.fillRect(m_menu->rect(), Qt::transparent);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                m_menu->render(&painter);
                painter.end();
                store->endPaint();
                store->flush(region, m_menu->windowHandle());
            }
        }
    }
    m_committing = false;
}
}
