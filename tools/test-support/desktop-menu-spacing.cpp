// SPDX-License-Identifier: MIT
// Loaded only into the isolated Plasma shell by verify-desktop-menus.py.
#include <QApplication>
#include <QMenu>
#include <QTimer>
#include <QTest>
#include <QWindow>
#include <QStyle>
#include <QEvent>
#include <cstdlib>
static void require(bool value, const char *message) {
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2); }
}
#include "gwenview-menu-input.h"
class MenuEvents : public QObject {
public:
    int resizes = 0, hides = 0, paints = 0;
    explicit MenuEvents(QMenu *menu) { menu->installEventFilter(this); }
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Resize) ++resizes;
        if (event->type() == QEvent::Hide) ++hides;
        if (event->type() == QEvent::Paint) ++paints;
        return false;
    }
};
static void updateOpenMenu(QMenu *menu) {
    MenuEvents events(menu);
    const QSize original = menu->size();
    QList<QAction *> additions;
    for (int i = 0; i < 4; ++i)
        additions << menu->addAction(QStringLiteral("Dynamically discovered context action %1").arg(i));
    require(menu->size() == original, "live menu retains geometry until action batch completes");
    QTest::qWait(150);
    require(menu->isVisible() && !events.hides, "live action discovery keeps the native popup mapped");
    require(events.resizes == 1 && events.paints > 0, "live action batch resizes once and paints its final contents");
    require(menu->width() > original.width(), "live menu grows to fit discovered actions");
    events.resizes = 0; events.paints = 0;
    for (auto action : additions) { menu->removeAction(action); delete action; }
    QTest::qWait(150);
    require(menu->size() == original && events.resizes == 1 && events.paints > 0,
            "live menu shrinks back and redraws without intermediate resizes");
    require(menu->isVisible() && !events.hides, "live updates preserve native popup grab");
}
static double popupFrameError(const QImage &a, const QImage &b, const QRect &interior) {
    double error = 0;
    for (int y=interior.top(); y<=interior.bottom(); ++y) {
        for (int x=interior.left(); x<=interior.right(); ++x) {
            const auto first=a.pixelColor(x,y), second=b.pixelColor(x,y);
            error += qAbs(first.red()-second.red()) + qAbs(first.green()-second.green()) + qAbs(first.blue()-second.blue());
        }
    }
    return error / (interior.width()*interior.height()*3);
}
static void checkPopupMotion(GwenviewMenuInput &input) {
    auto loaded = [](const QString &name) {
        auto request = QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.kde.kwin.Effects", "isEffectLoaded");
        request << name;
        const auto reply = QDBusConnection::sessionBus().call(request);
        return reply.type() == QDBusMessage::ReplyMessage && reply.arguments().first().toBool();
    };
    require(loaded("sequoia-motion-fadingpopups"), "popup motion effect loaded in private compositor");
    require(loaded("blur") || loaded("sequoia_liquid_glass_v6"), "backdrop effect loaded in private compositor");
    input.move(QPoint(400,350), 1); QTest::qWait(100);
    ei_device_button_button(input.buttons, 0x111, true);
    ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(40);
    ei_device_button_button(input.buttons, 0x111, false);
    ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(2050);
    auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    require(menu, "desktop context menu opens for animation check");
    const QRect interior = menu->geometry().adjusted(24,24,-24,-24);
    const auto opening = gwenviewCompositorCapture();
    opening.save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + "motion-opening.png");
    QTest::qWait(700);
    const auto before = gwenviewCompositorCapture();
    before.save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + "motion-open.png");
    menu->hide(); QTest::qWait(60);
    const auto closing = gwenviewCompositorCapture();
    closing.save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + "motion-closing.png");
    const double openingError = popupFrameError(before, opening, interior);
    const double error = popupFrameError(before, closing, interior);
    fprintf(stderr, "POPUP FRAME: opening material discontinuity %.3f RGB levels\n", openingError);
    require(openingError < 1.0, "popup blur remains present until the opening translation finishes");
    fprintf(stderr, "POPUP FRAME: closing material discontinuity %.3f RGB levels\n", error);
    require(error < 1.0, "popup blur remains present when the closing translation starts");
    QTest::qWait(1800);
    fprintf(stderr, "PASS: actual Plasma desktop popup animation retains its material through opening and closing\n");
}
static void startDesktopMenuCheck() {
    if (!qEnvironmentVariableIsSet("SEQUOIA_DESKTOP_MENU_CHECK")) return;
    QTimer::singleShot(5000, qApp, [] {
        GwenviewMenuInput input;
        if (qEnvironmentVariableIsSet("SEQUOIA_DESKTOP_MENU_MOTION_CHECK")) {
            checkPopupMotion(input);
            std::_Exit(0);
        }
        int levels = 0;
        for (const QPoint point : {QPoint(400, 350), QPoint(1550, 350)}) {
            input.move(point, 1); QTest::qWait(120);
            ei_device_button_button(input.buttons, 0x111, true);
            ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(60);
            ei_device_button_button(input.buttons, 0x111, false);
            ei_device_frame(input.buttons, ei_now(input.context)); input.dispatch(); QTest::qWait(450);
            auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            require(menu && menu->style()->property("sequoiaSharedMenus").toBool(), "desktop right click uses shared native menu style");
            updateOpenMenu(menu);
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
                updateOpenMenu(child);
                const QRect parentRect = menu->geometry(), childRect = child->geometry();
                const int gap = qMax(childRect.left()-parentRect.right()-1, parentRect.left()-childRect.right()-1);
                fprintf(stderr, "DESKTOP GAP: %d at depth %d (%d,%d,%d,%d -> %d,%d,%d,%d)\n", gap, depth,
                    parentRect.x(),parentRect.y(),parentRect.width(),parentRect.height(),childRect.x(),childRect.y(),childRect.width(),childRect.height());
                require(gap >= 6, "desktop cascading menus have separated surfaces, including edge flips");
                for (auto ancestor : chain)
                    require(!childRect.intersects(ancestor->geometry()), "nested desktop submenu avoids every ancestor menu");
                ++levels; chain << child; menu = child;
            }
            require(gwenviewCompositorCapture().save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + QString::number(point.x()) + ".png"), "capture desktop menu chain");
            for (auto it = chain.crbegin(); it != chain.crend(); ++it) { QTest::keyClick(*it, Qt::Key_Escape); QTest::qWait(40); }
        }
        require(levels >= 2, "desktop menu branches were exercised");
        fprintf(stderr, "PASS: actual Plasma desktop right-click menus, live action updates, nested submenu spacing and screen-edge flips\n");
        std::_Exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startDesktopMenuCheck)
