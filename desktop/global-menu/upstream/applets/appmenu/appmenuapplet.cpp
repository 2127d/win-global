/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenuapplet.h"
#include "appmenumodel.h"

#include <QAction>
#include <QActionEvent>
#include <QBackingStore>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

int AppMenuApplet::s_refs = 0;
namespace
{
QString viewService()
{
    return QStringLiteral("org.kde.kappmenuview");
}
}

AppMenuApplet::AppMenuApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    ++s_refs;
    // if we're the first, register the service
    if (s_refs == 1) {
        QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                   QDBusConnectionInterface::QueueService,
                                                                   QDBusConnectionInterface::DontAllowReplacement);
    }
    /*it registers or unregisters the service when the destroyed value of the applet change,
      and not in the dtor, because:
      when we "delete" an applet, it just hides it for about a minute setting its status
      to destroyed, in order to be able to do a clean undo: if we undo, there will be
      another destroyedchanged and destroyed will be false.
      When this happens, if we are the only appmenu applet existing, the dbus interface
      will have to be registered again*/
    connect(this, &Applet::destroyedChanged, this, [](bool destroyed) {
        if (destroyed) {
            // if we were the last, unregister
            if (--s_refs == 0) {
                QDBusConnection::sessionBus().interface()->unregisterService(viewService());
            }
        } else {
            // if we're the first, register the service
            if (++s_refs == 1) {
                QDBusConnection::sessionBus().interface()->registerService(viewService(),
                                                                           QDBusConnectionInterface::QueueService,
                                                                           QDBusConnectionInterface::DontAllowReplacement);
            }
        }
    });
}

AppMenuApplet::~AppMenuApplet() = default;

void AppMenuApplet::init()
{
}

QAbstractItemModel *AppMenuApplet::model() const
{
    return m_model;
}

void AppMenuApplet::setModel(QAbstractItemModel *model)
{
    if (m_model != model) {
        m_model = model;
        Q_EMIT modelChanged();
    }
}

int AppMenuApplet::view() const
{
    return m_viewType;
}

void AppMenuApplet::setView(int type)
{
    if (m_viewType != type) {
        m_viewType = type;
        Q_EMIT viewChanged();
    }
}

int AppMenuApplet::currentIndex() const
{
    return m_currentIndex;
}

void AppMenuApplet::setCurrentIndex(int currentIndex)
{
    if (m_currentIndex != currentIndex) {
        m_currentIndex = currentIndex;
        Q_EMIT currentIndexChanged();
    }
}

QQuickItem *AppMenuApplet::buttonGrid() const
{
    return m_buttonGrid;
}

void AppMenuApplet::setButtonGrid(QQuickItem *buttonGrid)
{
    if (m_buttonGrid != buttonGrid) {
        m_buttonGrid = buttonGrid;
        Q_EMIT buttonGridChanged();
    }
}

QMenu *AppMenuApplet::createMenu(int idx) const
{
    QMenu *menu = nullptr;

    if (view() == CompactView) {
        if (auto *menuAction = m_model->data(QModelIndex(), AppMenuModel::ActionRole).value<QAction *>()) {
            menu = menuAction->menu();
        }
    } else if (view() == FullView) {
        const QModelIndex index = m_model->index(idx, 0);
        if (auto *action = m_model->data(index, AppMenuModel::ActionRole).value<QAction *>()) {
            menu = action->menu();
        }
    }

    return menu;
}

void AppMenuApplet::onMenuAboutToHide()
{
    auto menuAction = m_currentMenu->menuAction();
    menuAction->setMenu(m_sourceMenu);
    setCurrentIndex(-1);
}

Qt::Edges edgeFromLocation(Plasma::Types::Location location)
{
    switch (location) {
    case Plasma::Types::TopEdge:
        return Qt::TopEdge;
    case Plasma::Types::BottomEdge:
        return Qt::BottomEdge;
    case Plasma::Types::LeftEdge:
        return Qt::LeftEdge;
    case Plasma::Types::RightEdge:
        return Qt::RightEdge;
    case Plasma::Types::Floating:
    case Plasma::Types::Desktop:
    case Plasma::Types::FullScreen:
        break;
    }
    return {};
}

