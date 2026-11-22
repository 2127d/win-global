// SPDX-License-Identifier: MIT
// Exercise automatic sizing and native handles in an isolated Plasma session.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <QSignalSpy>
#include <QFileInfo>
#include <QDBusInterface>
#include <QDBusReply>
#include <cstdlib>
static void require(bool ok,const char *s){fprintf(stderr,"%s: %s\n",ok?"PASS":"FAIL",s);if(!ok)std::_Exit(2);}
static QVariant eval(QObject *o,const QString &s){QQmlExpression e(qmlContext(o),o,s);auto v=e.evaluate();if(e.hasError()){fprintf(stderr,"%s\n",qPrintable(e.error().toString()));std::_Exit(3);}return v;}
static QQuickItem *named(QQuickItem *o,const QString &n){if(o->objectName()==n)return o;for(auto c:o->childItems())if(auto v=named(c,n))return v;return nullptr;}
static QQuickItem *handle(QQuickItem *o){if(o->property("resizeCorner").toInt()==5)return o;for(auto c:o->childItems())if(auto v=handle(c))return v;return nullptr;}
static void check(QQuickItem *grid){
 QDBusInterface activities("org.kde.ActivityManager", "/ActivityManager/Activities", "org.kde.ActivityManager.Activities");
 const QString original = QDBusReply<QString>(activities.call("CurrentActivity")).value();
 const QString second = QDBusReply<QString>(activities.call("AddActivity", "Gaming")).value();
 require(!second.isEmpty(), "created activity in private test session");
 QTest::qWait(400);
 bool panelFound=false;
 for(auto w:QGuiApplication::allWindows()) if(auto q=qobject_cast<QQuickWindow*>(w)) if(auto p=named(q->contentItem(),"sequoiaActivityBar")) {
  if(p->property("panel").toBool()) {panelFound=true;require(eval(p,"Layout.maximumWidth === Infinity && Layout.maximumHeight === Infinity && !sequoiaAutomaticSize").toBool(),"panel retains native unconstrained sizing");require(!named(p,"sequoiaActivityPill")->isVisible(),"desktop pill is hidden in panel");}
 }
 require(panelFound,"panel instance loaded");
 eval(grid,"GridSession.addApplet(containment, 'org.sequoia.activitybar')");QTest::qWait(700);
 auto c=qvariant_cast<QQuickItem*>(eval(grid,"layout.children.find(c=>c.applet && c.applet.plasmoid.pluginName==='org.sequoia.activitybar')"));require(c,"desktop applet created");
 auto root=named(c,"sequoiaActivityBar");auto pill=named(c,"sequoiaActivityPill");require(root&&pill,"desktop QML loaded");
 auto actualTabs=named(pill,"activityPillTabs");QQuickItem *secondTab=nullptr;
 for(auto item:actualTabs->childItems()) if(item->property("id").toString()==second)secondTab=item;
 require(secondTab,"real activity label appears");
 QTest::mouseClick(c->window(),Qt::LeftButton,Qt::NoModifier,secondTab->mapToScene(QPointF(secondTab->width()/2,secondTab->height()/2)).toPoint());QTest::qWait(400);
 require(QDBusReply<QString>(activities.call("CurrentActivity")).value()==second,"click switches the real activity backend");
 activities.call("SetCurrentActivity",original);QTest::qWait(400);
 c->window()->requestActivate();secondTab->forceActiveFocus();QTest::qWait(100);QTest::keyClick(c->window(),Qt::Key_Space);QTest::qWait(400);
 require(QDBusReply<QString>(activities.call("CurrentActivity")).value()==second,"Space switches the real activity backend");
 activities.call("SetCurrentActivity",original);QTest::qWait(400);

 eval(c,"layout.releaseSpace(this); x=96; y=96; layout.positionItem(this)");
 eval(pill,"model=Qt.createQmlObject('import QtQml.Models; ListModel {}',this); model.append([{name:'Default',id:'default',current:true},{name:'Gaming',id:'gaming',current:false}])");QTest::qWait(400);
 fprintf(stderr,"GEOMETRY %f %f %s\n",c->width(),c->height(),qPrintable(eval(root,"JSON.stringify({minW:Layout.minimumWidth,maxW:Layout.maximumWidth,minH:Layout.minimumHeight,maxH:Layout.maximumHeight})").toString()));
 require(c->width()==192 && c->height()==96,"two activities fit a 2x1 footprint");

 eval(pill,"model.append({name:'Creative Work',id:'work',current:false})");QTest::qWait(400);
 require(c->width()==288 && c->height()==96,"adding an activity grows by one column");
 
 require(eval(c,"!layout.isRectAvailable(x+width-96,y,96,96)").toBool(),"growth reserves the new grid cell");
 eval(pill,"model.setProperty(2,'name','An activity with a much longer name')");QTest::qWait(400);
 require(c->width()>288 && int(c->width())%96==0,"renaming grows on grid boundaries");
 auto tabs=named(pill,"activityPillTabs");
 require(qAbs(tabs->x()*2+tabs->width()-pill->width())<1,"long activity names remain a centered group");
 c->window()->grabWindow().copy(c->mapRectToScene(c->boundingRect()).toAlignedRect()).save(qEnvironmentVariable("ACTIVITY_CAPTURE")+"-long.png");
 eval(pill,"model.remove(2)");QTest::qWait(400);
 require(c->width()==192 && c->height()==96,"removing an activity shrinks automatically");
 require(eval(c,"layout.isRectAvailable(x+width,y,96,96)").toBool(),"shrink releases its former grid cell");
 eval(c,"editMode=true; configOverlayVisible=true");QTest::qWait(400);
 auto overlay=c->property("configOverlayItem").value<QQuickItem*>();
 require(overlay,"native edit overlay exists");
 int tested=0;auto win=c->window();
 const QPoint outward[]={{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1}};
 for(auto h:overlay->childItems()) {
  if(!h->property("resizeCorner").isValid())continue;
  auto blocker=named(h,"sequoiaFixedSizeHandleGuard");require(blocker,"native handle has a gesture guard");
  for(int direction:{1,-1}) {
   const auto point=h->mapToScene(QPointF(h->width()/2,h->height()/2)).toPoint();
   QSignalSpy widths(c,&QQuickItem::widthChanged),heights(c,&QQuickItem::heightChanged);
   QSignalSpy xs(c,&QQuickItem::xChanged),ys(c,&QQuickItem::yChanged);
   QTest::mousePress(win,Qt::LeftButton,Qt::NoModifier,point);
   for(int distance:{1,8,32,96}) {
    QTest::mouseMove(win,point+outward[h->property("resizeCorner").toInt()]*distance*direction,10);
    QTest::qWait(10);
    require(widths.isEmpty()&&heights.isEmpty()&&xs.isEmpty()&&ys.isEmpty(),"drag emits zero geometry changes, including transient changes");
    require(blocker->property("pressed").toBool(),"handle consumes the drag before native resizing");
    require(eval(blocker,"surface !== null && surface.color == blockedColor").toBool(),"existing Plasma handle surface is red");
   }
   if(tested==0)win->grabWindow().save(qEnvironmentVariable("ACTIVITY_CAPTURE")+"-handles.png");
   QTest::mouseRelease(win,Qt::LeftButton,Qt::NoModifier,point+outward[h->property("resizeCorner").toInt()]*96*direction);
   QTest::qWait(20);
   require(widths.isEmpty()&&heights.isEmpty()&&xs.isEmpty()&&ys.isEmpty(),"release leaves geometry untouched");
  }
  ++tested;
 }
 require(tested==8,"all eight handles hard-stop inward and outward drags");
 eval(c,"editMode=false; configOverlayVisible=false");QTest::qWait(300);
 win->grabWindow().copy(c->mapRectToScene(c->boundingRect()).toAlignedRect()).save(qEnvironmentVariable("ACTIVITY_CAPTURE")+"-pill.png");
 eval(pill,"model.clear()");QTest::qWait(200);require(c->width()==192&&c->height()==96,"empty model has stable grid footprint");
 eval(pill,"model.append({name:'Default',id:'default',current:true})");
 fprintf(stderr,"PASS: activity bar checks complete\n");qApp->quit();
}
static void start(){auto timer=new QTimer(qApp);QObject::connect(timer,&QTimer::timeout,[timer]{if(!QFileInfo::exists(qEnvironmentVariable("SEQUOIA_PICKER_READY")))return;for(auto w:QGuiApplication::allWindows())if(auto q=qobject_cast<QQuickWindow*>(w))if(auto g=named(q->contentItem(),"sequoiaDesktopGrid")){timer->stop();check(g);return;}});timer->start(200);QTimer::singleShot(40000,[]{require(false,"timeout");});}
Q_COREAPP_STARTUP_FUNCTION(start)
