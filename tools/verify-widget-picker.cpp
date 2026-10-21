// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <QFileInfo>
#include <QDrag>
#include <cstdlib>

static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2); }
}
#include "test-support/picker-input.h"
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
static QQuickItem *resizeHandle(QQuickItem *root) {
    if (root->property("resizeCorner").isValid() && root->property("resizeCorner").toInt() == 5) return root;
    for (auto child : root->childItems()) if (auto found = resizeHandle(child)) return found;
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
    eval(picker, "searchInput.text = 'Sequoia Grid Test'");
    QTest::qWait(300);
    require(eval(picker, "list.count === 1").toBool(), "test widget discoverable");
    eval(picker, "list.currentIndex = 0; list.forceActiveFocus()");
    int before = eval(picker, "main.containment.applets.length").toInt();
    QTest::keyClick(window, Qt::Key_Return);
    QTest::qWait(500);
    require(eval(picker, "GridSession.phase === 'destination'").toBool(), "Return opens destination chooser");
    require(window->grabWindow().save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE") + ".destinations.png"), "destination map screenshot");
    fprintf(stderr, "TARGETS: %s\n", qPrintable(eval(picker, "JSON.stringify(GridSession.targets.map(t=>({kind:t.kind,screen:t.screen,ready:t.ready,geometry:t.geometry})))").toString()));
    require(eval(picker, "GridSession.targets.some(t=>t.kind === 'desktop' && t.ready) && GridSession.targets.some(t=>t.kind === 'panel' && t.ready)").toBool(), "live desktop and panel destinations");
    eval(picker, "GridSession.choose(GridSession.targets.find(t=>t.kind === 'desktop' && t.screen === main.containment.screen))");
    require(eval(picker, "GridSession.phase === 'preset' && GridSession.preset.columns === 2 && GridSession.preset.rows === 2").toBool(), "first preset is square");
    eval(picker, "GridSession.presetIndex = 2");
    QTest::qWait(200);
    require(window->grabWindow().save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE") + ".presets.png"), "preset screenshot");
    eval(picker, "GridSession.place()");
    QTest::qWait(200);
    require(eval(picker, "main.opacity === 0 && sidePanel.outputOnly && !sidePanel.hideOnWindowDeactivate").toBool(), "picker hidden for placement without destroying drag source");
    auto placement = qvariant_cast<QQuickItem *>(eval(picker, "GridSession.destination"));
    require(placement, "placement desktop exists");
    QTest::qWait(500);
    require(eval(placement, "active && visible && width > 0 && height > 0").toBool(), "placement grid is visible");
    fprintf(stderr, "GRID: %s\n", qPrintable(eval(placement, "JSON.stringify({active:active,visible:visible,width:width,height:height,columns:columns,rows:rows,parentVisible:parent.visible,opacity:opacity})").toString()));
    require(placement->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE") + ".grid.png"), "placement grid screenshot");
    require(eval(placement, "available(0,1)").toBool(), "selected grid cells available");
    QTest::mouseClick(placement->window(), Qt::LeftButton, Qt::NoModifier,
                      placement->mapToScene(QPointF(12, 108)).toPoint());
    QTest::qWait(400);
    require(eval(picker, "main.containment.applets.length").toInt() == before + 1, "guided placement adds exactly one widget");
    require(eval(picker, "GridSession.phase === 'catalog' && sidePanel.opacity === 1").toBool(), "picker returns after placement");
    require(eval(placement, "layout.children.some(c=>c.applet && c.applet.plasmoid.pluginName === 'org.sequoia.gridtest' && c.width === 384 && c.height === 192)").toBool(), "preset footprint preserved by native layout");
    auto grid = named(picker, "sequoiaWidgetGrid");
    auto card = qvariant_cast<QQuickItem *>(eval(picker, "list.currentItem"));
    require(grid && card, "selected widget card exists");
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        card->mapToScene(QPointF(card->width()/2, card->height()-25)).toPoint());
    QTest::qWait(300);
    require(eval(picker, "GridSession.phase === 'destination'").toBool(), "click opens destination chooser");
    eval(picker, "GridSession.choose(GridSession.targets.find(t=>t.kind === 'panel' && t.ready))");
    QTest::qWait(300);
    require(eval(picker, "GridSession.targets.some(t=>t.kind === 'panel' && t.containment.plasmoid.applets.some(a=>a.pluginName === 'org.sequoia.gridtest'))").toBool(), "panel widget added to selected panel");
    eval(picker, "GridSession.begin('org.sequoia.gridtest', 'Grid Test', 'view-grid', false); GridSession.choose(GridSession.targets.find(t=>t.kind === 'desktop' && t.screen === main.containment.screen)); GridSession.presetIndex = 2; GridSession.place()");
    QTest::qWait(100);
    fprintf(stderr, "OCCUPANCY: %s\n", qPrintable(eval(placement, "JSON.stringify(layout.children.filter(c=>c.applet).map(c=>({id:c.applet.plasmoid.id,x:c.x,y:c.y,w:c.width,h:c.height,managed:layout.itemIsManaged(c),edit:c.configOverlayVisible})))").toString()));
    require(!eval(placement, "placeAt(0,1)").toBool(), "occupied cells reject placement");
    QTest::keyClick(placement->window(), Qt::Key_Escape);
    QTest::qWait(200);
    require(eval(picker, "GridSession.phase === 'catalog'").toBool(), "Escape cancels placement");
    // Exercise Plasma's actual resize handle, including the in-progress snap.
    eval(picker, "sidePanel.hideOnWindowDeactivate = false");
    auto container = qvariant_cast<QQuickItem *>(eval(placement, "layout.children.find(c=>c.applet && c.applet.plasmoid.pluginName === 'org.sequoia.gridtest')"));
    require(container, "native applet container");
    eval(container, "editMode = true; configOverlayVisible = true");
    QTest::qWait(500);
    auto handle = resizeHandle(container);
    require(handle && handle->isVisible(), "native bottom-right resize handle");
    auto desktopWindow = container->window();
    QPoint start = handle->mapToScene(QPointF(handle->width()/2, handle->height()/2)).toPoint();
    QTest::mousePress(desktopWindow, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(desktopWindow, start + QPoint(-175, 175), 50);
    QTest::qWait(200);
    require(int(container->width()) % 96 == 0 && int(container->height()) % 96 == 0, "resize snaps while pointer is held");
    QTest::mouseRelease(desktopWindow, Qt::LeftButton, Qt::NoModifier, start + QPoint(-175, 175));
    QTest::qWait(300);
    require(container->width() == 192 && container->height() == 384, "native resize produces tall footprint");
    auto face = named(container, "sequoiaGridTestFace");
    require(face && face->property("arrangement").toString() == "Tall", "widget changes layout after resize");
    eval(container, "configOverlayVisible = false; editMode = false");
    // Exercise the native drag source, compositor routing and desktop DropArea.
    bool dragChecked = false;
    PickerInput input;
    window->requestActivate();
    QTest::qWait(300);
    card = qvariant_cast<QQuickItem *>(eval(picker, "list.currentItem"));
    QPoint dragStart = card->mapToScene(QPointF(card->width()/2, 45)).toPoint();
    QTimer::singleShot(1500, [&] {
        fprintf(stderr, "DRAG: %s\n", qPrintable(eval(picker, "JSON.stringify({phase:GridSession.phase,drag:main.draggingWidget,visible:sidePanel.visible})").toString()));
        require(eval(picker, "GridSession.phase === 'drag' && main.draggingWidget").toBool(), "native card gesture starts grid drag");
        require(eval(picker, "sidePanel.outputOnly && main.opacity === 0 && sidePanel.backgroundHints === PlasmaCore.Dialog.NoBackground").toBool(), "picker has no content, frame or input during drag");
        QPoint dropPoint = placement->mapToScene(QPointF(5*96 + 12, 12)).toPoint();
        input.move(desktopWindow->mapToGlobal(dropPoint), desktopWindow->devicePixelRatio());
        dragChecked = true;
        QTimer::singleShot(250, [&] { input.button(false); });
    });
    input.move(window->mapToGlobal(dragStart), window->devicePixelRatio());
    QTest::qWait(500);
    input.button(true);
    QTest::qWait(200);
    fprintf(stderr, "PRESS: %s\n", qPrintable(eval(card, "JSON.stringify({pressed:pressed,point:mapToGlobal(width/2,45)})").toString()));
    input.move(window->mapToGlobal(dragStart + QPoint(40,40)), window->devicePixelRatio());
    QTest::qWait(2000);
    require(dragChecked, "native drag lifecycle completed");
    QTest::qWait(300);
    require(window->isVisible(), "picker returns after native drop");
    require(eval(placement, "layout.children.some(c=>c.applet && c.x === 480 && c.y === 0 && c.width === 192 && c.height === 192)").toBool(), "drag uses first preset at exact grid position");
    require(eval(picker, "main.containment.applets.length").toInt() == before + 2, "drag creates exactly one instance");
    // An invalid native drop must not fall through to Folder View and add an
    // overlapping widget using Plasma's original drop handler.
    window->requestActivate();
    QTest::qWait(150);
    QTimer::singleShot(400, [&] {
        require(eval(picker, "GridSession.phase === 'drag'").toBool(), "occupied-cell drag starts");
        input.move(desktopWindow->mapToGlobal(placement->mapToScene(QPointF(12,108)).toPoint()), desktopWindow->devicePixelRatio());
        QTimer::singleShot(200, [&] { input.button(false); });
    });
    input.move(window->mapToGlobal(dragStart), window->devicePixelRatio());
    QTest::qWait(50);
    input.button(true);
    QTest::qWait(50);
    input.move(window->mapToGlobal(dragStart + QPoint(40,40)), window->devicePixelRatio());
    QTest::qWait(900);
    require(window->isVisible() && eval(picker, "main.containment.applets.length").toInt() == before + 2, "occupied native drop adds no widget");
    window->requestActivate();
    QTest::qWait(150);
    QTimer::singleShot(500, [&] {
        require(eval(picker, "GridSession.phase === 'drag'").toBool(), "second native drag starts");
        QDrag::cancel();
        input.button(false);
    });
    input.move(window->mapToGlobal(dragStart), window->devicePixelRatio());
    QTest::qWait(50);
    input.button(true);
    QTest::qWait(50);
    input.move(window->mapToGlobal(dragStart + QPoint(40,40)), window->devicePixelRatio());
    QTest::qWait(800);
    require(window->isVisible() && eval(picker, "GridSession.phase === 'catalog' && main.containment.applets.length === " + QString::number(before + 2)).toBool(), "cancelled native drag restores picker without adding widget");
    eval(picker, "sidePanel.hideOnWindowDeactivate = Qt.binding(()=>!main.preventWindowHide)");
    eval(picker, "searchInput.text = ''; widgetExplorer.widgetsModel.filterType = 'running'; widgetExplorer.widgetsModel.filterQuery = true");
    QTest::qWait(300);
    require(eval(picker, "list.count > 0 && list.count < " + QString::number(count)).toBool(), "running-widget filter works");
    eval(picker, "widgetExplorer.widgetsModel.filterType = ''; widgetExplorer.widgetsModel.filterQuery = ''");
    eval(picker, "main.draggingWidget = true");
    QTest::qWait(100);
    require(eval(picker, "!sidePanel.hideOnWindowDeactivate && sidePanel.outputOnly && main.opacity === 0").toBool(), "drag keeps picker alive and allows drop through");
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
    fprintf(stderr, "PASS: picker search, destination map, desktop/panel placement, presets, occupied cells, native live resize, adaptive layout, drag/drop/cancel, Escape, activity sidebar\n");
    qApp->quit();
}
static void start() {
    auto timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, [timer] {
        if (!QFileInfo::exists(qEnvironmentVariable("SEQUOIA_PICKER_READY"))) return;
        static bool opened = false;
        for (auto window : QGuiApplication::allWindows()) {
            if (!window->isVisible()) continue;
            if (auto view = qobject_cast<QQuickWindow *>(window)) {
                if (qEnvironmentVariableIsSet("SEQUOIA_GRID_RESTORE")) {
                    if (auto grid = named(view->contentItem(), "sequoiaDesktopGrid")) {
                        if (eval(grid, "layout && layout.children.filter(c=>c.applet && c.applet.plasmoid.pluginName === 'org.sequoia.gridtest').length === 2").toBool()) {
                            timer->stop();
                            QTest::qWait(600);
                            require(eval(grid, "layout.children.some(c=>c.applet && c.x === 0 && c.y === 96 && c.width === 192 && c.height === 384)").toBool(), "resized geometry restored after shell restart");
                            require(eval(grid, "layout.children.some(c=>c.applet && c.x === 480 && c.y === 0 && c.width === 192 && c.height === 192)").toBool(), "dragged geometry restored after shell restart");
                            fprintf(stderr, "PASS: widget sizes and positions survive a Plasma restart\n");
                            qApp->quit();
                            return;
                        }
                    }
                    continue;
                }
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