void AppMenuApplet::trigger(QQuickItem *ctx, int idx)
{
    if (m_currentIndex == idx) {
        return;
    }

    if (!ctx || !ctx->window() || !ctx->window()->screen()) {
        return;
    }

    QMenu *actionMenu = createMenu(idx);
    if (actionMenu) {
        const bool replacingVisibleMenu = view() == FullView && m_currentMenu && m_currentMenu->isVisible();
        const QSize minimumSize = replacingVisibleMenu ? m_currentMenu->minimumSize() : QSize();
        const QSize maximumSize = replacingVisibleMenu ? m_currentMenu->maximumSize() : QSize();
        if (replacingVisibleMenu) {
            // QMenu resizes after individual action changes. Hold the existing
            // geometry until the complete replacement category is installed.
            m_currentMenu->setFixedSize(m_currentMenu->size());
        }
        // this is a workaround where Qt will fail to realize a mouse has been released
        // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
        // whilst the mouse is depressed
        // https://bugreports.qt.io/browse/QTBUG-59044
        // this causes the next click to go missing

        // by releasing manually we avoid that situation
        auto ungrabMouseHack = [ctx]() {
            if (ctx && ctx->window() && ctx->window()->mouseGrabberItem()) {
                // FIXME event forge thing enters press and hold move mode :/
                ctx->window()->mouseGrabberItem()->ungrabMouse();
            }
        };

        if (view() == FullView) {
            if (!m_currentMenu) {
                m_currentMenu = new QMenu(qobject_cast<QWidget *>(actionMenu->parent()));
                connect(m_currentMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide, Qt::UniqueConnection);
            } else if (m_sourceMenu != actionMenu) {
                // Exchange the category as one paint transaction. On Wayland
                // a queued repaint lets the compositor stretch the old menu's
                // buffer to the new category's geometry before it is repainted.
                m_currentMenu->setUpdatesEnabled(false);
                auto menuAction = m_currentMenu->menuAction();
                for (QAction *action : m_currentMenu->actions()) {
                    m_currentMenu->removeAction(action);
                    m_sourceMenu->addAction(action);
                }
                menuAction->setMenu(m_sourceMenu);
            }
            m_sourceMenu = actionMenu;
            auto menuAction = m_sourceMenu->menuAction();
            for (QAction *action : m_sourceMenu->actions()) {
                m_sourceMenu->removeAction(action);
                m_currentMenu->addAction(action);
            }
            menuAction->setMenu(m_currentMenu);
        } else {
            m_currentMenu = actionMenu;
            m_sourceMenu = actionMenu;
        }

        QTimer::singleShot(0, ctx, ungrabMouseHack);
        // end workaround

        const auto &geo = ctx->window()->screen()->availableVirtualGeometry();

        QPoint pos = ctx->window()->mapToGlobal(ctx->mapToScene(QPointF()).toPoint());

        const Qt::Edges edges = edgeFromLocation(location());
        m_currentMenu->setProperty("_breeze_menu_seamless_edges", QVariant::fromValue(edges));

        if (location() == Plasma::Types::TopEdge) {
            pos.setY(pos.y() + ctx->height());
        }

        if (replacingVisibleMenu) {
            m_currentMenu->setMinimumSize(minimumSize);
            m_currentMenu->setMaximumSize(maximumSize);
            // The cached action rectangles were measured under the temporary
            // size constraint. Recompute them once with the restored bounds so
            // a narrow category can shrink after a wider one.
            if (!m_currentMenu->actions().isEmpty()) {
                QActionEvent layoutChanged(QEvent::ActionChanged, m_currentMenu->actions().constFirst());
                QCoreApplication::sendEvent(m_currentMenu, &layoutChanged);
            }
        }
        // QWidget::adjustSize caps top-level windows at two thirds of the
        // screen. Menus use their own screen-aware size hint and scrolling.
        m_currentMenu->resize(m_currentMenu->sizeHint());
        m_currentMenu->setUpdatesEnabled(true);

        pos = QPoint(qBound(geo.x(), pos.x(), geo.x() + geo.width() - m_currentMenu->width()),
                     qBound(geo.y(), pos.y(), geo.y() + geo.height() - m_currentMenu->height()));

        if (view() == FullView) {
            if (m_currentMenu->isVisible()) {
                m_currentMenu->move(pos);
                // Qt can postpone QWidget repaint while a Wayland popup is
                // awaiting its reposition acknowledgement. Submit the complete
                // replacement buffer now, so the old category is never scaled
                // into the new geometry. Keep the popup and its input grab.
                if (auto store = m_currentMenu->backingStore()) {
                    const QRegion region(m_currentMenu->rect());
                    store->resize(m_currentMenu->size());
                    store->beginPaint(region);
                    QPainter painter(store->paintDevice());
                    painter.setCompositionMode(QPainter::CompositionMode_Source);
                    painter.fillRect(m_currentMenu->rect(), Qt::transparent);
                    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    m_currentMenu->render(&painter);
                    painter.end();
                    store->endPaint();
                    store->flush(region, m_currentMenu->windowHandle());
                }
            } else {
                m_currentMenu->installEventFilter(this);
                m_currentMenu->winId(); // create window handle
                m_currentMenu->windowHandle()->setTransientParent(ctx->window());
                m_currentMenu->popup(pos);
            }
        } else if (view() == CompactView) {
            if (m_currentMenu->isEmpty()) {
                // don't try to popup an empty menu in case the app gives us one
                return;
            }
            m_currentMenu->popup(pos);
            connect(actionMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide, Qt::UniqueConnection);
        }

        setCurrentIndex(idx);

        // FIXME TODO connect only once
    } else { // is it just an action without a menu?
        if (auto *action = m_model->index(idx, 0).data(AppMenuModel::ActionRole).value<QAction *>()) {
            Q_ASSERT(!action->menu());
            action->trigger();
        }
    }
}

