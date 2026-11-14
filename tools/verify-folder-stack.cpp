// SPDX-License-Identifier: MIT
#include <QBitArray>
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <cstdlib>
static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for(int i=0;i<result.size();++i) result.append(result[i]->childItems());
    return result;
}
static void require(bool ok, const char *message) { if(!ok) {fprintf(stderr,"FAIL: %s\n",message);std::_Exit(2);} }
static QVariant eval(QQuickItem *root,const QString &s) {
    QQmlExpression e(qmlContext(root),root,s); auto v=e.evaluate();
    require(!e.hasError(),qPrintable(e.error().toString()));return v;
}
static void start() {
 QTimer::singleShot(1500,qApp,[]{
    QQuickItem *root=nullptr,*button=nullptr;
    for(auto w:QGuiApplication::allWindows()) if(auto v=qobject_cast<QQuickWindow*>(w))
      for(auto i:items(v->contentItem())) if(i->objectName()=="sequoiaFolderStack") root=i;
    require(root,"widget failed to load");
    for(auto i:items(root)) if(i->objectName()=="folderStackButton") button=i;
    require(button,"dock button missing");
    eval(root,"Plasmoid.configuration.folder = " + QString("'%1'").arg(qEnvironmentVariable("STACK_FOLDER")));
    eval(root,"Plasmoid.configuration.showPreviews = false; Plasmoid.configuration.markNew = false;");
    QTest::qWait(400);
    auto click=[&]{QTest::mouseClick(button->window(),Qt::LeftButton,Qt::NoModifier,button->mapToScene(QPointF(button->width()/2,button->height()/2)).toPoint());};
    click();
    int duration=root->property("openDuration").toInt();
    QTest::qWait(duration/2);
    double progress=root->property("progress").toDouble();
    require(progress>0 && progress<1,"opening skipped intermediate frames");
    QQuickWindow *popup=nullptr; QQuickItem *row=nullptr;
    for(auto w:QGuiApplication::allWindows()) if(w->objectName()=="folderStackWindow") popup=qobject_cast<QQuickWindow*>(w);
    require(popup && popup->isVisible(),"popup missing");
    for(auto i:items(popup->contentItem())) if(i->objectName()=="folderStackRow0_2") row=i;
    require(row,"fixture files missing");
    auto midY=row->y();
    require(row->property("unfolding").toDouble()<1,"row skipped stagger");
    QTest::qWait(duration+80);
    require(row->y()<midY && row->property("unfolding").toDouble()==1,"fan did not travel up from dock");
    const auto openY=row->y();
    popup->grabWindow().save(qEnvironmentVariable("STACK_CAPTURE"));
    QTest::keyClick(popup,Qt::Key_Escape);
    int closing=root->property("closeDuration").toInt();
    QTest::qWait(closing/2);
    require(popup->isVisible() && root->property("progress").toDouble()>0 && root->property("progress").toDouble()<1 && row->y()>openY,"fold hid popup too early or did not travel back");
    QTest::qWait(closing+80);
    require(!root->property("popupOpen").toBool(),"Escape failed to finish closing");
    QTest::qWait(260); click();QTest::qWait(100);click();QTest::qWait(closing+100);
    require(!root->property("popupOpen").toBool(),"rapid second click reopened popup");
    QTest::qWait(260);
    eval(root,"Plasmoid.configuration.viewMode=1");click();QTest::qWait(duration+100);
    require(root->property("showGrid").toBool() && root->property("popupOpen").toBool(),"grid mode broken");
    eval(root,"closeFan()");QTest::qWait(closing+100);
    require(!root->property("popupOpen").toBool(),"grid close failed");
    fprintf(stderr,"PASS: real dock button, staggered travel/growth, fold-before-hide, Escape, rapid toggle, grid settings\n");
    std::_Exit(0);
 });
}
Q_COREAPP_STARTUP_FUNCTION(start)
