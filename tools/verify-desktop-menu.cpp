// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QMenu>
#include <QStyle>
#include <QStyleFactory>
#include <cstdlib>

static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2); }
}
static QVariant eval(QObject *object, const QString &code) {
    QQmlExpression expression(qmlContext(object), object, code);
    auto value = expression.evaluate();
    if (expression.hasError()) fprintf(stderr, "%s\n", qPrintable(expression.error().toString()));
    require(!expression.hasError(), "QML expression");
    return value;
}
static QQuickItem *named(QQuickItem *root, const QString &name) {
    if (root->objectName() == name) return root;
    for (auto child : root->childItems()) if (auto found = named(child, name)) return found;
    return nullptr;
}
static void dump(QQuickItem *item, int depth = 0) {
    fprintf(stderr, "%*s%s %s %gx%g visible=%d\n", depth, "", item->metaObject()->className(),
        qPrintable(item->objectName()), item->width(), item->height(), item->isVisible());
    if (item->property("text").isValid()) fprintf(stderr, "TEXT: %s\n", qPrintable(item->property("text").toString()));
    for (auto child : item->childItems()) dump(child, depth + 1);
}
static void start() {
    QTimer::singleShot(8000, qApp, [] {
        QQuickItem *root = nullptr;
        for (auto window : QGuiApplication::allWindows()) {
            if (auto view = qobject_cast<QQuickWindow *>(window)) {
                root = named(view->contentItem(), "sequoiaDesktopMenu");
                if (root) break;
            }
        }
        if (!root) {
            for (auto window : QGuiApplication::allWindows()) {
                fprintf(stderr, "Window %s %s\n", window->metaObject()->className(), qPrintable(window->title()));
                window->dumpObjectTree();
                if (auto view = qobject_cast<QQuickWindow *>(window)) dump(view->contentItem());
            }
        }
        require(root, "desktop menu loaded in Plasma");
        auto style = QStyleFactory::create("SequoiaEleven");
        require(style, "Sequoia application style is installed");
        QApplication::setStyle(style);
        QQuickItem *titleRoot = nullptr;
        for (auto window : QGuiApplication::allWindows()) {
            if (auto view = qobject_cast<QQuickWindow *>(window)) {
                titleRoot = named(view->contentItem(), "sequoiaWindowTitle");
                if (titleRoot) break;
            }
        }
        require(root->property("desktopActive").toBool(), "desktop menu visible without active application");
        if (eval(root, "fullRepresentationItem.count").toInt() != 4) dump(root);
        require(eval(root, "fullRepresentationItem.count === 4").toBool(), "four top-level menus");
        auto button = named(root, "desktopMenuButton-0");
        require(button && button->width() > 20, "Plasma menu button has usable geometry");
        // Synthetic QTest events do not carry a compositor-issued input serial.
        // Exercise popup interaction offscreen, and real task focus on Wayland.
        if (!qEnvironmentVariableIsSet("SEQUOIA_DESKTOP_MENU_NATIVE")) {
            QTest::mouseClick(root->window(), Qt::LeftButton, Qt::NoModifier,
                button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint());
            QTest::qWait(250);
            require(eval(root, "popupOpen").toBool(), "Plasma menu opens");
            auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            require(menu && menu->actions().size() == 4, "native QMenu contains all menu rows");
            require(menu->style() == QApplication::style(), "menu inherits the application style");
            require(!eval(root, "popup.preferSeamlessEdges").toBool(), "menu uses a floating frame");
            require(menu->grab().save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + "-plasma.png"), "native popup preview saved");
            // Trigger the actual restart item, then cancel: never restart a user's shell in a test.
            QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier,
                              menu->actionGeometry(menu->actions().last()).center());
            QTest::qWait(250);
            require(eval(root, "messageDialog.visible && confirmRestart").toBool(), "restart requires confirmation");
            eval(root, "messageDialog.visible = false");
            QTest::qWait(400);
            for (int i = 1; i < 4; ++i) {
                eval(root, QString("fullRepresentationItem.openMenu(%1)").arg(i));
                QTest::qWait(150);
                require(eval(root, QString("popupOpen && menuIndex === %1").arg(i)).toBool(), "menu opens");
                menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                require(menu && menu->actions().size() == (i == 1 ? 5 : i == 2 ? 6 : 2), "native menu rows update when changing menus");
                QTest::keyClick(menu, Qt::Key_Escape);
                QTest::qWait(200);
                require(!eval(root, "popupOpen").toBool(), "Escape dismisses native popup");
            }
            require(eval(root, "shellQuote(\"a'b\")").toString() == QString::fromLatin1("'a'\\''b'"), "command quoting");
            fprintf(stderr, "PASS: four menus load and open; restart confirmation cancels safely\n");
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_DESKTOP_MENU_NATIVE")) {
            require(titleRoot, "window title widget loaded");
            QProcess client;
            auto env = QProcessEnvironment::systemEnvironment();
            env.remove("LD_PRELOAD");
            client.setProcessEnvironment(env);
            client.start("python3", {"-c", "from PySide6.QtWidgets import QApplication, QWidget; "
                "a=QApplication([]); w=QWidget(); w.setWindowTitle('Desktop menu focus test'); "
                "w.resize(400,300); w.show(); w.activateWindow(); a.exec()"});
            require(client.waitForStarted(), "test application launched");
            for (int i=0; i<50 && !root->property("applicationActive").toBool(); ++i) QTest::qWait(100);
            QTest::qWait(250);
            require(!root->property("desktopActive").toBool(), "menu hidden for application without exported menus");
            require(titleRoot->property("existsWindowActive").toBool(), "active application title is shown");
            eval(root, "tasks.requestToggleMinimized(tasks.activeTask)");
            for (int i=0; i<50 && !root->property("desktopActive").toBool(); ++i) QTest::qWait(100);
            require(root->property("desktopActive").toBool(), "desktop activation restores menu with application still open");
            require(!titleRoot->property("existsWindowActive").toBool(), "minimizing clears application title without a desktop click");
            client.terminate();
            client.waitForFinished();
            fprintf(stderr, "PASS: real Wayland application focus hides menus; minimizing the last window restores them\n");
            // Reproduce a compositor/model transition which still marks the
            // minimized task active. Neither widget may display it.
            eval(root, "const fake = Qt.createQmlObject('import QtQml.Models; ListModel {}', root); fake.append({IsActive:true, IsMinimized:false}); windows.model = fake");
            QTest::qWait(100);
            require(!root->property("desktopActive").toBool(), "fixture represents an active window");
            eval(root, "windows.model.setProperty(0, 'IsMinimized', true)");
            QTest::qWait(100);
            require(root->property("desktopActive").toBool(), "stale active flag on minimized window does not hide desktop menu");
            eval(root, "windows.model = tasks");
            eval(titleRoot, "windowInfoLoader.item.activeTaskItem = Qt.createQmlObject('import QtQuick; Item { property bool isActive:true; property bool isMinimized:true; property bool isMaximized:false; property string appName: \"Hidden\"; property string title: \"Hidden\" }', root)");
            require(!titleRoot->property("existsWindowActive").toBool(), "title rejects a minimized task with a stale active flag");
            eval(titleRoot, "windowInfoLoader.item.activeTaskItem.destroy(); windowInfoLoader.item.activeTaskItem = null");
            fprintf(stderr, "PASS: both widgets reject stale active flags on minimized windows\n");
            auto panel = root->window();
            require(panel->grabWindow().save(qEnvironmentVariable("SEQUOIA_DESKTOP_MENU_CAPTURE") + ".png"), "panel preview saved");
            // Exercise the real scripting API used by Configure Desktop.
            eval(root, "configureDesktop()");
            QTest::qWait(1000);
            require(!eval(root, "messageDialog.visible").toBool(), "desktop settings action succeeds");
            bool configFound = false;
            for (auto window : QGuiApplication::allWindows()) {
                if (QByteArray(window->metaObject()->className()).contains("DesktopView") || window == panel) continue;
                if (window->isVisible() && window->title().contains("Desktop")) configFound = true;
            }
            require(configFound, "desktop configuration window opens");
            fprintf(stderr, "PASS: desktop settings opens for the panel's screen\n");
        }
        qApp->quit();
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
