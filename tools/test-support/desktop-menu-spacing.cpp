// SPDX-License-Identifier: MIT
// Loaded only into the isolated Plasma shell by verify-desktop-menus.py.
#include <QApplication>
#include <QMenu>
#include <QTimer>
#include <QTest>
#include <QWindow>
#include <QStyle>
#include <cstdlib>
static void require(bool value, const char *message) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2); }
}
#include "gwenview-menu-input.h"
static void startDesktopMenuCheck() {
    if (!qEnvironmentVariableIsSet("SEQUOIA_DESKTOP_MENU_CHECK")) return;
    QTimer::singleShot(5000, qApp, [] {
        GwenviewMenuInput input;
        int levels = 0;
        for (const QPoint point : {QPoint(400, 350), QPoint(1550, 350)}) {
            input.move(point, 1); QTest::qWait(120);
            ei_device_button_button(input.buttons, 0x111, true);
            ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(60);
            ei_device_button_button(input.buttons, 0x111, false);
            ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(450);
            auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            require(menu && menu->style()->property("sequoiaSharedMenus").toBool(), "desktop right click uses shared native menu style");
            QList<QMenu *> chain{menu};
            for (int depth = 0; depth < 2; ++depth) {
                QAction *branch = nullptr;
                for (auto action : menu->actions()) {
                    if (!action->isEnabled() || !action->menu()) continue;
                    if (!branch || action->text().contains("Icons")) branch = action;
                }
                if (!branch) break;
                menu->setActiveAction(branch); QTest::keyClick(menu, Qt::Key_Right); QTest::qWait(250);
                auto child = branch->menu(); require(child->isVisible(), "desktop submenu opens");
                const QRect parentRect = menu->geometry(), childRect = child->geometry();
                const int gap = qMax(childRect.left()-parentRect.right()-1, parentRect.left()-childRect.right()-1);
                fprintf(stderr, "DESKTOP GAP: %d at depth %d (%d,%d,%d,%d -> %d,%d,%d,%d)\n", gap, depth,
                    parentRect.x(),parentRect.y(),parentRect.width(),parentRect.height(),childRect.x(),childRect.y(),childRect.width(),childRect.height());
                require(gap >= 6, "desktop cascading menus have separated surfaces, including edge flips");
                ++levels; chain << child; menu = child;
            }
            require(gwenviewCompositorCapture().save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + QString::number(point.x()) + ".png"), "capture desktop menu chain");
            for (auto it = chain.crbegin(); it != chain.crend(); ++it) { QTest::keyClick(*it, Qt::Key_Escape); QTest::qWait(40); }
        }
        require(levels >= 2, "desktop menu branches were exercised");
        fprintf(stderr, "PASS: actual Plasma desktop right-click menus, nested submenu spacing and screen-edge flips\n");
        std::_Exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startDesktopMenuCheck)
