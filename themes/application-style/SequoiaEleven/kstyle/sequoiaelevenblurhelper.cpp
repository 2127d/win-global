//////////////////////////////////////////////////////////////////////////////
// sequoiaelevenblurhelper.cpp
// handle regions passed to kwin for blurring
// -------------------
//
// Copyright (C) 2018 Alex Nemeth <alex.nemeth329@gmail.com>
//
// Largely rewritten from Oxygen widget style
// Copyright (C) 2007 Thomas Luebking <thomas.luebking@web.de>
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "sequoiaelevenblurhelper.h"
#include "sequoiaelevenpropertynames.h"
#include "sequoiaelevenstyleconfigdata.h"

#include <KWindowEffects>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QMainWindow>
#include <QMenu>
#include <QPair>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QRegularExpression>
#include <QToolBar>
#include <QTimer>
#include <QWindow>
#if SEQUOIAELEVEN_HAVE_QTQUICK
#include <QQuickWindow>
#include <QQuickItem>
#endif
#include <QVector>
// #include <QDebug>

namespace
{

QRegion roundedRegion(const QRect &rect, int radius, bool topLeft, bool topRight, bool bottomLeft, bool bottomRight)
{
    QRegion region(rect, QRegion::Rectangle);

    if (topLeft) {
        // Round top-left corner.
        const QRegion topLeftCorner(rect.x(), rect.y(), radius, radius, QRegion::Rectangle);
        const QRegion topLeftRounded(rect.x(), rect.y(), 2 * radius, 2 * radius, QRegion::Ellipse);
        const QRegion topLeftEar = topLeftCorner - topLeftRounded;
        region -= topLeftEar;
    }

    if (topRight) {
        // Round top-right corner.
        const QRegion topRightCorner(rect.x() + rect.width() - radius, rect.y(), radius, radius, QRegion::Rectangle);
        const QRegion topRightRounded(rect.x() + rect.width() - 2 * radius, rect.y(), 2 * radius, 2 * radius, QRegion::Ellipse);
        const QRegion topRightEar = topRightCorner - topRightRounded;
        region -= topRightEar;
    }

    if (bottomRight) {
        // Round bottom-right corner.
        const QRegion bottomRightCorner(rect.x() + rect.width() - radius, rect.y() + rect.height() - radius, radius, radius, QRegion::Rectangle);
        const QRegion bottomRightRounded(rect.x() + rect.width() - 2 * radius, rect.y() + rect.height() - 2 * radius, 2 * radius, 2 * radius, QRegion::Ellipse);
        const QRegion bottomRightEar = bottomRightCorner - bottomRightRounded;
        region -= bottomRightEar;
    }

    if (bottomLeft) {
        // Round bottom-left corner.
        const QRegion bottomLeftCorner(rect.x(), rect.y() + rect.height() - radius, radius, radius, QRegion::Rectangle);
        const QRegion bottomLeftRounded(rect.x(), rect.y() + rect.height() - 2 * radius, 2 * radius, 2 * radius, QRegion::Ellipse);
        const QRegion bottomLeftEar = bottomLeftCorner - bottomLeftRounded;
        region -= bottomLeftEar;
    }

    return region;
}

}