// FIXME TODO doesn't work on submenu
bool AppMenuApplet::eventFilter(QObject *watched, QEvent *event)
{
    auto *menu = qobject_cast<QMenu *>(watched);
    if (!menu) {
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);

        // TODO right to left languages
        if (e->key() == Qt::Key_Left) {
            int desiredIndex = m_currentIndex - 1;
            Q_EMIT requestActivateIndex(desiredIndex);
            return true;
        } else if (e->key() == Qt::Key_Right) {
            if (menu->activeAction() && menu->activeAction()->menu()) {
                return false;
            }

            int desiredIndex = m_currentIndex + 1;
            Q_EMIT requestActivateIndex(desiredIndex);
            return true;
        }

    } else if (event->type() == QEvent::MouseMove) {
        auto *e = static_cast<QMouseEvent *>(event);

        if (!m_buttonGrid || !m_buttonGrid->window()) {
            return false;
        }

        // FIXME the panel margin breaks Fitt's law :(
        const QPointF &windowLocalPos = m_buttonGrid->window()->mapFromGlobal(e->globalPosition());
        const QPointF &buttonGridLocalPos = m_buttonGrid->mapFromScene(windowLocalPos);
        auto *item = m_buttonGrid->childAt(buttonGridLocalPos.x(), buttonGridLocalPos.y());
        if (!item) {
            return false;
        }

        bool ok;
        const int buttonIndex = item->property("buttonIndex").toInt(&ok);
        if (!ok) {
            return false;
        }

        Q_EMIT requestActivateIndex(buttonIndex);
    }

    return false;
}

K_PLUGIN_CLASS_WITH_JSON(AppMenuApplet, "metadata.json")

#include "appmenuapplet.moc"
#include "moc_appmenuapplet.cpp"
