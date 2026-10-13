// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
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
static void check(QQuickItem *picker) {
    auto window = picker->window();
    QTest::qWait(800);
    require(window->isVisible(), "picker visible");
    if (qEnvironmentVariableIsSet("SEQUOIA_PICKER_SECOND_OUTPUT")) {
        require(eval(picker, "desktop.x > 0").toBool(), "picker is on offset second monitor");
    }
    require(eval(picker, "sidePanel.location === PlasmaCore.Types.Floating").toBool(), "floating frame");
    require(eval(picker, "Math.abs(sidePanel.x - (desktop.x + sidePanel.referenceRect.x + (sidePanel.referenceRect.width-sidePanel.width)/2)) <= 1 && Math.abs(sidePanel.y - (desktop.y + sidePanel.referenceRect.y + (sidePanel.referenceRect.height-sidePanel.height)/2)) <= 1").toBool(), "centered in available monitor rectangle");
    require(eval(picker, "sidePanel.width <= sidePanel.referenceRect.width - 20 && sidePanel.height <= sidePanel.referenceRect.height - 20 && width <= 1080 && height <= 760").toBool(), "fits available monitor with margins and desktop cap");
    require(eval(picker, "list.count > 0 && list.columns >= 1 && list.cellWidth >= 140 && list.height > 120").toBool(), "usable catalog and grid geometry");
    require(eval(picker, "root.editModeRect.width > sidePanel.referenceRect.width - 10").toBool(), "centered picker does not reserve sidebar space");
    fprintf(stderr, "GEOMETRY: %s\n", qPrintable(eval(picker, "JSON.stringify({x:sidePanel.x,y:sidePanel.y,width:sidePanel.width,height:sidePanel.height,available:sidePanel.referenceRect,columns:list.columns,count:list.count})").toString()));
    require(window->grabWindow().save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE")), "picker screenshot saved");
    int count = eval(picker, "list.count").toInt();
    eval(picker, "searchInput.text = 'sequoia-no-match-93845'");
    QTest::qWait(300);
    require(eval(picker, "list.count === 0").toBool(), "search filters catalog");
    QTest::keyClick(window, Qt::Key_Escape);
    QTest::qWait(300);
    require(eval(picker, "list.count").toInt() == count && window->isVisible(), "first Escape clears search");
    eval(picker, "searchInput.text = 'Digital Clock'");
    QTest::qWait(300);
    require(eval(picker, "list.count > 0").toBool(), "installed clock discoverable");
    eval(picker, "list.currentIndex = 0; list.forceActiveFocus()");
    int before = eval(picker, "main.containment.applets.length").toInt();
    QTest::keyClick(window, Qt::Key_Return);
    QTest::qWait(500);
    require(eval(picker, "main.containment.applets.length").toInt() == before + 1, "Return adds widget to private desktop");
    auto grid = named(picker, "sequoiaWidgetGrid");
    auto card = qvariant_cast<QQuickItem *>(eval(picker, "list.currentItem"));
    require(grid && card, "selected widget card exists");
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        card->mapToScene(QPointF(card->width()/2, card->height()-25)).toPoint());
    QTest::qWait(300);
    require(eval(picker, "main.containment.applets.length").toInt() == before + 2, "click adds widget to private desktop");
    eval(picker, "searchInput.text = ''; widgetExplorer.widgetsModel.filterType = 'running'; widgetExplorer.widgetsModel.filterQuery = true");
    QTest::qWait(300);
    require(eval(picker, "list.count > 0 && list.count < " + QString::number(count)).toBool(), "running-widget filter works");
    eval(picker, "widgetExplorer.widgetsModel.filterType = ''; widgetExplorer.widgetsModel.filterQuery = ''");
    eval(picker, "main.draggingWidget = true");
    QTest::qWait(100);
    require(eval(picker, "!sidePanel.hideOnWindowDeactivate && sidePanel.outputOnly && sidePanel.opacity < 0.5").toBool(), "drag keeps picker alive and allows drop through");
    eval(picker, "main.draggingWidget = false");
    QTest::qWait(100);
    require(eval(picker, "sidePanel.hideOnWindowDeactivate && !sidePanel.outputOnly && sidePanel.opacity === 1").toBool(), "drag cleanup restores focus and opacity");
    eval(picker, "searchInput.text = ''");
    QTest::qWait(200);
    auto desktopRoot = qvariant_cast<QQuickItem *>(eval(picker, "root"));
    require(desktopRoot, "owning desktop exists");
    QTest::keyClick(window, Qt::Key_Escape);
    QTest::qWait(300);
    require(!window->isVisible(), "Escape dismisses picker");
    eval(desktopRoot, "root.toggleActivityManager()");
    QTest::qWait(500);
    require(window->isVisible() && !window->property("centeredPicker").toBool(), "activity manager still opens as sidebar");
    require(window->width() < 600 && window->opacity() == 1, "sidebar geometry and opacity restored");
    eval(desktopRoot, "root.toggleWidgetExplorer(root.containment.plasmoid)");
    QTest::qWait(500);
    require(window->property("centeredPicker").toBool(), "picker reopens from activity manager");
    QQuickWindow other;
    other.setGeometry(0, 0, 150, 100);
    other.show();
    other.requestActivate();
    QTest::qWait(500);
    require(!window->isVisible(), "activating another window dismisses picker");
    fprintf(stderr, "PASS: centered responsive native picker, catalog search/filter, click/keyboard add, drag lifecycle, Escape/focus dismissal, activity sidebar\n");
    qApp->quit();
}
static void start() {
    auto timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, [timer] {
        static bool opened = false;
        for (auto window : QGuiApplication::allWindows()) {
            if (!window->isVisible()) continue;
            if (auto view = qobject_cast<QQuickWindow *>(window)) {
                if (!opened && qEnvironmentVariableIsSet("SEQUOIA_PICKER_SECOND_OUTPUT")) {
                    if (auto desktop = named(view->contentItem(), "sequoiaDesktopView")) {
                        if (eval(desktop, "desktop.x > 0 && root.containment !== null").toBool()) {
                            opened = true;
                            eval(desktop, "root.toggleWidgetExplorer(root.containment.plasmoid)");
                        }
                    }
                }
                if (auto picker = named(view->contentItem(), "sequoiaWidgetPicker")) {
                    timer->stop();
                    check(picker);
                    return;
                }
            }
        }
    });
    timer->start(250);
    QTimer::singleShot(25000, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            fprintf(stderr, "WINDOW: %s %s x=%d visible=%d\n", window->metaObject()->className(), qPrintable(window->objectName()), window->x(), window->isVisible());
            if (auto view = qobject_cast<QQuickWindow *>(window)) {
                if (auto desktop = named(view->contentItem(), "sequoiaDesktopView")) {
                    fprintf(stderr, "DESKTOP: %s\n", qPrintable(eval(desktop, "JSON.stringify({x:desktop.x, containment:!!root.containment})").toString()));
                }
            }
        }
        require(false, "picker test timed out");
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
