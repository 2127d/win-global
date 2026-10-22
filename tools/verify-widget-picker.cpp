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
static QQuickItem *moveHandle(QQuickItem *root, QQuickItem *container) {
    auto drag = root->property("drag").value<QObject *>();
    if (drag && drag->property("target").value<QObject *>() == container) return root;
    for (auto child : root->childItems()) if (auto found = moveHandle(child, container)) return found;
    return nullptr;
}
static void checkAnalogClock(QQuickItem *picker, QQuickWindow *wizardWindow, QQuickItem *grid) {
    for (int preset = 0; preset < 2; ++preset) {
        eval(picker, "GridSession.begin('org.sequoia.analogclock', 'Sequoia Analog Clock', 'clock', false); GridSession.choose(GridSession.targets.find(t=>t.kind === 'desktop' && t.screen === main.containment.screen))");
        require(eval(picker, "GridSession.definition.presets.length === 2").toBool(), "clock has exactly two presets");
        eval(picker, "GridSession.presetIndex = " + QString::number(preset));
        QTest::qWait(350);
        auto preview = named(wizardWindow->contentItem(), "sequoiaAnalogFace");
        require(preview && preview->property("rounded").toBool() == (preset == 0), "preset preview selects matching clock face");
        require(wizardWindow->grabWindow().save(qEnvironmentVariable("SEQUOIA_CLOCK_CAPTURE") + (preset == 0 ? "-sequoia.png" : "-original.png")), "clock preset capture");
        eval(picker, "GridSession.place()");
        require(eval(grid, "placeAt(" + QString::number(3 + preset * 2) + ",2)").toBool(), "clock placed at chosen grid location");
        QTest::qWait(350);
        auto container = qvariant_cast<QQuickItem *>(eval(grid, "layout.children.find(c=>c.applet && c.applet.plasmoid.pluginName === 'org.sequoia.analogclock' && c.applet.plasmoid.configuration.faceStyle === '" + QString(preset == 0 ? "sequoia" : "original") + "')"));
        require(container && container->width() == 192 && container->height() == 192, "clock instance retains selected preset and 2x2 footprint");
        auto clock = named(container, "sequoiaAnalogClock");
        auto face = named(container, "sequoiaAnalogFace");
        require(clock && face && face->property("rounded").toBool() == (preset == 0), "placed face matches preview");
        require(eval(clock, "hours === clockSource.dateTime.getHours() && minutes === clockSource.dateTime.getMinutes()").toBool(), "clock uses KDE live time source");
        eval(clock, "Plasmoid.configuration.showSecondHand = true; Plasmoid.configuration.showTimezoneString = true");
        QTest::qWait(100);
        require(eval(clock, "showSecondsHand && showTimezone && clockSource.trackSeconds").toBool() && face->property("showSecondsHand").toBool(), "native seconds and timezone settings retained");
        eval(container, "configOverlayVisible = true; width = 270; height = 270");
        QTest::qWait(150);
        require(container->width() == 288 && container->height() == 288 && face->property("rounded").toBool() == (preset == 0), "resize snaps and preserves face style");
        eval(container, "width = 192; height = 192");
        QTest::qWait(100);
        eval(container, "configOverlayVisible = false");
        // Keep the picker alive while giving the real calendar keyboard focus.
        eval(picker, "sidePanel.hideOnWindowDeactivate = false");
        auto compact = qvariant_cast<QQuickItem *>(eval(clock, "compactRepresentationItem"));
        compact->window()->requestActivate();
        QTest::qWait(100);
        compact->forceActiveFocus();
        QTest::keyClick(compact->window(), Qt::Key_Space);
        QTest::qWait(250);
        fprintf(stderr, "CALENDAR: %s\n", qPrintable(eval(clock, "JSON.stringify({expanded:expanded,loaded:!!fullRepresentationItem,focus:compactRepresentationItem.activeFocus})").toString()));
        require(eval(clock, "expanded && fullRepresentationItem !== null").toBool(), "native calendar opens from keyboard");
        eval(clock, "expanded = false");
        eval(picker, "sidePanel.requestActivate()");
    }
    eval(picker, "GridSession.begin('org.sequoia.analogclock', 'Sequoia Analog Clock', 'clock', false); GridSession.choose(GridSession.targets.find(t=>t.kind === 'panel' && t.ready))");
    QTest::qWait(250);
    auto panel = qvariant_cast<QQuickItem *>(eval(picker, "GridSession.targets.find(t=>t.kind === 'panel' && t.ready).containment"));
    auto panelClock = named(panel, "sequoiaAnalogClock");
    require(panelClock && panelClock->property("panel").toBool(), "clock supports native panel form factor");
    auto panelFace = named(panelClock, "sequoiaAnalogFace");
    require(panelFace && panelFace->property("rounded").toBool(), "panel uses first clock preset");
    eval(picker, "sidePanel.hideOnWindowDeactivate = Qt.binding(()=>!main.preventWindowHide)");
    fprintf(stderr, "PASS: analog clock presets, live time, seconds/timezone, calendar, resize, panel and preview configuration\n");
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
    auto wizardWindow = qobject_cast<QQuickWindow *>(eval(picker, "placementDialog").value<QObject *>());
    require(wizardWindow && wizardWindow != window && wizardWindow->isVisible() && !window->isVisible(), "destination has its own window and picker is unmapped");
    const bool transparent = qEnvironmentVariable("SEQUOIA_PICKER_MATERIAL") == "transparent";
    fprintf(stderr, "MATERIAL: %s\n", qPrintable(eval(picker, "JSON.stringify({alpha:windowPalette.window.a,hints:placementDialog.backgroundHints})").toString()));
    require(eval(picker, transparent ? "placementDialog.backgroundHints === PlasmaCore.Dialog.StandardBackground" : "placementDialog.backgroundHints === PlasmaCore.Dialog.SolidBackground").toBool(), "wizard follows global theme material");
    require(eval(picker, "Math.abs(placementDialog.x - (sidePanel.x + (sidePanel.width-placementDialog.width)/2)) <= 1 && Math.abs(placementDialog.y - (sidePanel.y + (sidePanel.height-placementDialog.height)/2)) <= 1").toBool(), "wizard centered on picker monitor");
    const auto wizardImage = wizardWindow->grabWindow();
    const auto backgroundAlpha = wizardImage.pixelColor(QPoint(32,32) * wizardWindow->devicePixelRatio()).alpha();
    require(transparent ? backgroundAlpha > 0 && backgroundAlpha < 255 : backgroundAlpha == 255, "rendered frame uses transparent or opaque theme surface");
    require(wizardImage.save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE") + ".destinations.png"), "destination map screenshot");
    QTest::keyClick(wizardWindow, Qt::Key_Escape);
    QTest::qWait(200);
    require(window->isVisible() && !wizardWindow->isVisible() && eval(picker, "GridSession.phase === 'catalog' && searchInput.text === 'Sequoia Grid Test'").toBool(), "wizard Escape restores picker and search");
    eval(picker, "beginPlacement('org.sequoia.gridtest', 'Sequoia Grid Test', 'view-grid')");
    QTest::qWait(200);
    wizardWindow->close();
    QTest::qWait(200);
    require(window->isVisible() && !wizardWindow->isVisible() && eval(picker, "GridSession.phase === 'catalog'").toBool(), "window close cancels wizard and restores picker");
    eval(picker, "beginPlacement('org.sequoia.gridtest', 'Sequoia Grid Test', 'view-grid')");
    QTest::qWait(200);
    fprintf(stderr, "TARGETS: %s\n", qPrintable(eval(picker, "JSON.stringify(GridSession.targets.map(t=>({kind:t.kind,screen:t.screen,ready:t.ready,geometry:t.geometry})))").toString()));
    require(eval(picker, "GridSession.targets.some(t=>t.kind === 'desktop' && t.ready) && GridSession.targets.some(t=>t.kind === 'panel' && t.ready)").toBool(), "live desktop and panel destinations");
    eval(picker, "GridSession.choose(GridSession.targets.find(t=>t.kind === 'desktop' && t.screen === main.containment.screen))");
    require(eval(picker, "GridSession.phase === 'preset' && GridSession.preset.columns === 2 && GridSession.preset.rows === 2").toBool(), "first preset is square");
    eval(picker, "GridSession.presetIndex = 2");
    QTest::qWait(200);
    require(wizardWindow->isVisible() && !window->isVisible(), "preset window keeps picker hidden");
    require(wizardWindow->grabWindow().save(qEnvironmentVariable("SEQUOIA_PICKER_CAPTURE") + ".presets.png"), "preset screenshot");
    eval(picker, "GridSession.place()");
    QTest::qWait(200);
    require(!wizardWindow->isVisible() && !window->isVisible(), "both windows hidden during desktop placement");
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
    // Real movement arrives in small steps, not a single jump past half a cell.
    // Snapping x/y on every step used to pin the widget to its original cell.
    QPoint moveStart = container->mapToScene(QPointF(container->width()/2, 60)).toPoint();
    QTest::mousePress(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart);
    for (int dx = 8; dx <= 208; dx += 8) {
        QTest::mouseMove(desktopWindow, moveStart + QPoint(dx, 0), 20);
        QTest::qWait(20);
    }
    require(container->x() > 96, "slow edit-mode body drag moves an existing widget");
    QTest::mouseRelease(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart + QPoint(208, 0));
    QTest::qWait(400);
    require(container->x() == 192 && container->y() == 96 && container->width() == 192 && container->height() == 384,
            "existing widget snaps on release without changing its footprint");
    require(eval(placement, "layout.isRectAvailable(0,96,192,384) && !layout.isRectAvailable(192,96,192,384)").toBool(), "move releases old cells and occupies new cells");
    // The edit toolbar has a separate native MouseArea drag path.
    auto grip = moveHandle(container, container);
    require(grip && grip->isVisible(), "native edit toolbar move handle");
    moveStart = grip->mapToScene(QPointF(grip->width()/2, grip->height()/2)).toPoint();
    QTest::mousePress(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart);
    for (int dx = 8; dx <= 208; dx += 8) {
        QTest::mouseMove(desktopWindow, moveStart - QPoint(dx, 0), 20);
        QTest::qWait(20);
    }
    QTest::mouseRelease(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart - QPoint(208, 0));
    QTest::qWait(400);
    require(container->x() == 0 && container->y() == 96, "existing widget can move back into released cells");
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
    checkAnalogClock(picker, wizardWindow, placement);
    // Leave an existing instance at a new location and verify it after restart.
    eval(container, "editMode = true; configOverlayVisible = true");
    QTest::qWait(200);
    moveStart = container->mapToScene(QPointF(container->width()/2, 60)).toPoint();
    QTest::mousePress(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart);
    for (int dx = 8; dx <= 112; dx += 8) {
        QTest::mouseMove(desktopWindow, moveStart + QPoint(dx, 0), 20);
        QTest::qWait(20);
    }
    QTest::mouseRelease(desktopWindow, Qt::LeftButton, Qt::NoModifier, moveStart + QPoint(112, 0));
    QTest::qWait(400);
    require(container->x() == 96 && container->y() == 96, "existing widget moved to its saved destination");
    eval(container, "configOverlayVisible = false; editMode = false");
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
    fprintf(stderr, "PASS: separate themed wizard, material alpha, picker visibility, wizard Escape/close, picker search, destination map, desktop/panel placement, presets, occupied cells, native live resize, adaptive layout, drag/drop/cancel, Escape, activity sidebar\n");
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
                            require(eval(grid, "layout.children.some(c=>c.applet && c.x === 96 && c.y === 96 && c.width === 192 && c.height === 384)").toBool(), "resized and repositioned geometry restored after shell restart");
                            require(eval(grid, "layout.children.some(c=>c.applet && c.x === 480 && c.y === 0 && c.width === 192 && c.height === 192)").toBool(), "dragged geometry restored after shell restart");
                            require(eval(grid, "['sequoia','original'].every(style=>layout.children.some(c=>c.applet && c.applet.plasmoid.pluginName === 'org.sequoia.analogclock' && c.applet.plasmoid.configuration.faceStyle === style && c.applet.plasmoid.configuration.showSecondHand && c.applet.plasmoid.configuration.showTimezoneString))").toBool(), "both clock presets and compatible settings persist after restart");
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