namespace SequoiaEleven
{
//___________________________________________________________
BlurHelper::BlurHelper(QObject *parent)
    : QObject(parent)
{
    // Quick windows do not necessarily contain a native-styled control. Track
    // their native surface lifecycle independently of the window-drag code.
    qApp->installEventFilter(this);
}

//___________________________________________________________
void BlurHelper::registerWidget(QWidget *widget, const bool isDolphin)
{
    _isDolphin = isDolphin;
    if (!_widgets.contains(widget)) {
        _widgets.insert(widget);
        connect(widget, &QObject::destroyed, this, [this, widget] { _widgets.remove(widget); });
    }
    update(widget);
}

//___________________________________________________________
void BlurHelper::unregisterWidget(QWidget *widget)
{
    if (_widgets.remove(widget) && widget->windowHandle())
        KWindowEffects::enableBlurBehind(widget->windowHandle(), false);
}

//___________________________________________________________
bool BlurHelper::eventFilter(QObject *object, QEvent *event)
{
#if SEQUOIAELEVEN_HAVE_QTQUICK
    // Qt 6.11 closes a ComboBox on focus-out even when focus moves into its
    // own Popup.Window. Keep that specific handoff alive for the clock's
    // compositor-blurred menus; ordinary focus loss still dismisses them.
    if (event->type() == QEvent::FocusOut && object->inherits("QQuickComboBox")) {
        auto combo = qobject_cast<QQuickItem *>(object);
        if (combo && combo->window() && combo->window()->objectName() == QLatin1String("sequoiaDigitalClockSettings")) {
            auto popup = combo->property("popup").value<QObject *>();
            auto content = popup ? qobject_cast<QQuickItem *>(popup->property("contentItem").value<QObject *>()) : nullptr;
            if (popup && popup->property("visible").toBool() && popup->property("activeFocus").toBool()
                && content && content->window() && content->window() != combo->window()
                && content->window()->transientParent() == combo->window()) {
                return true;
            }
        }
    }
    if (auto window = qobject_cast<QQuickWindow *>(object); window
        && (event->type() == QEvent::Show || event->type() == QEvent::Expose || event->type() == QEvent::PlatformSurface)) {
        if (!_quickWindows.contains(window)) {
            _quickWindows.insert(window);
            connect(window, &QObject::destroyed, this, [this, window] { _quickWindows.remove(window); });
            connect(window, &QQuickWindow::colorChanged, this, [this, window] { updateQuickWindow(window); });
        }
        if (event->type() == QEvent::Show || event->type() == QEvent::Expose
            || (event->type() == QEvent::PlatformSurface
                && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated)) {
            // On Wayland Show may precede creation of the wl_surface. Reapply
            // after creation/exposure, including hide/show and recreated windows.
            QTimer::singleShot(0, this, [this, window = QPointer<QWindow>(window)] {
                if (window)
                    updateQuickWindow(window);
            });
        }
        return false;
    }
#endif
    auto widget = qobject_cast<QWidget *>(object);
    if (!widget || !_widgets.contains(widget) || !widget->isWindow())
        return false;

    switch (event->type()) {
    case QEvent::Show:
    case QEvent::WinIdChange:
    case QEvent::PlatformSurface:
        if (event->type() == QEvent::PlatformSurface
            && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() != QPlatformSurfaceEvent::SurfaceCreated)
            break;
        QTimer::singleShot(0, this, [this, widget = QPointer<QWidget>(widget)] {
            if (widget && _widgets.contains(widget))
                update(widget);
        });
        break;
    case QEvent::PaletteChange:
    case QEvent::WindowStateChange:
    case QEvent::Resize:
        update(widget);
        break;

    default:
        break;
    }

    // Widget blur tracking does not consume events.
    return false;
}

#if SEQUOIAELEVEN_HAVE_QTQUICK
void BlurHelper::updateQuickWindow(QWindow *window) const
{
    auto quick = qobject_cast<QQuickWindow *>(window);
    if (!quick || !quick->isVisible() || !quick->handle())
        return;

    // The Downloads fan explicitly opts out even when hosted by Plasma.
    if (quick->property("sequoiaNoBlur").toBool()) {
        KWindowEffects::enableBlurBehind(quick, false);
        return;
    }

    // This settings frame opts into the application's normal window material.
    // Include its transient menu windows (and nested submenus), which Qt Quick
    // creates as separate surfaces. Other Plasma windows keep their own masks.
    bool sequoiaClockSettings = false;
    for (QWindow *owner = quick; owner; owner = owner->transientParent()) {
        if (owner->objectName() == QLatin1String("sequoiaDigitalClockSettings")) {
            sequoiaClockSettings = true;
            break;
        }
    }

    // Plasma owns its panel and popup blur regions. An empty region here means
    // blur the entire window, replacing its rounded SVG mask with a rectangle
    // on show/expose (including the transparent area around a floating panel).
    const QString appName = QCoreApplication::applicationName();
    if (!sequoiaClockSettings && (appName == QLatin1String("plasmashell") || appName == QLatin1String("plasmawindowed")
        || appName == QLatin1String("plasma") || appName.startsWith(QLatin1String("plasma-"))))
        return;

    // Spectacle's capture UI is a transparent, screen-sized view of the live
    // desktop. Blurring behind it obscures the screen when selecting a recording
    // region. Keep ordinary Spectacle windows on the normal material path.
    if (appName == QLatin1String("spectacle") && quick->inherits("CaptureWindow")) {
        KWindowEffects::enableBlurBehind(quick, false);
        return;
    }

    KWindowEffects::enableBlurBehind(quick, quick->color().alpha() < 255);
}
#endif

//___________________________________________________________
QRegion BlurHelper::blurRegion(QWidget *widget) const
{
    if (!widget->isVisible())
        return QRegion();

    QRect rect = widget->rect();
    if ((qobject_cast<QMenu *>(widget) && !widget->testAttribute(Qt::WA_X11NetWmWindowTypeMenu)) // not a detached menu
             || widget->inherits("QComboBoxPrivateContainer")) {
        return roundedRegion(rect, StyleConfigData::cornerRadius() + 1, true, true, true, true);
    } else {
        // blur entire window
        if (widget->palette().color(QPalette::Window).alpha() < 255){
            QWidget *window = widget->window();
            if (window->isFullScreen() || window->isMaximized())
            return roundedRegion(rect, StyleConfigData::cornerRadius(), false, false, false, false);
            else if (StyleConfigData::floatingTitlebar())
            return roundedRegion(rect, StyleConfigData::cornerRadius(), true, true, true, true);
            else
            return roundedRegion(rect, StyleConfigData::cornerRadius(), false, false, true, true);
        }

        // blur specific widgets
        QRegion region;

        // toolbar and menubar
        if (_translucentTitlebar || StyleConfigData::menuBarOpacity() < 100 || StyleConfigData::toolBarOpacity() < 100
            || StyleConfigData::dolphinSidebarOpacity() < 100) {
            // menubar
            int menubarHeight = 0;
            if (QMainWindow *mw = qobject_cast<QMainWindow *>(widget)) {
                if (QWidget *mb = mw->menuWidget()) {
                    if (mb->isVisible()) {
                        region += mb->rect();
                        menubarHeight = mb->height();
                    }
                }
            }

            QList<QToolBar *> toolbars = widget->window()->findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
            QRect mainToolbar = QRect();

            // just assuming
            Qt::Orientation orientation = Qt::Vertical;

            // find which one is the main toolbar
            for (auto tb : toolbars) {
                // single toolbar
                if (tb && tb->isVisible() && toolbars.length() == 1) {
                    region += QRegion(QRect(tb->pos(), tb->rect().size()));
                    orientation = tb->orientation();
                }

                else if (tb && tb->isVisible()) {
                    if (mainToolbar.isNull()) {
                        mainToolbar = QRect(tb->pos(), tb->rect().size());
                        orientation = tb->orientation();
                    }

                    // test against the previous best candidate
                    else {
                        if ((tb->y() < mainToolbar.y()) || (tb->y() == mainToolbar.y() && tb->x() < mainToolbar.x())) {
                            mainToolbar = QRect(tb->pos(), tb->rect().size());
                            orientation = tb->orientation();
                        }
                    }
                }
            }

            if (mainToolbar.isValid()) {
                // make adjustments
                if (orientation == Qt::Horizontal) {
                    // toolbar may be at the top but not ocupy the whole avaliable width
                    // so we blur the whole area instead
                    if (mainToolbar.y() == 0 || mainToolbar.y() == menubarHeight) {
                        mainToolbar.setX(0);
                        mainToolbar.setWidth(widget->width());
                        region += mainToolbar;
                    }

                    // round corners if it is at the bottom
                    else if (mainToolbar.y() + mainToolbar.height() == widget->height())
                        region += roundedRegion(mainToolbar, StyleConfigData::cornerRadius(), false, false, false, true);

                    // else
                    //     region += mainToolbar;

                } else {
                    // round bottom left
                    if (mainToolbar.x() == 0)
                        region += roundedRegion(mainToolbar, StyleConfigData::cornerRadius(), false, false, true, false);

                    // round bottom right
                    else if (mainToolbar.x() + mainToolbar.width() == widget->width())
                        region += roundedRegion(mainToolbar, StyleConfigData::cornerRadius(), false, false, false, true);

                    // no round corners
                    // else region += mainToolbar; //FIXME: is this valid?
                }
            }
        }

        if (_isDolphin) {
            // dolphin's sidebar
            if (StyleConfigData::dolphinSidebarOpacity() < 100) {
                // sidetoolbar
                if (!_translucentTitlebar) {
                    QToolBar *toolbar = widget->window()->findChild<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
                    if (toolbar) {
                        if (toolbar->orientation() == Qt::Vertical) {
                            if (toolbar->x() == 0)
                                region +=
                                    roundedRegion(QRect(toolbar->pos(), toolbar->rect().size()), StyleConfigData::cornerRadius(), false, false, true, false);
                            else
                                region +=
                                    roundedRegion(QRect(toolbar->pos(), toolbar->rect().size()), StyleConfigData::cornerRadius(), false, false, false, true);
                        }
                    }
                }

                // sidepanels
                QList<QWidget *> sidebars =
                    widget->findChildren<QWidget *>(QRegularExpression("^(places|terminal|info|folders)Dock$"), Qt::FindDirectChildrenOnly);
                for (auto sb : sidebars) {
                    if (sb && sb->isVisible()) {
                        if (sb->x() == 0)
                            region += roundedRegion(QRect(sb->pos(), sb->rect().size()), StyleConfigData::cornerRadius(), false, false, true, false);
                        else if (sb->x() + sb->width() == widget->width())
                            region += roundedRegion(QRect(sb->pos(), sb->rect().size()), StyleConfigData::cornerRadius(), false, false, false, true);
                        else
                            region += QRect(sb->pos(), sb->rect().size());
                    }
                }

                // settings page
                // moved to blurSettingsDialogRegion
            }

            // Dolphin main view
            if (StyleConfigData::dolphinViewOpacity() < 100) {
                QList<QAbstractScrollArea *> itemContainers = widget->findChildren<QAbstractScrollArea *>();
                for (QAbstractScrollArea *container : itemContainers) {
                    if (container->inherits("KItemListContainer") && container->isVisible()) {
                        region += QRect(container->mapTo(widget, QPoint(0, 0)), container->rect().size());
                    }
                }
            }

            /*if( (widget->windowFlags() & Qt::WindowType_Mask) == Qt::Dialog )
            {
                QList<QWidget *> dialogWidgets = widget->findChildren<QWidget *>( QString(), Qt::FindDirectChildrenOnly );
                for( auto w : dialogWidgets )
                {
                    if( w->inherits( "KPageWidget" ) )
                    {
                        QList<QWidget *> KPageWidgets = w->findChildren<QWidget *>( QString(), Qt::FindDirectChildrenOnly );
                        for ( auto wid : KPageWidgets ){
                            if( wid->property( PropertyNames::sidePanelView ).toBool() ) {
                                region += roundedRegion( QRect( wid->pos(), wid->rect().size() ), StyleConfigData::cornerRadius(),  false, false, true, false );
                                break;
                            }
                        }
                    }
                }
            }*/
        }

        // tabs
        if (StyleConfigData::tabBarOpacity() < 100) {
            region += blurTabWidgetRegion(widget);
        }

        // settings

        region += blurSettingsDialogRegion(widget);

        return region;
    }
}

//___________________________________________________________
QRegion BlurHelper::blurTabWidgetRegion(QWidget *widget) const
{
    QRegion region;

    // tabs blur only works on dolphin / konsole main windows

    if (widget->inherits("Konsole::MainWindow") || widget->inherits("DolphinMainWindow")) {
        const QTabWidget *tw = widget->window()->findChild<QTabWidget *>(QString(), Qt::FindDirectChildrenOnly);

        if (tw && tw->isVisible()) {
            if (tw->inherits("DolphinTabWidget") || tw->inherits("Konsole::TabbedViewContainer")) {
                // use the tabbar size as the blur region should only focus on the tabbar not the entire tabwidget
                // the width of the tabbar is always -1 smaller than the tabwidget width so increment it
                QSize tbSize(tw->tabBar()->rect().size());
                tbSize.rwidth() += 1;
                region += roundedRegion(QRect(tw->pos(), tbSize), StyleConfigData::cornerRadius(), false, false, false, false);
            }
        }
    }

    return region;
}

//___________________________________________________________
QRegion BlurHelper::blurSettingsDialogRegion(QWidget *widget) const
{
    QRegion region;

    // settings only change it for konsole or dolphin about window
    if ((widget->windowFlags() & Qt::WindowType_Mask) == Qt::Dialog
        && (widget->inherits("KAboutApplicationDialog") || widget->inherits("KDEPrivate::KAboutKdeDialog"))) {
        QList<QWidget *> widgets = widget->findChildren<QWidget *>();
        if (widgets.length() > 0) {
            for (auto w : widgets) {
                if (qobject_cast<QTabWidget *>(w) && (widget->inherits("KAboutApplicationDialog") || widget->inherits("KDEPrivate::KAboutKdeDialog"))) {
                    // about dialog
                    const QTabWidget *tw = qobject_cast<QTabWidget *>(w);
                    QSize tbSize(tw->rect().size());
                    // the blur region is too small without adjusting the height of the tabbar height
                    tbSize.setHeight(tw->tabBar()->rect().height() + 4);
                    region += roundedRegion(QRect(tw->pos(), tbSize), StyleConfigData::cornerRadius(), false, false, true, false);
                } else {
                    // settings main dialog
                    region += roundedRegion(QRect(w->mapToGlobal(w->pos()), w->rect().size()), StyleConfigData::cornerRadius(), false, false, true, false);
                }
                if (w->inherits("KPageWidget")) {
                    // sidebar
                    QList<QWidget *> KPageWidgets = w->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
                    for (auto wid : KPageWidgets) {
                        if (wid->property(PropertyNames::sidePanelView).toBool()) {
                            region += roundedRegion(QRect(wid->pos(), wid->rect().size()), StyleConfigData::cornerRadius(), false, false, true, false);
                            break;
                        }
                    }
                }
            }
        }
    }

    return region;
}

//___________________________________________________________
void BlurHelper::update(QWidget *widget) const
{
    /*
    directly from bespin code. Supposedly prevent playing with some 'pseudo-widgets'
    that have winId matching some other -random- window
    */
    if (!widget->isVisible() || !widget->windowHandle() || !widget->windowHandle()->handle())
        return;

    QRegion region = blurRegion(widget);
    if (!widget->mask().isEmpty())
        region &= widget->mask();
    // An empty region means no blur, not KWindowEffects' whole-window sentinel.
    KWindowEffects::enableBlurBehind(widget->windowHandle(), !region.isEmpty(), region);
}
}
