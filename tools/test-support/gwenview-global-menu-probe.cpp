// SPDX-License-Identifier: MIT
// Runs only in the private test Plasma shell, never in the user's shell.
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTimer>
#include <QSet>
#include <QDir>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <functional>
#include <cstdlib>
static void require(bool value, const char *message) {
    if (!value) {
        QFile failed(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-switches-failed");
        failed.open(QIODevice::WriteOnly); failed.close();
        fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2);
    }
}
#include "gwenview-menu-input.h"

class MenuPaintWatch : public QObject {
public:
    int visibleResizes = 0;
    int paints = 0;
    bool eventFilter(QObject *object, QEvent *event) override {
        if (auto menu = qobject_cast<QMenu *>(object); menu && event->type() == QEvent::Resize && menu->isVisible()) ++visibleResizes;
        if (qobject_cast<QMenu *>(object) && event->type() == QEvent::Paint) ++paints;
        return false;
    }
};

static void checkMenuSwitches()
{
    QObject *applet = nullptr;
    std::function<void(QQuickItem *)> visit = [&](QQuickItem *item) {
        if (applet) return;
        auto candidate = item->property("plasmoid").value<QObject *>();
        if (candidate && candidate->inherits("AppMenuApplet")) { applet = candidate; return; }
        for (auto child : item->childItems()) visit(child);
    };
    for (auto window : QGuiApplication::allWindows()) {
        auto quick = qobject_cast<QQuickWindow *>(window);
        if (!quick) continue;
        visit(quick->contentItem());
        if (applet) break;
    }
    require(applet, "locate real global-menu applet");
    auto grid = applet->property("buttonGrid").value<QQuickItem *>();
    require(grid && grid->window(), "global-menu buttons are laid out");
    QList<QQuickItem *> buttons;
    for (auto child : grid->childItems())
        if (child->isVisible() && child->property("buttonIndex").isValid()) buttons << child;
    require(buttons.size() >= 4, "global-menu categories available");
    GwenviewMenuInput input;
    MenuPaintWatch watch;
    qApp->installEventFilter(&watch);
    QMenu *previous = nullptr;
    QMap<int, QSize> sizes;
    for (int turn = 0; turn < buttons.size() * 2; ++turn) {
        auto button = buttons.at(turn % buttons.size());
        const int index = button->property("buttonIndex").toInt();
        const QPoint point = button->window()->mapToGlobal(button->mapToScene(QPointF(button->width()/2,button->height()/2)).toPoint());
        input.move(point, button->window()->devicePixelRatio());
        if (!turn) { input.button(true); QTest::qWait(30); input.button(false); }
        QTest::qWait(90);
        auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (!menu || !menu->isVisible()) fprintf(stderr, "MISSING MENU: requested=%d current=%d button=%s\n", index,
            applet->property("currentIndex").toInt(), qPrintable(button->property("text").toString()));
        require(menu && menu->isVisible(), "global menu remains open during category hover");
        require(applet->property("currentIndex").toInt() == index, "hover selects requested global-menu category");
        if (index == 3) require(menu->width() < sizes.value(2).width(), "Go shrinks back to its natural width after View");
        if (sizes.contains(index)) require(menu->size() == sizes.value(index), "category size stays stable across repeated switches");
        sizes.insert(index, menu->size());
        fprintf(stderr, "SWITCH: %s index=%d menu=%p previous=%p size=%dx%d first=%s\n", qPrintable(button->property("text").toString()), index,
            static_cast<void *>(menu), static_cast<void *>(previous), menu->width(), menu->height(), qPrintable(menu->actions().value(0)->text()));
        if (turn < 4 && qEnvironmentVariableIsSet("SEQUOIA_GLOBAL_MENU_CAPTURE"))
            require(gwenviewCompositorCapture().save(qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY") + QStringLiteral("/global-switch-%1.png").arg(turn)), "capture global-menu switch");
        previous = menu;
        QTest::qWait(180);
    }
    for (int index : {0, 3}) {
        const int painted = watch.paints;
        require(QMetaObject::invokeMethod(applet, "trigger", Q_ARG(QQuickItem *, buttons.at(index)), Q_ARG(int, index)), "switch category directly");
        require(watch.paints > painted, "replacement category is painted before the geometry transaction returns");
    }
    auto button = buttons.at(1);
    const QPoint point = button->window()->mapToGlobal(button->mapToScene(QPointF(button->width()/2,button->height()/2)).toPoint());
    input.move(point, button->window()->devicePixelRatio()); QTest::qWait(120);
    auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    require(menu && applet->property("currentIndex").toInt() == 1, "return to Edit category");
    menu->setActiveAction(nullptr);
    QTest::keyClick(menu, Qt::Key_Right); QTest::qWait(120);
    require(applet->property("currentIndex").toInt() == 2, "Right selects next category");
    menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    require(menu, "next category is open");
    QTest::keyClick(menu, Qt::Key_Left); QTest::qWait(120);
    require(applet->property("currentIndex").toInt() == 1, "Left selects previous category");
    menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    require(menu, "previous category is open");
    QTest::keyClick(menu, Qt::Key_Escape); QTest::qWait(100);
    require(!QApplication::activePopupWidget() && applet->property("currentIndex").toInt() == -1, "Escape clears popup and active category");
    input.button(true); QTest::qWait(30); input.button(false); QTest::qWait(120);
    require(QApplication::activePopupWidget(), "click reopens a dismissed global menu");
    input.move(QPoint(1400,900),1); input.button(true); QTest::qWait(30); input.button(false); QTest::qWait(120);
    require(!QApplication::activePopupWidget() && applet->property("currentIndex").toInt() == -1, "outside click dismisses menu");
    fprintf(stderr, "VISIBLE MENU RESIZES: %d\n", watch.visibleResizes);
    require(watch.visibleResizes <= buttons.size() * 2 + 6, "category exchange avoids intermediate per-action popup resizes");
    QFile done(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-switches-done");
    require(done.open(QIODevice::WriteOnly), "finish global-menu switch check");
    done.close();
    fprintf(stderr, "PASS: real global-menu category hover, keyboard navigation, dismissal and atomic repaints\n");
}
static void startGlobalMenuProbe()
{
    if (!qEnvironmentVariableIsSet("SEQUOIA_GLOBAL_MENU_PROBE")) return;
    QTimer::singleShot(1500, qApp, [] {
        auto timer = new QTimer(qApp);
        QObject::connect(timer, &QTimer::timeout, qApp, [timer] {
            static int samples = 0;
            static bool switchesStarted = false;
            int populated = 0;
            QList<QMenu *> menus;
            for (auto widget : QApplication::topLevelWidgets()) if (auto menu = qobject_cast<QMenu *>(widget)) menus << menu;
            for (auto menu : menus) {
                QSet<int> ids;
                bool imported = false;
                for (auto action : menu->actions()) {
                    const QVariant id = action->property("_dbusmenu_id");
                    if (!id.isValid()) continue;
                    imported = true;
                    if (ids.contains(id.toInt())) {
                        fprintf(stderr, "FAIL: duplicated imported global-menu command: %s id=%d\n", qPrintable(action->text()), id.toInt());
                        std::_Exit(2);
                    }
                    ids.insert(id.toInt());
                }
                if (!imported) continue;
                ++populated;
                // Exercise the widget's real asynchronous DBus importer, just
                // as opening File/Edit/View requests an up-to-date action list.
                QMetaObject::invokeMethod(menu, "aboutToShow");
            }
            if (populated >= 3) {
                ++samples;
                QFile ready(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-ready");
                if (ready.open(QIODevice::WriteOnly)) ready.write(QByteArray::number(samples));
            }
            if (samples >= 3 && !switchesStarted && qEnvironmentVariableIsSet("SEQUOIA_GLOBAL_MENU_SWITCH_CHECK")
                && QFile::exists(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-input-ready")) {
                switchesStarted = true;
                timer->stop();
                checkMenuSwitches();
                timer->start();
            }
            if (QFile::exists(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-done")) {
                if (samples < 20) { fprintf(stderr, "FAIL: global menu did not receive enough updates\n"); std::_Exit(2); }
                fprintf(stderr, "PASS: %d samples of real Plasma global-menu imports without duplicate command IDs\n", samples);
                std::_Exit(0);
            }
        });
        timer->start(80);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startGlobalMenuProbe)
