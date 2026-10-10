#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <cstdlib>
static QList<QQuickItem*> items(QQuickItem *root) { QList<QQuickItem*> result{root}; for(int i=0;i<result.size();++i) result.append(result[i]->childItems()); return result; }
static void start() {
 if (qEnvironmentVariableIsEmpty("SEQUOIA_DOCK_TEST")) return;
    if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_RTL")) qApp->setLayoutDirection(Qt::RightToLeft);
    qApp->setFont(QFont("Adwaita Sans", qEnvironmentVariableIsSet("SEQUOIA_VERIFY_LARGE") ? 13 : 10));
    // The generic platform theme doesn't read KDE's application palette.
    // Match it to the Plasma palette used by this isolated preview.
    const bool dark = qEnvironmentVariableIsSet("SEQUOIA_VERIFY_DARK");
    QPalette palette;
    for (const auto role : {QPalette::Window, QPalette::Base, QPalette::Button})
        palette.setColor(role, QColor(dark ? "#252629" : "#f2f3f5"));
    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        palette.setColor(role, QColor(dark ? "#f2f3f5" : "#252629"));
        palette.setColor(QPalette::Disabled, role, QColor(dark ? "#adb0b8" : "#686b73"));
    }
    palette.setColor(QPalette::Highlight, QColor(dark ? "#8db3ff" : "#2563db"));
    palette.setColor(QPalette::HighlightedText, QColor(dark ? "#161a22" : "#ffffff"));
    qApp->setPalette(palette);
 QTimer::singleShot(1500,qApp,[]{for(auto w:QGuiApplication::allWindows()) if(auto q=qobject_cast<QQuickWindow*>(w)) q->resize(640,68);});
 QTimer::singleShot(4000,qApp,[]{
  for(auto w:QGuiApplication::allWindows()) if(auto q=qobject_cast<QQuickWindow*>(w)) {
   QList<QQuickItem*> tasks;
   for(auto item:items(q->contentItem())) if(item->objectName()=="sequoiaDockTask") tasks<<item;
   if(tasks.size()<2) continue;
   auto task=tasks.first();
   QQmlExpression expr(qmlContext(task),task,"({dot: children.filter(c => c.objectName === 'sequoiaRunningDot').length, popup: inPopup, icon: tasksRoot.iconsOnly})");
   auto state=expr.evaluate().value<QJSValue>();
   if(expr.hasError() || state.property("dot").toInt()!=1 || !state.property("icon").toBool()) std::_Exit(3);
   QTest::mouseMove(q,task->mapToScene(QPointF(task->width()/2,task->height()/2)).toPoint());
   QTest::qWait(500);
   QQmlExpression zoom(qmlContext(task),task,"children.some(c => c.scale > 1.09)");
   if(!zoom.evaluate().toBool()) {qWarning("FAIL: dock hover zoom");std::_Exit(4);}
   q->grabWindow().save(qEnvironmentVariable("SEQUOIA_DOCK_TEST"));
   QTest::mouseMove(q,QPoint(task->mapToScene(QPointF(task->width()/2,0)).x() < q->width()/2 ? q->width()-1 : 0,0));QTest::qWait(500);
   if(zoom.evaluate().toBool()) {qWarning("FAIL: hover did not reset");std::_Exit(5);}
   auto eval = [task](const QString &code) {
    QQmlExpression expression(qmlContext(task), task, code);
    auto result = expression.evaluate();
    if (expression.hasError()) { qWarning() << expression.error(); std::_Exit(6); }
    return result;
   };
   if (eval("Plasmoid.configuration.dockStrength").toInt() != 12) { qWarning("FAIL: extended native schema"); std::_Exit(7); }
   eval("Plasmoid.configuration.dockAnimations = false");
   QTest::mouseMove(q,task->mapToScene(QPointF(task->width()/2,task->height()/2)).toPoint());
   QTest::qWait(300);
   if (zoom.evaluate().toBool()) {qWarning("FAIL: motion toggle"); std::_Exit(8);}
   eval("Plasmoid.configuration.dockAnimations = true; Plasmoid.configuration.dockHoverStyle = 1");
   QTest::qWait(300);
   if (zoom.evaluate().toBool()) {qWarning("FAIL: lift-only style still zooms"); std::_Exit(9);}
   eval("Plasmoid.configuration.dockHoverStyle = 0; Plasmoid.configuration.dockStrength = 18");
   QTest::qWait(400);
   if (!eval("children.some(c => c.objectName === 'sequoiaDockIcon' && c.scale > 1.17)").toBool()) {qWarning("FAIL: custom intensity"); std::_Exit(10);}
   eval("Plasmoid.internalAction('configure').trigger()");
   QTest::qWait(1200);
   bool foundSettings = false;
   for (auto window : QGuiApplication::allWindows()) if (auto settings = qobject_cast<QQuickWindow*>(window)) {
    for (auto item : items(settings->contentItem())) if (item->objectName() == "sequoiaAnimationSettings") {
     foundSettings = true;
     if (item->property("cfg_dockStrength").toInt() != 18) {qWarning("FAIL: settings load saved configuration"); std::_Exit(11);}
     settings->resize(900, 950);
     QTest::qWait(400);
     settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_DOCK_TEST") + ".settings.png");
     // Exercise Plasma's real Apply path and verify it reaches the live delegate.
     item->setProperty("cfg_dockStrength", 16);
     bool applied = false;
     for (auto control : items(settings->contentItem())) if (control->property("text").toString().remove('&') == "Apply") {
      QMetaObject::invokeMethod(control, "clicked"); applied = true; break;
     }
     QTest::qWait(300);
     if (!applied || eval("Plasmoid.configuration.dockStrength").toInt() != 16) {qWarning("FAIL: native Apply"); std::_Exit(12);}
    }
   }
   for (const auto &page : {QString("Appearance"), QString("Behavior")}) {
    bool loaded = false;
    for (auto window : QGuiApplication::allWindows()) if (auto settings = qobject_cast<QQuickWindow*>(window)) {
     QQuickItem *entry = nullptr;
     for (auto control : items(settings->contentItem())) if (control->isVisible() && control->property("text").toString() == page) {entry = control; break;}
     if (!entry) continue;
     QTest::mouseClick(settings, Qt::LeftButton, Qt::NoModifier, entry->mapToScene(QPointF(entry->width()/2,entry->height()/2)).toPoint());
     QTest::qWait(700);
     for (auto item : items(settings->contentItem())) if (item->objectName() == "sequoiaConfig" + page) loaded = true;
     settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_DOCK_TEST") + "." + page + ".png");
    }
    if (!loaded) {qWarning() << "FAIL: settings page" << page; std::_Exit(14);}
   }
   if (!foundSettings) {qWarning("FAIL: native animation settings missing"); std::_Exit(13);}
   qWarning("PASS: native task delegates, running-dot objects, hover zoom and reset; native schema, animation toggle, styles, intensity, Apply and all settings pages");std::_Exit(0);
  }
  for (auto window : QGuiApplication::allWindows()) if (auto quick = qobject_cast<QQuickWindow *>(window)) {
   for (auto item : items(quick->contentItem())) if (item->metaObject()->indexOfProperty("iconsOnly") >= 0) {
    QQmlExpression diagnostic(qmlContext(item), item, "JSON.stringify({plugin: Plasmoid.pluginName, iconsOnly: iconsOnly, launchers: Plasmoid.configuration.launchers, modelLaunchers: tasksModel.launcherList, count: tasksModel.count})");
    qWarning() << "Task fixture:" << diagnostic.evaluate() << diagnostic.error();
   }
  }
  qWarning("FAIL: native dock tasks missing");std::_Exit(2);
 });
}
Q_COREAPP_STARTUP_FUNCTION(start)
