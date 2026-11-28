// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only hook, loaded only into the isolated verification instance.
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTest>
#include <QQmlExpression>
#include <QQmlEngine>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QQuickWidget>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryFile>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QDBusArgument>
#include <cstdio>
static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (qsizetype i=0;i<result.size();++i) result.append(result[i]->childItems());
    return result;
}
static QQuickWindow *window() {
    for (auto w: QGuiApplication::allWindows()) if (auto q=qobject_cast<QQuickWindow *>(w); q && q->isVisible() && q->width()>400) return q;
    return nullptr;
}
static QQuickItem *find(QQuickWindow *w, const QString &name) {
    for (auto i:items(w->contentItem())) if(i->objectName()==name && i->isVisible()) return i;
    return nullptr;
}
static bool click(QQuickWindow *w,const QString &name) {
    auto i=find(w,name); if(!i) return false;
    QTest::mouseClick(w,Qt::LeftButton,Qt::NoModifier,i->mapToScene(QPointF(i->width()/2,i->height()/2)).toPoint());
    return true;
}
static void fail(const QString &message) { qCritical("MAKEOVER FAIL: %s", qPrintable(message)); QCoreApplication::exit(7); }
static bool captureAccountComposition(const QString &name) {
    QTemporaryFile file;if(!file.open()) return false;
    auto request=QDBusMessage::createMethodCall("org.kde.KWin.ScreenShot2","/org/kde/KWin/ScreenShot2","org.kde.KWin.ScreenShot2","CaptureWorkspace");
    request<<QVariantMap{{"native-resolution",false},{"hide-caller-windows",false}}<<QVariant::fromValue(QDBusUnixFileDescriptor(file.handle()));
    auto reply=QDBusConnection::sessionBus().call(request,QDBus::Block,5000);
    if(reply.type()!=QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) return false;
    const auto info=qdbus_cast<QVariantMap>(reply.arguments().first());
    const auto bytes=info["height"].toInt()*info["stride"].toInt();
    if(!QTest::qWaitFor([&]{return file.size()>=bytes;},2000)) return false;
    file.seek(0);const auto data=file.readAll();
    if(bytes<=0 || data.size()!=bytes) return false;
    return QImage(reinterpret_cast<const uchar*>(data.constData()),info["width"].toInt(),info["height"].toInt(),info["stride"].toInt(),QImage::Format(info["format"].toInt())).save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/"+name+".png");
}
static bool activateAccountAction(QDialog *editor, QAbstractButton *native, bool keyboard=false) {
    for(auto action:editor->findChildren<QQuickWidget*>("sequoiaNativeAccountActionHost"))
        if(action->property("nativeButton").value<QAbstractButton*>()==native) {
            if(keyboard) {action->setFocus();QTest::keyClick(action,Qt::Key_Space);}
            else QTest::mouseClick(action,Qt::LeftButton,Qt::NoModifier,QPoint(action->width()/2,action->height()/2));
            return true;
        }
    return false;
}
static QObject *scheduleMemory(QQuickWindow *w,const QString &key) {
    for(auto item:items(w->contentItem())) for(auto object:item->children())
        if(object->objectName()=="sequoiaScheduleScrollMemory" && object->property("active").toBool() && object->property("positionKey").toString()==key) return object;
    return nullptr;
}
static QString scrollAnchor(QQuickItem *list) {
    QQmlExpression anchor(qmlContext(list),list,"(function(){ const i=indexAt(width/2,contentY+1); const row=itemAtIndex(i); return [i,row ? contentY-row.y : -1].join(','); })()");
    return anchor.evaluate().toString();
}
static bool sameAnchor(const QString &actual,const QString &expected) {
    const auto a=actual.split(','),b=expected.split(',');
    return a.size()==2 && b.size()==2 && a[0]==b[0] && qAbs(a[1].toDouble()-b[1].toDouble())<3;
}
static bool scheduleScrollCheck(QQuickWindow *w,bool restarted) {
    w->resize(2200,1000);QTest::qWait(300);
    auto weather=find(w,"sequoiaScheduleWeather");
    const auto month=weather->property("month").toDateTime().date();
    QSettings expected(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/verify-scroll.ini",QSettings::IniFormat);
    auto weatherFixture=[&]() {
        weather=find(w,"sequoiaScheduleWeather");
        QQmlExpression fill(qmlContext(weather),weather,R"JS((function(){
            const start=monthStart;
            eventDays=Array.from({length:20},(_,i)=>{const date=new Date(start.getFullYear(),start.getMonth(),i+1);return {date:date,key:Qt.formatDate(date,"yyyy-MM-dd"),titles:["Planned event"]};});
        })())JS");
        fill.evaluate();QTest::qWait(300);
        return !fill.hasError();
    };
    if(!restarted) {
        // Recreate native month models after the earlier presentation fixtures.
        click(w,"sequoiaCalendarNav-open_month_view");QTest::qWait(350);
        click(w,"sequoiaCalendarNav-open_schedule_view");QTest::qWait(850);
        weather=find(w,"sequoiaScheduleWeather");
        for(int i=0;i<4;++i) {
            auto memory=scheduleMemory(w,"month/"+month.addMonths(i).toString("yyyy-MM"));
            auto list=memory ? qobject_cast<QQuickItem*>(memory->property("view").value<QObject*>()) : nullptr;
            if(!list) {fail("Schedule month memory missing");return false;}
            list->setProperty("contentY",list->property("originY").toReal()+150+i*95);
        }
        if(!click(w,"sequoiaScheduleWeatherNext") || !weatherFixture()) {fail("Weather scroll fixture failed");return false;}
        auto weatherList=find(w,"sequoiaScheduleForecastDays");
        weatherList->setProperty("contentY",weatherList->property("originY").toReal()+420);
        QTest::qWait(200);
        for(int i=0;i<4;++i) {
            auto memory=scheduleMemory(w,"month/"+month.addMonths(i).toString("yyyy-MM"));
            auto list=qobject_cast<QQuickItem*>(memory->property("view").value<QObject*>());
            expected.setValue(QString("month%1").arg(i),scrollAnchor(list));
        }
        expected.setValue("weather",scrollAnchor(weatherList));expected.sync();
        click(w,"sequoiaCalendarNav-open_month_view");QTest::qWait(350);
        click(w,"sequoiaCalendarNav-open_schedule_view");QTest::qWait(850);
    }
    weather=find(w,"sequoiaScheduleWeather");
    if(weather->property("monthOffset").toInt()!=1) {fail("Weather did not remember its selected month");return false;}
    if(!weatherFixture()) {fail("Weather restore fixture failed");return false;}
    for(int i=0;i<4;++i) {
        auto memory=scheduleMemory(w,"month/"+month.addMonths(i).toString("yyyy-MM"));
        auto list=memory ? qobject_cast<QQuickItem*>(memory->property("view").value<QObject*>()) : nullptr;
        const auto saved=expected.value(QString("month%1").arg(i)).toString();
        if(!list || !sameAnchor(scrollAnchor(list),saved)) {fail("Schedule month scroll was not restored: "+QString::number(i)+" expected="+saved+" actual="+(list?scrollAnchor(list):"missing"));return false;}
    }
    auto weatherList=find(w,"sequoiaScheduleForecastDays");
    if(!sameAnchor(scrollAnchor(weatherList),expected.value("weather").toString())) {fail("Weather scroll was not restored: expected="+expected.value("weather").toString()+" actual="+scrollAnchor(weatherList));return false;}
    qInfo("MAKEOVER: Schedule months and weather retain independent scroll positions%s",restarted?" after restart":" across view changes");
    return true;
}
static void step(int phase) {
    auto w=window(); if(!w) { fail("No application window"); return; }
    const QStringList pages{"month","today","week","schedule","tasks","compact"};
    const QStringList actions{"today","open_week_view","open_schedule_view","open_todo_view","open_month_view"};
    auto card=find(w,"sequoiaMerkuroCalendar");
    if (!card) {fail("Missing calendar floating card at phase "+QString::number(phase));return;}
    auto mapped=card->mapRectToScene(card->boundingRect());
    if(mapped.left()<12 || mapped.right()>w->width()-7 || mapped.bottom()>w->height()-7) {fail("Card touches application edge");return;}
    if(phase==0 && !find(w,"sequoiaAgendaCard")) {fail("Month agenda missing");return;}
    if(phase==1 && (!find(w,"sequoiaUpNextCard") || !find(w,"sequoiaComingUpCard") || !find(w,"sequoiaCalendarTasksCard") || !find(w,"sequoiaMiniCalendar"))) {fail("Today dashboard cards missing");return;}
    if(phase==1) {
        auto bounds=[w](const QString &name){auto i=find(w,name);return i->mapRectToScene(i->boundingRect());};
        const auto next=bounds("sequoiaUpNextCard"), tasks=bounds("sequoiaCalendarTasksCard"), mini=bounds("sequoiaMiniCalendar"), upcoming=bounds("sequoiaComingUpCard");
        if(qAbs(mapped.top()-next.top())>1 || qAbs(next.top()-mini.top())>1 || qAbs(mapped.bottom()-tasks.bottom())>1 || qAbs(tasks.bottom()-upcoming.bottom())>1 || next.right()>=mini.left() || next.bottom()>=tasks.top()) {fail("Today card arrangement does not match concept");return;}
    }
    if(phase==0) {
        auto chevron=find(w,"sequoiaCalendarChevron-collectionHeadingItem");
        if(!chevron) {fail("Calendar glass chevron missing");return;}
        const bool expanded=chevron->property("expanded").toBool();
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");QTest::qWait(50);
        if(chevron->property("expanded").toBool()==expanded) {fail("Calendar chevron did not toggle exactly once");return;}
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");QTest::qWait(50);
        if(chevron->property("expanded").toBool()!=expanded) {fail("Calendar chevron did not reopen");return;}
        // Private visual rows exercise motion without creating user calendars.
        QQmlExpression motionRows(qmlContext(chevron),chevron,R"JS((function() {
            Config.collectionsSectionExpanded = true;
            return Qt.createQmlObject('import QtQuick; import QtQuick.Layouts; Item { implicitHeight:120; Layout.fillWidth:true }',calendarRows);
        })())JS");
        auto motionFixture=motionRows.evaluate().value<QObject*>();
        if(!motionFixture || motionRows.hasError()) {fail("Calendar motion fixture failed");return;}
        QTest::qWait(300);
        auto section=find(w,"sequoiaCalendarSection");
        if(!section || section->height()<120) {fail("Expanded calendar section has no height");return;}
        const qreal fullHeight=section->height();
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");QTest::qWait(80);
        const qreal closingHeight=section->height();
        if(closingHeight<=0 || closingHeight>=fullHeight || section->isEnabled()) {fail("Calendar collapse did not animate or still accepts input");return;}
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");
        if(qAbs(section->height()-closingHeight)>1) {fail("Calendar reversal jumped");return;}
        QTest::qWait(300);
        if(qAbs(section->height()-fullHeight)>1 || !section->isEnabled()) {fail("Calendar reversal did not finish expanded");return;}
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");QTest::qWait(300);
        if(section->isVisible() || section->height()>1) {fail("Calendar collapse left visible rows or space");return;}
        click(w,"sequoiaCalendarChevron-collectionHeadingItem");QTest::qWait(80);
        if(section->height()<=0 || section->height()>=fullHeight) {fail("Calendar expansion did not animate");return;}
        QTest::qWait(300);
        delete motionFixture;
        if(!expanded) click(w,"sequoiaCalendarChevron-collectionHeadingItem");
        QTest::qWait(300);
        qInfo("MAKEOVER: calendar section smoothly closes, opens, and reverses");
        // A private native tree exercises real descendant removal/insertion,
        // nested sources, rapid reversal, and independent account state.
        auto tree=new QStandardItemModel(w);
        tree->setItemRoleNames({{Qt::DisplayRole,"display"},{Qt::DecorationRole,"decoration"},
            {Qt::CheckStateRole,"checkState"},{Qt::UserRole+1,"collectionId"},
            {Qt::UserRole+2,"collection"},{Qt::UserRole+3,"collectionColor"}});
        QQmlExpression emptyCollection(qmlContext(chevron),chevron,
            "Qt.createQmlObject('import QtQuick; import org.kde.akonadi as Akonadi; QtObject { property Akonadi.collection emptyCollection }',root).emptyCollection");
        const QVariant collection=emptyCollection.evaluate();
        int collectionId=0;
        auto entry=[&](const QString &name) {
            auto item=new QStandardItem(name);
            item->setData("view-calendar",Qt::DecorationRole);item->setData(2,Qt::CheckStateRole);
            item->setData(++collectionId,Qt::UserRole+1);item->setData(collection,Qt::UserRole+2);
            item->setData(QColor("#9162e9"),Qt::UserRole+3);return item;
        };
        auto firstAccount=entry("First account"), secondAccount=entry("Second account"), nested=entry("Shared calendars");
        firstAccount->appendRow(entry("Personal"));nested->appendRow(entry("Team"));firstAccount->appendRow(nested);
        secondAccount->appendRow(entry("Birthdays"));tree->appendRow(firstAccount);tree->appendRow(secondAccount);
        qmlEngine(chevron)->rootContext()->setContextProperty("calendarMotionTestTree",tree);
        auto expression=[&](const QString &code) {
            QQmlExpression expr(qmlContext(chevron),chevron,code);
            const auto value=expr.evaluate();
            if(expr.hasError()) qCritical("MAKEOVER FAIL: %s",qPrintable(expr.error().toString()));
            return value;
        };
        auto proxy=expression("(function(){ Config.collectionsSectionExpanded=true; const proxy=Qt.createQmlObject('import QtQuick; import org.kde.kitemmodels; KDescendantsProxyModel { expandsByDefault:true; model:calendarMotionTestTree }',root); collectionList.model=proxy; return proxy; })()").value<QObject*>();
        auto row=[&](int i) {return qobject_cast<QQuickItem*>(expression(QString("collectionList.itemAt(%1)").arg(i)).value<QObject*>());};
        auto count=[&]() {return expression("collectionList.count").toInt();};
        auto toggleRow=[&](QQuickItem *item) {
            for(auto child:items(item)) if(child->objectName()=="sequoiaCalendarChevron-collectionSourceItem") {
                QTest::mouseClick(w,Qt::LeftButton,Qt::NoModifier,child->mapToScene(QPointF(child->width()/2,child->height()/2)).toPoint());return;
            }
        };
        QTest::qWait(350);
        if(!proxy || count()!=6 || !row(1)) {fail("Native account tree fixture missing rows");return;}
        const qreal childHeight=row(1)->height(), otherHeight=row(5)->height();
        toggleRow(row(0));QTest::qWait(80);
        if(count()!=6 || row(1)->height()<=0 || row(1)->height()>=childHeight || row(1)->isEnabled() || qAbs(row(5)->height()-otherHeight)>1) {fail("Account close did not animate independently");return;}
        const qreal midway=row(1)->height();
        toggleRow(row(0));
        if(qAbs(row(1)->height()-midway)>1) {fail("Account reversal jumped");return;}
        QTest::qWait(300);
        if(count()!=6 || qAbs(row(1)->height()-childHeight)>1) {fail("Account reversal lost rows");return;}
        toggleRow(row(2));QTest::qWait(300);
        if(count()!=5) {fail("Nested account did not collapse");return;}
        toggleRow(row(0));QTest::qWait(300);
        if(count()!=3) {fail("Native account children not removed after closing");return;}
        toggleRow(row(0));QTest::qWait(80);
        if(count()!=5 || row(1)->height()<=0 || row(1)->height()>=childHeight) {fail("Account expansion did not animate or lost nested state");return;}
        QTest::qWait(300);
        toggleRow(row(2));QTest::qWait(300);
        if(count()!=6 || firstAccount->child(0)->data(Qt::CheckStateRole).toInt()!=2) {fail("Account expansion changed calendar selection");return;}
        w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/calendar-account-motion.png");
        expression("collectionList.model=collectionList.collectionModel");
        delete proxy;delete tree;
        qInfo("MAKEOVER: individual and nested accounts smoothly close, open, and reverse without changing checks");
        // Instantiate the real account delegate with a non-checkable source,
        // as used by Google account headings. It must keep the leading space.
        QQmlExpression account(qmlContext(chevron),chevron,R"JS((function() {
            const holder=Qt.createQmlObject('import QtQuick; import org.kde.akonadi as Akonadi; QtObject { property Akonadi.collection emptyCollection }',root);
            return collectionList.delegate.choices[0].delegate.createObject(root, {
                index:0, model:{display:"account@example.com",checkState:undefined,collection:holder.emptyCollection},
                decoration:"view-calendar",collectionId:0,collection:holder.emptyCollection,
                kDescendantExpanded:true,kDescendantLevel:0,collectionColor:"#9162e9",checkState:0
            });
        })())JS");
        auto accountRow=qobject_cast<QQuickItem*>(account.evaluate().value<QObject*>());
        if(!accountRow || account.hasError()) {fail("Real account delegate fixture failed: "+account.error().toString());return;}
        auto originalParent=accountRow->parentItem();
        const auto pos=originalParent->mapToScene(QPointF(0,0));
        accountRow->setParentItem(w->contentItem());accountRow->setPosition(QPointF(pos.x(),510));accountRow->setWidth(originalParent->width());accountRow->setZ(1000);
        QTest::qWait(100);
        QQuickItem *accountLabel=nullptr, *headingLabel=nullptr;
        for(auto item:items(accountRow)) if(item->property("text").toString()=="account@example.com" && item->isVisible() && item!=accountRow) accountLabel=item;
        for(auto item:items(w->contentItem())) if(item->property("text").toString()=="Calendars" && item->isVisible() && item->childItems().isEmpty()) headingLabel=item;
        if(!accountLabel || !headingLabel || qAbs(accountLabel->mapToScene(QPointF()).x()-headingLabel->mapToScene(QPointF()).x())>1) {fail("Account address and Calendars label do not align");return;}
        w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/calendar-account-alignment.png");
        delete accountRow;
        auto sidebar=find(w,"sequoiaCalendarSidebarCard");
        QQmlExpression indicators(qmlContext(sidebar),sidebar,R"JS(Qt.createQmlObject('import QtQuick; import QtQuick.Controls as C; import org.kde.merkuro.calendar as Calendar; Column { objectName:"sequoiaCalendarFixture"; x:24; y:490; spacing:6; Row { spacing:6; Calendar.SequoiaCalendarIndicator { objectName:"verifyCalendarOne"; checked:true; color:"#9162e9"; text:"Personal" } C.Label { text:"Personal"; anchors.verticalCenter:parent.verticalCenter } } Row { spacing:6; Calendar.SequoiaCalendarIndicator { objectName:"verifyCalendarTwo"; checked:false; color:"#2bbdc4"; text:"Birthdays" } C.Label { text:"Birthdays"; anchors.verticalCenter:parent.verticalCenter } } }',drawer.background))JS");
        auto fixture=indicators.evaluate().value<QObject*>();
        if(!fixture || indicators.hasError()) {fail("Calendar indicator fixture failed: "+indicators.error().toString());return;}
        auto fixtureItem=qobject_cast<QQuickItem*>(fixture);
        fixtureItem->setParentItem(w->contentItem());fixtureItem->setPosition(QPointF(24,510));fixtureItem->setZ(1000);
        QTest::qWait(50);
        auto first=find(w,"verifyCalendarOne"), second=find(w,"verifyCalendarTwo");
        for(auto control:{first,second}) {
            if(!control) {fail("Calendar indicator missing");return;}
            auto circle=qobject_cast<QQuickItem*>(control->property("indicator").value<QObject*>());
            if(!circle || circle->width()!=14 || circle->height()!=14 || circle->property("radius").toReal()!=7 || control->width()<26 || control->height()<28) {fail("Calendar circles must be 14px with their original click area");return;}
            if((circle->property("color").value<QColor>().alpha()>0)!=control->property("checked").toBool()) {fail("Calendar filled/outline state incorrect");return;}
        }
        click(w,"verifyCalendarTwo");
        if(!second->property("checked").toBool()) {fail("Calendar circle does not toggle");return;}
        click(w,"verifyCalendarTwo");
        w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/calendar-controls.png");
        delete fixture;
    }
    const auto path=qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY");
    if(phase==0 || phase==3) {
        const QString prefix=phase==0 ? "sequoiaMonth" : "sequoiaSchedule";
        auto button=find(w,prefix+"Extend"),arrow=find(w,prefix+"ExtendOptions");
        if(!button || !arrow || qAbs(arrow->mapToScene(QPointF()).x()-button->mapToScene(QPointF(button->width(),0)).x()-8)>1) {fail("Extend dropdown spacing does not match the toolbar");return;}
    }
    if(phase==0) {
        auto extend=find(w,"sequoiaMonthExtend");
        if(!extend) {fail("Month extend control missing");return;}
        if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_RESTART") && (!extend->property("checked").toBool() || !find(w,"sequoiaMonthCalendar3"))) {fail("Month grid preference not restored after restart");return;}
        for(int count=2;count<=4;++count) {
            click(w,"sequoiaMonthExtendOptions");QTest::qWait(100);
            if(!click(w,QString("sequoiaMonthMonths%1").arg(count))) {fail("Month view count option missing");return;}
            QTest::qWait(250);
            auto first=find(w,"sequoiaExtendedMonthGrid0");
            if(!first) {fail("Extended month grid missing");return;}
            const auto base=first->property("firstDayOfMonth").toDateTime().date();
            for(int index=0;index<count;++index) {
                auto grid=find(w,QString("sequoiaExtendedMonthGrid%1").arg(index));
                if(!grid || grid->property("firstDayOfMonth").toDateTime().date()!=base.addMonths(index) || grid->width()<300 || grid->height()<190) {fail("Month grid has wrong dates or insufficient space");return;}
                if(index%2==1 && grid->mapToScene(QPointF()).x()<=first->mapToScene(QPointF(first->width(),0)).x()) {fail("Month calendars are not in two columns");return;}
                if(index>=2 && grid->mapToScene(QPointF()).y()<=first->mapToScene(QPointF(0,first->height())).y()) {fail("Month calendars are not in two rows");return;}
            }
        }
        QQmlExpression selectVisible(qmlContext(extend),extend,R"JS((function(){ const first=new Date(bar.page.sequoiaMonthStart); Calendar.DateTimeState.selectedDate=new Date(first.getFullYear(),first.getMonth()+1,12); return bar.page.sequoiaMonthStart.getTime()===first.getTime(); })())JS");
        if(!selectVisible.evaluate().toBool()) {fail("Selecting another visible month shifted the grid");return;}
        QQmlExpression navigate(qmlContext(extend),extend,R"JS((function(){ const first=new Date(bar.page.sequoiaMonthStart); bar.page.nextAction.trigger(); const next=new Date(first.getFullYear(),first.getMonth()+1,1); return bar.page.sequoiaMonthStart.getTime()===next.getTime(); })())JS");
        if(!navigate.evaluate().toBool()) {fail("Month grid navigation did not advance");return;}
        QQmlExpression reset(qmlContext(extend),extend,"bar.page.todayAction.trigger()");reset.evaluate();QTest::qWait(200);
        w->grabWindow().save(path+"/month-four-calendars.png");
        w->resize(850,740);QTest::qWait(200);
        if(find(w,"sequoiaExtendedMonthGrid0")) {fail("Month grid did not collapse on a small window");return;}
        w->resize(1660,1000);QTest::qWait(200);
        if(!find(w,"sequoiaMonthCalendar3")) {fail("Month grid did not restore after widening");return;}
    }
    if(phase==2) {
        auto extend=find(w,"sequoiaWeekExtend");
        if(!extend) {fail("Week extend button missing");return;}
        if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_RESTART") && !extend->property("checked").toBool()) {fail("Week extension not restored after restart");return;}
        if(!extend->property("checked").toBool()) click(w,"sequoiaWeekExtend");
        QTest::qWait(250);
        auto next=find(w,"sequoiaNextWeek");
        QQmlExpression weekStart(qmlContext(extend),extend,"bar.page.sequoiaWeekStart");
        const auto start=weekStart.evaluate().toDateTime().date();
        if(!next || next->property("startDate").toDateTime().date()!=start.addDays(7) || next->property("daysToShow").toInt()!=7) {fail("Week extension did not show the following week");return;}
        if(!find(w,"sequoiaHourlyLineGrid")) {fail("Week line grid missing");return;}
        if(qAbs(find(w,"sequoiaNextWeekPanel")->width()-card->width())>1) {fail("Week extension is not split into equal halves");return;}
        QQmlExpression owner(qmlContext(extend),extend,"bar.page");
        auto ownerObject=owner.evaluate().value<QObject*>();
        auto sync=ownerObject ? ownerObject->findChild<QObject*>("sequoiaWeekScrollSync") : nullptr;
        if(!sync) {fail("Week scroll synchronizer missing");return;}
        auto viewport=[&sync](const char *property) {return qobject_cast<QQuickItem*>(sync->property(property).value<QObject*>());};
        auto aligned=[&]() {
            auto first=viewport("first"),second=viewport("second");
            return first && second && qAbs(first->property("contentY").toReal()-first->property("originY").toReal()-second->property("contentY").toReal()+second->property("originY").toReal())<1
                && qAbs(first->mapToScene(QPointF()).y()-second->mapToScene(QPointF()).y())<1
                && qAbs(first->height()-second->height())<1;
        };
        if(!aligned()) {fail("Extended Week time rows are not initially aligned");return;}
        if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_RESTART") && qAbs((viewport("first")->property("contentY").toReal()-viewport("first")->property("originY").toReal())/sync->property("pixelsPerMinute").toReal()-570)>1) {fail("Week did not restore its saved time after app restart");return;}
        viewport("first")->setProperty("contentY",500);QTest::qWait(80);
        if(!aligned() || qAbs(viewport("second")->property("contentY").toReal()-500)>1) {fail("Left Week scrolling did not move both panes");return;}
        viewport("second")->setProperty("contentY",800);QTest::qWait(80);
        if(!aligned() || qAbs(viewport("first")->property("contentY").toReal()-800)>1) {fail("Right Week scrolling did not move both panes");return;}
        auto firstView=sync->property("firstView").value<QObject*>();
        firstView->setProperty("multiDayLinesShown",4);QTest::qWait(250);
        if(!aligned()) {fail("Different all-day header heights offset Week time rows");return;}
        firstView->setProperty("multiDayLinesShown",0);QTest::qWait(200);
        QQmlExpression mode(qmlContext(extend),extend,"Calendar.Config.hourlyViewMode");
        const int savedMode=mode.evaluate().toInt();
        QQmlExpression basic(qmlContext(extend),extend,"Calendar.Config.hourlyViewMode = Calendar.Config.BasicInternalHourlyView");
        basic.evaluate();QTest::qWait(250);
        viewport("second")->setProperty("contentY",650);QTest::qWait(80);
        if(!aligned()) {fail("Basic Week mode did not synchronize scrolling");return;}
        QQmlExpression restore(qmlContext(extend),extend,QString("Calendar.Config.hourlyViewMode = %1").arg(savedMode));
        restore.evaluate();QTest::qWait(800);
        if(!aligned()) {fail("Week scrolling did not reconnect after replacing the native view");return;}
        w->grabWindow().save(path+"/week-extended.png");
        w->resize(950,740);QTest::qWait(200);
        if(find(w,"sequoiaNextWeekPanel")) {fail("Week split did not collapse on a small window");return;}
        w->resize(1660,1000);QTest::qWait(200);
        if(!find(w,"sequoiaNextWeekPanel")) {fail("Week split did not restore after widening");return;}
        click(w,"sequoiaWeekExtend");QTest::qWait(150);
        if(find(w,"sequoiaNextWeekPanel")) {fail("Single week did not collapse split view");return;}
        // Scroll in single-week mode, then leave immediately to exercise the
        // destruction flush rather than waiting for the disk-write timer.
        viewport("first")->setProperty("contentY",600*sync->property("pixelsPerMinute").toReal());
        click(w,"sequoiaCalendarNav-open_month_view");QTest::qWait(350);
        click(w,"sequoiaCalendarNav-open_week_view");QTest::qWait(850);
        extend=find(w,"sequoiaWeekExtend");
        QQmlExpression reopenedOwner(qmlContext(extend),extend,"bar.page");
        ownerObject=reopenedOwner.evaluate().value<QObject*>();
        sync=ownerObject ? ownerObject->findChild<QObject*>("sequoiaWeekScrollSync") : nullptr;
        if(!sync || !viewport("first") || qAbs(viewport("first")->property("contentY").toReal()/sync->property("pixelsPerMinute").toReal()-600)>1) {fail("Single Week forgot its time after switching views");return;}
        click(w,"sequoiaWeekExtend");QTest::qWait(250);
        if(!aligned() || qAbs(viewport("second")->property("contentY").toReal()/sync->property("pixelsPerMinute").toReal()-600)>1) {fail("Extended halves did not restore the single-week time");return;}
        viewport("second")->setProperty("contentY",570*sync->property("pixelsPerMinute").toReal());
        QTest::qWait(200);
        QSettings weekSettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/sequoia-merkuro.ini",QSettings::IniFormat);
        if(qAbs(weekSettings.value("Week/scrollMinute",-1).toReal()-sync->property("rememberedMinute").toReal())>.001 || qAbs(sync->property("rememberedMinute").toReal()-570)>1) {fail("Week scroll time was not persisted: saved="+weekSettings.value("Week/scrollMinute",-1).toString()+" memory="+sync->property("rememberedMinute").toString()+" y="+viewport("second")->property("contentY").toString());return;}
    }
    if(phase==4) {
        auto list=find(w,"sequoiaTaskPageList"), add=find(w,"sequoiaTaskPageAdd"), create=find(w,"sequoiaCalendarCreate");
        if(!list || !add || !create) {fail("Tasks content or create control missing");return;}
        const auto bounds=card->mapRectToScene(card->boundingRect());
        if(bounds.top()<=create->mapToScene(QPointF(0,create->height())).y() || !bounds.adjusted(15,15,-15,-15).contains(list->mapRectToScene(list->boundingRect())) || !bounds.adjusted(15,15,-15,-15).contains(add->mapRectToScene(add->boundingRect()))) {fail("Tasks surface includes toolbar or content touches card edges");return;}
        const auto compactWidth=add->width();
        if(compactWidth>350 || compactWidth<120) {fail("Task entry does not fit its placeholder content");return;}
        add->setProperty("text",QString(300,'W'));QTest::qWait(80);
        if(add->width()>card->width()-31 || add->width()<=compactWidth) {fail("Task entry does not grow within its card");return;}
        add->setProperty("text",QString());QTest::qWait(80);
        if(qAbs(add->width()-compactWidth)>1) {fail("Task entry did not shrink after clearing text");return;}
        auto taskModel=new QStandardItemModel(w);
        const QStringList roles{"text","displayDueDate","isOverdue","percent","todoCompleted","isReadOnly","color","incidencePtr","collectionId","startTime","endTime","priority","todoCategories","recurs","topMostParentDueDate"};
        QHash<int,QByteArray> roleNames{{Qt::CheckStateRole,"checked"}};
        for(int i=0;i<roles.size();++i) roleNames[Qt::UserRole+i+1]=roles[i].toUtf8();
        taskModel->setItemRoleNames(roleNames);
        auto task=new QStandardItem;
        const QVariantList values{QString("Review the upcoming project milestones"),QString(),false,0,false,false,QColor("#c496dc"),QVariant::fromValue<QObject*>(nullptr),0,QDateTime::currentDateTime(),QDateTime(),0,QStringList{},false,QString("Today")};
        for(int i=0;i<values.size();++i) task->setData(values[i],Qt::UserRole+i+1);
        task->setData(Qt::Unchecked,Qt::CheckStateRole);taskModel->appendRow(task);
        qmlEngine(list)->rootContext()->setContextProperty("taskFixtureSource",taskModel);
        QQmlExpression fixture(qmlContext(list),list,"model = Qt.createQmlObject('import org.kde.kitemmodels; KDescendantsProxyModel { expandsByDefault: true; model: taskFixtureSource }', incompleteView)");
        fixture.evaluate();
        if(fixture.hasError()) {fail("Tasks row fixture failed: " + fixture.error().toString());return;}
        QTest::qWait(200);
        auto check=find(w,"sequoiaTaskPageCheck");
        if(!check) {fail("Styled task checkbox missing");return;}
        click(w,"sequoiaTaskPageCheck");
        if(task->data(Qt::CheckStateRole).toInt()!=Qt::Checked) {fail("Task completion control stopped updating the model");return;}
        w->grabWindow().save(path+"/tasks-styled.png");
    }
    if(phase==3) {
        auto weather=find(w,"sequoiaScheduleWeather");
        if(!weather || weather->mapToScene(QPointF()).x()<=mapped.right()) {fail("Schedule forecast column is not to the right");return;}
        auto list=find(w,"sequoiaScheduleForecastDays");
        if(!list) {fail("Schedule weather list missing");return;}
        auto extend=find(w,"sequoiaScheduleExtend");
        if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_RESTART")) {
            w->resize(2200,1000);QTest::qWait(250);
            if(!extend || !extend->property("checked").toBool() || !find(w,"sequoiaExtraScheduleMonth3")
                || weather->property("monthCount").toInt()!=4) {fail("Saved four-month choice was not restored after restart");return;}
            if(!scheduleScrollCheck(w,true)) return;
            qInfo("MAKEOVER RESTART PASS");qInfo("MAKEOVER FINISHED");QCoreApplication::quit();return;
        }
        if(!click(w,"sequoiaScheduleExtend")) {fail("Extend view button missing");return;}
        QTest::qWait(250);
        auto nextPanel=find(w,"sequoiaNextScheduleMonth");
        auto nextLoader=find(w,"sequoiaNextScheduleLoader");
        auto nextView=nextLoader ? nextLoader->property("item").value<QObject*>() : nullptr;
        const auto baseMonth=weather->property("month").toDateTime().date();
        if(!nextPanel || !nextView || nextView->property("startDate").toDateTime().date()!=baseMonth.addMonths(1)
            || nextPanel->mapToScene(QPointF()).x()<=card->mapToScene(QPointF(card->width(),0)).x()
            || weather->mapToScene(QPointF()).x()<=nextPanel->mapToScene(QPointF(nextPanel->width(),0)).x()) {fail("Extended Schedule month or column arrangement is incorrect");return;}
        if(!click(w,"sequoiaScheduleWeatherNext")) {fail("Split weather Next button missing");return;}
        QTest::qWait(100);
        if(weather->property("monthStart").toDateTime().date()!=baseMonth.addMonths(1)) {fail("Weather Next did not select the second month");return;}
        click(w,"sequoiaScheduleWeatherPrevious");QTest::qWait(100);
        if(weather->property("monthStart").toDateTime().date()!=baseMonth) {fail("Weather Previous did not return to the first month");return;}
        QQmlExpression savedMode(qmlContext(list),list,"Calendar.Config.monthListMode");
        const int monthListMode=savedMode.evaluate().toInt();
        QQmlExpression basicMode(qmlContext(list),list,"Calendar.Config.monthListMode = Calendar.Config.BasicMonthList");
        basicMode.evaluate();QTest::qWait(200);
        auto basicLoader=find(w,"sequoiaPrimaryScheduleLoader");
        auto basicView=basicLoader ? basicLoader->property("item").value<QObject*>() : nullptr;
        if(!basicView || basicView->property("startDate").toDateTime().date()!=baseMonth
            || nextView->property("startDate").toDateTime().date()!=baseMonth.addMonths(1)) {fail("Basic Schedule mode did not cover the two labeled months");return;}
        QQmlExpression restoreMode(qmlContext(list),list,QString("Calendar.Config.monthListMode = %1").arg(monthListMode));
        restoreMode.evaluate();QTest::qWait(200);
        QQmlExpression december(qmlContext(list),list,"Calendar.DateTimeState.selectedDate = new Date(2026, 11, 15)");
        december.evaluate();QTest::qWait(250);
        if(nextView->property("startDate").toDateTime().date()!=QDate(2027,1,1)) {fail("Extended Schedule failed December-to-January rollover");return;}
        QQmlExpression resetMonth(qmlContext(list),list,"Calendar.DateTimeState.resetTime()");
        resetMonth.evaluate();QTest::qWait(800);
        QQuickItem *nativeList=nullptr;
        const auto currentMonthBounds=card->mapRectToScene(card->boundingRect());
        for(auto candidate:items(card)) if(candidate->objectName()=="sequoiaNativeScheduleDays" && candidate->isVisible()) {
            const auto bounds=candidate->mapRectToScene(candidate->boundingRect());
            if(bounds.intersected(currentMonthBounds).width()>candidate->width()*.5) nativeList=candidate;
        }
        if(!nativeList) {fail("Native schedule list missing");return;}
        QQmlExpression nativeFixture(qmlContext(nativeList),nativeList,R"JS((function() {
            const today=new Date();today.setHours(0,0,0,0);
            const start=new Date(today);start.setHours(15);
            const end=new Date(start);end.setHours(16);
            const event={incidenceId:"test-event",incidencePtr:null,startTime:start,endTime:end,
                text:"Afternoon walk with the neighborhood group and a stop at the community garden",color:"#8fb7b1",collectionId:0,allDay:false,
                todoCompleted:false,isReadOnly:true,
                description:"Meet at the main entrance.\nBring water and comfortable shoes.\nFollow the riverside path to the community garden.\nLeave time for a short break at the overlook.\nReturn along the shaded trail.\nEveryone is welcome to join for part of the route.",
                location:"Local park, north entrance beside the community center",
                hasReminders:false,recurs:false,incidenceTypeIcon:"view-calendar",incidenceType:0};
            const shortEvent=Object.assign({},event,{incidenceId:"test-short",text:"Evening check-in",description:"",location:""});
            const reminder=Object.assign({},shortEvent,{incidenceId:"test-reminder",text:"Plan tomorrow"});
            scheduleListView.model=[{periodStartDate:today,incidences:[[event],[shortEvent],[reminder]]}];
            scheduleListView.positionViewAtBeginning();return true;
        })())JS");
        if(!nativeFixture.evaluate().toBool() || nativeFixture.hasError()) {fail("Native Schedule fixture failed: "+nativeFixture.error().toString());return;}
        QQuickItem *nextList=nullptr;
        for(auto item:items(nextPanel)) if(item->objectName()=="sequoiaNativeScheduleDays") nextList=item;
        if(!nextList) {fail("Next month's native event list missing");return;}
        QQmlExpression nextFixture(qmlContext(nextList),nextList,R"JS((function() {
            const date=new Date(scrollView.startDate);date.setDate(4);
            const event={incidenceId:"test-next-month",incidencePtr:null,startTime:date,endTime:date,
                text:"Next-month planning",color:"#c496dc",collectionId:0,allDay:true,
                todoCompleted:false,isReadOnly:true,description:"Review the coming month's plans.",location:"",
                hasReminders:false,recurs:false,incidenceTypeIcon:"view-calendar",incidenceType:0};
            scheduleListView.model=[{periodStartDate:date,incidences:[[event]]}];
            return true;
        })())JS");
        if(!nextFixture.evaluate().toBool() || nextFixture.hasError()) {fail("Next-month event fixture failed");return;}
        // Feed the same day grouping used by the real occurrence model. Duplicate
        // occurrences and empty days must not produce extra forecast cards.
        QQmlExpression fixture(qmlContext(list),list,R"JS((function() {
            const today=new Date(); today.setHours(0,0,0,0);
            const tomorrow=new Date(today);tomorrow.setDate(today.getDate()+1);
            const distant=new Date(today);distant.setDate(today.getDate()+24);
            const event={incidenceId:"test-event",startTime:today,text:"Afternoon walk"};
            periods.model=[{periodStartDate:today,incidences:[[event,event]]},
                {periodStartDate:tomorrow,incidences:[]},
                {periodStartDate:distant,incidences:[[{incidenceId:"later",startTime:distant,text:"Weekend plans"}]]}];
            const forecasts={};forecasts[Qt.formatDate(today,"yyyy-MM-dd")]={temperature_2m_max:18,temperature_2m_min:8,weather_code:2,precipitation_probability_max:20};
            panel.forecasts=forecasts;
            return true;
        })())JS");
        if(!fixture.evaluate().toBool() || fixture.hasError()) {fail("Schedule weather fixture failed: "+fixture.error().toString());return;}
        QTest::qWait(200);
        if(!list || list->property("count").toInt()!=2) {fail("Weather cards did not exclude empty days/deduplicate events");return;}
        QQmlExpression grouped(qmlContext(list),list,"panel.eventDays[0].titles.length === 1 && panel.eventDays[1].titles[0] === 'Weekend plans'");
        if(!grouped.evaluate().toBool()) {fail("Weather day grouping lost events");return;}
        QQmlExpression overflow(qmlContext(list),list,"panel.eventDays = Array.from({length: 12}, (_,i) => ({date:new Date(2026,10,i+1),key:'2026-11-'+String(i+1).padStart(2,'0'),titles:['Planned event']}))");
        overflow.evaluate();QTest::qWait(200);
        auto weatherBar=find(w,"sequoiaWeatherScrollBar");
        QTest::mouseMove(w,weather->mapToScene(QPointF(24,24)).toPoint());QTest::qWait(350);
        if(!weatherBar || weatherBar->opacity()<.95 || weatherBar->mapToScene(QPointF()).x()>=list->mapToScene(QPointF(list->width(),0)).x()) {fail("Weather scrollbar is not a visible overlay on hover");return;}
        QTest::mouseMove(w,QPoint(5,5));QTest::qWait(350);
        if(weatherBar->opacity()>.05) {fail("Weather scrollbar did not hide after leaving panel");return;}
        w->grabWindow().save(path+"/schedule-weather.png");
        w->resize(2200,1000);QTest::qWait(200);
        for(int count=2;count<=4;++count) {
            if(!click(w,"sequoiaScheduleExtendOptions")) {fail("Month count dropdown missing");return;}
            QTest::qWait(150);
            if(count==4) w->grabWindow().save(path+"/schedule-extend-menu.png");
            if(!click(w,QString("sequoiaScheduleMonths%1").arg(count))) {fail("Month count option missing");return;}
            QTest::qWait(250);
            if(weather->property("monthCount").toInt()!=count) {fail("Selected month count was not applied");return;}
            for(int offset=1;offset<count;++offset) {
                const auto name=offset==1 ? QString("sequoiaNextScheduleLoader") : QString("sequoiaExtraScheduleLoader%1").arg(offset);
                auto loader=find(w,name);auto view=loader ? loader->property("item").value<QObject*>() : nullptr;
                if(!view || view->property("startDate").toDateTime().date()!=baseMonth.addMonths(offset)) {fail("Extended month has the wrong date");return;}
                auto panel=loader->parentItem()->parentItem();
                if(panel->width()<280 || panel->mapToScene(QPointF(panel->width(),0)).x()>=weather->mapToScene(QPointF()).x()) {fail("Extended months overlap or are too narrow");return;}
            }
        }
        for(int offset=1;offset<4;++offset) {
            click(w,"sequoiaScheduleWeatherNext");QTest::qWait(100);
            if(weather->property("monthStart").toDateTime().date()!=baseMonth.addMonths(offset)) {fail("Weather cannot navigate all four months");return;}
        }
        if(find(w,"sequoiaScheduleWeatherNext")->isEnabled()) {fail("Weather can advance beyond the last visible month");return;}
        for(int offset=0;offset<3;++offset) click(w,"sequoiaScheduleWeatherPrevious");
        QTest::qWait(100);
        if(find(w,"sequoiaScheduleWeatherPrevious")->isEnabled()) {fail("Weather can go before the first month");return;}
        w->grabWindow().save(path+"/schedule-four-months.png");
        w->resize(700,740);QTest::qWait(250);
        card=find(w,"sequoiaMerkuroCalendar");weather=find(w,"sequoiaScheduleWeather");
        if(!card || !weather || weather->mapToScene(QPointF()).y()<card->mapToScene(QPointF(0,card->height())).y()
            || weather->mapToScene(QPointF(weather->width(),weather->height())).y()>w->height()) {fail("Narrow Schedule did not stack weather below the agenda");return;}
        if(find(w,"sequoiaNextScheduleMonth") || find(w,"sequoiaScheduleWeatherNext")) {fail("Split month did not collapse on a narrow window");return;}
        w->grabWindow().save(path+"/schedule-weather-narrow.png");
        w->resize(1660,1000);QTest::qWait(200);
        if(!find(w,"sequoiaNextScheduleMonth") || weather->property("monthCount").toInt()!=2) {fail("Extended Schedule did not restore as many months as fit beside the expanded sidebar");return;}
        w->resize(2200,1000);QTest::qWait(200);
        if(!find(w,"sequoiaExtraScheduleMonth3") || weather->property("monthCount").toInt()!=4) {fail("Four-month choice was lost after narrowing and widening");return;}
        click(w,"sequoiaScheduleExtend");QTest::qWait(150);
        if(find(w,"sequoiaNextScheduleMonth") || find(w,"sequoiaScheduleWeatherNext")) {fail("Single month did not collapse the extended view");return;}
        const auto settingsPath=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/sequoia-merkuro.ini";
        QSettings saved(settingsPath,QSettings::IniFormat);
        if(saved.value("Schedule/extended",true).toBool() || saved.value("Schedule/viewCount").toInt()!=4) {fail("Single-month preference or remembered extended count was not saved: " + settingsPath + " keys=" + saved.allKeys().join(",") + " extended=" + saved.value("Schedule/extended").toString() + " count=" + saved.value("Schedule/viewCount").toString());return;}
        click(w,"sequoiaScheduleExtend");QTest::qWait(150);
        if(weather->property("monthCount").toInt()!=4) {fail("Extend toggle forgot the chosen month count");return;}
        w->resize(1660,1000);QTest::qWait(150);
        if(!scheduleScrollCheck(w,false)) return;
        w->resize(1660,1000);QTest::qWait(200);
    }
    if(!w->grabWindow().save(path+"/"+pages.at(phase)+".png")) {fail("Capture failed");return;}
    qInfo("MAKEOVER PASS: %s",qPrintable(pages.at(phase)));
    if(phase==1) {
        auto taskList=find(w,"sequoiaDashboardTasks");
        auto addTask=find(w,"sequoiaDashboardAddTask");
        if(!taskList || !addTask || taskList->property("count").toInt()!=0) {fail("Tasks empty state contains unexpected tasks or lacks Add task");return;}
        // Only this private test ListView gets sample rows. Never persist tasks.
        QQmlExpression taskFixture(qmlContext(taskList),taskList,R"JS((function() {
            model=Qt.createQmlObject('import QtQuick; ListModel { ListElement { text:"Review notes"; todoCompleted:false; isReadOnly:false; checked:0 } ListElement { text:"Plan next week"; todoCompleted:false; isReadOnly:false; checked:0 } }',todos);
            return true;
        })())JS");
        if(!taskFixture.evaluate().toBool() || taskFixture.hasError()) {fail("Task fixture failed: "+taskFixture.error().toString());return;}
        QTest::qWait(100);
        if(!click(w,"sequoiaDashboardTaskCheck")) {fail("Task checkbox missing");return;}
        QQmlExpression completedCheck(qmlContext(taskList),taskList,"model.get(0).checked === 2");
        if(!completedCheck.evaluate().toBool()) {fail("Task checkbox did not update the model");return;}
        w->grabWindow().save(path+"/tasks-card.png");
        taskList->setProperty("model",QVariantList{});
        auto next=find(w,"sequoiaUpNextCard");
        QQmlExpression check(qmlContext(next),next,R"JS((function() {
            const original = events;
            now = new Date(2026, 10, 14, 10);
            events = [{text:"All day",allDay:true,startTime:new Date(2026,10,14)},
                      {text:"Past",allDay:false,startTime:new Date(2026,10,14,9)},
                      {text:"Next",allDay:false,startTime:new Date(2026,10,14,11)},
                      {text:"Later",allDay:false,startTime:new Date(2026,10,14,12)}].map(event => {event.endTime=new Date(event.startTime.getTime()+3600000);return event;});
            const selected = nextEvent.text === "Next";
            now = new Date(2026,10,14,13);
            const exhausted = nextEvent === null;
            events = original;
            now = new Date();
            return selected && exhausted;
        })())JS");
        const auto selectionResult=check.evaluate();
        if(!selectionResult.toBool() || check.hasError()) {fail("Up next event selection failed: "+check.error().toString()+" result="+selectionResult.toString());return;}
        if(find(w,"sequoiaUpNextTitle") || find(w,"sequoiaUpNextTime") || find(w,"sequoiaUpNextCalendar") || find(w,"sequoiaUpNextCountdown") || find(w,"sequoiaUpNextViewEvent") || !find(w,"sequoiaUpNextEmpty")) {fail("Empty Up next exposes event details");return;}
        QQmlExpression upcoming(qmlContext(next),next,R"JS((function() {
            now=new Date(2026,10,15,11,15);
            events=[{text:"Lunch with Alex",allDay:false,startTime:new Date(2026,10,15,12,30),endTime:new Date(2026,10,15,13,30),color:"#a25cff",collectionId:0}];
            calendarDetails={displayName:"Personal"};
            return timeUntil === "In 1 hour 15 minutes" && calendarName === "Personal";
        })())JS");
        if(!upcoming.evaluate().toBool() || upcoming.hasError()) {fail("Up next countdown/calendar fixture failed: "+upcoming.error().toString());return;}
        QTest::qWait(100);
        for(const auto &name:QStringList{"sequoiaUpNextTitle","sequoiaUpNextTime","sequoiaUpNextCalendar","sequoiaUpNextCountdown","sequoiaUpNextViewEvent"}) {
            auto item=find(w,name);
            if(!item || !next->mapRectToScene(next->boundingRect()).contains(item->mapRectToScene(item->boundingRect()))) {fail("Up next content missing or outside card: "+name);return;}
        }
        if(find(w,"sequoiaUpNextEmpty")) {fail("Up next empty message overlaps an event");return;}
        w->grabWindow().save(path+"/up-next-event.png");
        next->setProperty("events",QVariantList{});
        QTest::qWait(50);
        if(find(w,"sequoiaUpNextTitle") || find(w,"sequoiaUpNextTime") || find(w,"sequoiaUpNextCalendar") || find(w,"sequoiaUpNextCountdown") || find(w,"sequoiaUpNextViewEvent") || !find(w,"sequoiaUpNextEmpty")) {fail("Removing the event left stale Up next details");return;}
        w->grabWindow().save(path+"/up-next-empty.png");
        // Populate the native visual repeater only in this isolated test process.
        // No sample events are written to Akonadi or installed in the app.
        auto repeater=find(w,"sequoiaTimelineEvents");
        if(!repeater) {fail("Today native timeline repeater missing");return;}
        QQmlExpression fixture(qmlContext(repeater),repeater,R"JS((function() {
            const day = viewColumn.startDate;
            function at(hour, minute) {return new Date(day.getFullYear(),day.getMonth(),day.getDate(),hour,minute);}
            model = [{text:"Design review",color:"#3678ff",hour:9,minute:0},
                     {text:"Lunch with Alex",color:"#a25cff",hour:12,minute:30},
                     {text:"Family dinner",color:"#2bbdc4",hour:18,minute:0}].map((event,index)=>{
                event.startTime=at(event.hour,event.minute);event.endTime=at(event.hour+1,event.minute);
                event.starts=event.hour*4+event.minute/15;event.duration=4;
                event.widthShare=1;event.priorTakenWidthShare=0;event.allDay=false;
                event.collectionId=0;event.incidencePtr=null;event.incidenceId="fixture-"+index;
                event.isReadOnly=true;event.resizeable=false;event.todoCompleted=false;
                event.incidenceTypeIcon="view-calendar";
                return event;
            });
            return true;
        })())JS");
        if(!fixture.evaluate().toBool() || fixture.hasError()) {fail("Timeline fixture failed: "+fixture.error().toString());return;}
        int eventCount=0;
        for(auto item:items(w->contentItem())) if(item->objectName()=="sequoiaTimelineEvent" && item->isVisible()) {
            const auto title=item->property("title").toString();
            item->setProperty("calendarName",title=="Design review" ? "Work" : title=="Lunch with Alex" ? "Personal" : "Family");
            if(item->width()<50 || item->height()<50 || item->property("radius").toInt()!=8) {fail("Timeline event styling/size incorrect");return;}
            ++eventCount;
        }
        if(!find(w,"sequoiaTodayTimePill")) {QQmlExpression dates(qmlContext(repeater),repeater,"[viewColumn.currentDate.toString(),viewColumn.startDate.toString(),viewColumn.endDate.toString()].join(' / ')");qInfo("TIMELINE DATES: %s",qPrintable(dates.evaluate().toString()));}
        w->grabWindow().save(path+"/today-events.png");
        if(eventCount!=3 || !find(w,"sequoiaTodayTimePill")) {fail(QString("Timeline event count %1, current-time pill %2").arg(eventCount).arg(find(w,"sequoiaTodayTimePill")!=nullptr));return;}
        w->grabWindow().save(path+"/today-events.png");
        auto allDayRows=find(w,"sequoiaAllDayRows");
        if(!allDayRows) {fail("All-day rows missing");return;}
        QQmlExpression allDayFixture(qmlContext(allDayRows),allDayRows,R"JS((function() {
            hourlyView.contentItem.contentY=0;
            const start = viewColumn.startDate;
            const end = new Date(start.getFullYear(),start.getMonth(),start.getDate()+1);
            model = [[{text:"Birthday",color:"#2bbdc4",starts:0,duration:1,
                incidenceId:"fixture-birthday",incidencePtr:null,startTime:start,endTime:end,
                allDay:true,collectionId:0,isReadOnly:true,resizeable:false,todoCompleted:false,
                incidenceType:Calendar.IncidenceWrapper.TypeEvent,incidenceTypeIcon:"view-calendar"}]];
            return true;
        })())JS");
        if(!allDayFixture.evaluate().toBool() || allDayFixture.hasError()) {fail("All-day fixture: "+allDayFixture.error().toString());return;}
        QTest::qWait(250);
        int allDaySkins=0;
        for(auto item:items(w->contentItem())) if(item->objectName()=="sequoiaTimelineEvent" && item->isVisible() && item->property("allDay").toBool()) {
            item->setProperty("calendarName","Birthdays");
            if(item->height()<50) {fail("All-day card is still a thin bar");return;}
            ++allDaySkins;
        }
        QQmlExpression positioned(qmlContext(allDayRows),allDayRows,"Math.abs(hourlyView.contentItem.contentY-(8*(hourlyView.hourHeight+viewColumn.gridLineWidth)-10))<2");
        if(allDaySkins!=1 || !positioned.evaluate().toBool()) {fail("All-day loading reset the timeline start or lost its styled card");return;}
        w->grabWindow().save(path+"/today-all-day.png");
        allDayRows->setProperty("model",QVariantList{});
        repeater->setProperty("model",QVariantList{});
        w->resize(850,740);
        QTimer::singleShot(600,qApp,[]{
            auto w=window(); auto timeline=find(w,"sequoiaMerkuroCalendar"), next=find(w,"sequoiaUpNextCard"), mini=find(w,"sequoiaMiniCalendar");
            if(!timeline || !next || !mini || next->mapToScene(QPointF()).x()<=timeline->mapToScene(QPointF()).x() || mini->mapToScene(QPointF()).y()<=timeline->mapToScene(QPointF()).y()) {fail("Two-column Today reflow failed");return;}
            w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/today-compact.png");
            w->resize(600,740);
            QTimer::singleShot(600,qApp,[]{
                auto w=window(); auto timeline=find(w,"sequoiaMerkuroCalendar"), next=find(w,"sequoiaUpNextCard"), dashboard=find(w,"sequoiaTodayDashboard");
                if(!timeline || !next || !dashboard || qAbs(next->mapToScene(QPointF()).x()-timeline->mapToScene(QPointF()).x())>1 || dashboard->property("contentHeight").toReal()<=dashboard->height()) {fail("Single-column Today scrolling failed");return;}
                w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/today-narrow.png");
                w->resize(1660,1000);
                QTimer::singleShot(400,qApp,[]{auto w=window();if(!click(w,"sequoiaCalendarNav-open_week_view")){fail("Week navigation missing");return;}QTimer::singleShot(850,qApp,[]{step(2);});});
            });
        });
        return;
    }
    if(phase==5) {
        // Open the actual native event editor, then leave it without saving.
        if(!click(w,"sequoiaCalendarCreate")) {fail("Create control missing");return;}
        QTimer::singleShot(1000,qApp,[]{
            QQuickWindow *editor=nullptr;
            for(auto candidate:QGuiApplication::allWindows()) if(candidate->isVisible() && candidate!=window()) editor=qobject_cast<QQuickWindow *>(candidate);
            // The editor may be the first window in Qt's list; distinguish by size.
            for(auto candidate:QGuiApplication::allWindows()) if(candidate->isVisible() && candidate->width()<800) editor=qobject_cast<QQuickWindow *>(candidate);
            if(!editor) {fail("Native event editor did not open");return;}
            editor->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/event-editor.png");
            qInfo("MAKEOVER PASS: native event editor");
            QTest::keyClick(editor,Qt::Key_Escape);
            QTimer::singleShot(500,qApp,[]{qInfo("MAKEOVER FINISHED"); QCoreApplication::quit();});
        });
        return;
    }
    if(phase==4) w->resize(820,740);
    if(!click(w,"sequoiaCalendarNav-"+actions.at(phase))) {fail("Navigation control missing");return;}
    QTimer::singleShot(850,qApp,[phase]{step(phase+1);});
}
static void verifySidebar(int phase=0) {
    auto w=window(); if(!w) {fail("Missing window in sidebar check");return;}
    auto card=find(w,"sequoiaMerkuroCalendar");
    if(!card) {fail("Missing calendar in sidebar check");return;}
    const auto rect=card->mapRectToScene(card->boundingRect());
    if(phase==1) {
        if(find(w,"sequoiaCalendarSidebarCard") || rect.left()>20) {fail("Sidebar did not fully hide or still reserves space");return;}
        w->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/sidebar-hidden.png");
    } else {
        auto sidebar=find(w,"sequoiaCalendarSidebarCard");
        if(!sidebar) {fail("Sidebar did not reopen");return;}
        const auto side=sidebar->mapRectToScene(sidebar->boundingRect());
        auto toggle=find(w,"sequoiaCalendarSidebarToggle");
        if(!toggle || qAbs(toggle->mapToScene(QPointF()).y()-side.top())>1) {fail(QString("Toolbar/sidebar top: %1 / %2").arg(toggle ? toggle->mapToScene(QPointF()).y() : -1).arg(side.top()));return;}
        if(qAbs(rect.left()-side.right()-16)>1) {fail("Sidebar/calendar gap is not 16px");return;}
    }
    if(phase==2) {
        auto toggle=find(w,"sequoiaCalendarSidebarToggle");
        QQmlExpression drawerExpr(qmlContext(toggle),toggle,"bar.app.mainDrawer");
        auto drawer=drawerExpr.evaluate().value<QObject*>();
        for(int width:{950,1660,600,1660}) {
            w->resize(width,1000);QTest::qWait(250);
            if(!drawer || drawer->property("collapsed").toBool()!=(width<1050) || qAbs(drawer->property("width").toReal()-(width<1050 ? 80 : 250))>1) {fail("Sidebar did not regain its expanded shape after resizing");return;}
        }
        QQmlExpression force(qmlContext(toggle),toggle,"Calendar.Config.forceCollapsedMainDrawer = true");force.evaluate();QTest::qWait(150);
        if(!drawer->property("collapsed").toBool()) {fail("Sidebar ignored explicit compact preference");return;}
        QQmlExpression unforce(qmlContext(toggle),toggle,"Calendar.Config.forceCollapsedMainDrawer = false");unforce.evaluate();QTest::qWait(150);
        if(drawer->property("collapsed").toBool()) {fail("Sidebar did not expand after clearing compact preference");return;}
        qInfo("MAKEOVER: sidebar hides, reopens, and restores full width after resizing");step(0);return;
    }
    if(!click(w,"sequoiaCalendarSidebarToggle")) {fail("Sidebar toggle missing");return;}
    QTimer::singleShot(600,qApp,[phase]{verifySidebar(phase+1);});
}
static void verifySettings(QQuickWindow *main) {
    QQmlExpression open(qmlContext(main),main,"configureAction.trigger()");
    open.evaluate();
    QTest::qWait(900);
    QQuickWindow *settings=nullptr;
    for(auto candidate:QGuiApplication::allWindows())
        if(candidate->objectName()=="sequoiaMerkuroSettingsWindow") settings=qobject_cast<QQuickWindow *>(candidate);
    if(!settings || !settings->isVisible()) {fail("Settings window did not open");return;}
    settings->requestActivate();QTest::qWait(100);
    QSettings remembered(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/verify-settings.ini",QSettings::IniFormat);
    auto restored=[&](QQuickWindow *dialog) {
        if(dialog->size()!=remembered.value("size").toSize()) {fail("Settings size not restored: "+QString::number(dialog->width())+"x"+QString::number(dialog->height()));return false;}
        if(QGuiApplication::platformName()=="xcb" && dialog->position()!=remembered.value("position").toPoint()) {fail("Settings position not restored");return false;}
        for(const auto &id:QStringList{"freebusy","appearance"}) {
            if(id=="appearance") {click(dialog,"sequoiaSettingsNav-appearance");QTest::qWait(350);}
            auto page=find(dialog,"sequoiaSettingsPage-"+(id=="freebusy"?QString("FreeBusySettingsPage"):QString("ViewSettingsPage")));
            if(!page) {fail("Settings did not restore its last page");return false;}
            QQmlExpression offset(qmlContext(page),page,"flickable.contentY - flickable.originY + flickable.topMargin");
            if(qAbs(offset.evaluate().toDouble()-remembered.value(id).toDouble())>2) {fail("Settings scroll not restored: "+id+" actual="+offset.evaluate().toString()+" expected="+remembered.value(id).toString());return false;}
        }
        click(dialog,"sequoiaSettingsNav-freebusy");QTest::qWait(200);
        return true;
    };
    if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_SETTINGS_RESTART")) {
        if(!restored(settings)) return;
        QTest::keyClick(settings,Qt::Key_Escape);QTest::qWait(150);
        qInfo("MAKEOVER SETTINGS RESTART PASS");qInfo("MAKEOVER FINISHED");QCoreApplication::quit();return;
    }
    settings->resize(1100,850);QTest::qWait(300);
    const QStringList modules{"appearance","users","freebusy"};
    const QStringList pages{"ViewSettingsPage","SourceSettingsPage","FreeBusySettingsPage"};
    for(int n=0;n<modules.size();++n) {
        if(!click(settings,"sequoiaSettingsNav-"+modules[n])) {fail("Settings navigation missing: "+modules[n]);return;}
        QTest::qWait(350);
        auto page=find(settings,"sequoiaSettingsPage-"+pages[n]);
        if(!page || !find(settings,"sequoiaSettingsCard")) {fail("Settings page/card missing: "+modules[n]);return;}
        settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-"+modules[n]+".png");
        if(n==0) {
            // Exercise the real setting and native multi-select holiday model in a private profile.
            QQmlExpression enable(qmlContext(page),page,"Config.showHolidaysInCalendarViews = true; Config.save()");enable.evaluate();
            QTest::qWait(100);
            if(!click(settings,"sequoiaSettingsSelector")) {fail("Holiday selector missing");return;}
            QTest::qWait(200);
            if(!find(settings,"appearanceThemeOptions")) {fail("Holiday menu did not open");return;}
            settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-holidays.png");
            auto options=find(settings,"appearanceThemeOptions");
            QQmlExpression select(qmlContext(options),options,R"JS((function(){
                const row=itemAtIndex(1);
                if(!row) return false;
                const before=row.checked;
                row.toggle();row.toggled();
                return row.checked !== before;
            })())JS");
            if(!select.evaluate().toBool() || select.hasError()) {fail("Holiday region toggle failed");return;}
            QQmlExpression regions(qmlContext(page),page,"Config.holidayRegions.length > 0");
            if(!regions.evaluate().toBool()) {fail("Holiday region not saved to native config");return;}
            QTest::keyClick(settings,Qt::Key_Escape);
            QQuickItem *tasks=nullptr;
            for(auto item:items(page)) if(item->property("text").toString()=="Show tasks in calendar views") tasks=item;
            if(!tasks) {fail("Native task setting missing");return;}
            const bool before=tasks->property("checked").toBool();
            QTest::mouseClick(settings,Qt::LeftButton,Qt::NoModifier,tasks->mapToScene(QPointF(tasks->width()/2,tasks->height()/2)).toPoint());
            QQmlExpression taskSaved(qmlContext(page),page,"Config.showTodosInCalendarViews");
            if(taskSaved.evaluate().toBool()==before) {fail("Task setting no longer saves");return;}
            QQmlExpression scroll(qmlContext(page),page,"flickable.contentY = flickable.contentHeight - flickable.height");scroll.evaluate();
            QTest::qWait(200);
            settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-appearance-bottom.png");
        }
        if(n==1) {
            auto dialog=page->findChild<QObject*>("sequoiaAccountDialog");
            if(!dialog) {fail("Shared account dialog missing");return;}
            // A presentation-only fixture; never invoke edit/remove on a real account.
            QQmlExpression fixture(qmlContext(dialog),dialog,"agentDelegate = {name: 'Personal Calendar', index: -1}; open()");
            fixture.evaluate();QTest::qWait(250);
            if(fixture.hasError() || !dialog->property("visible").toBool()) {fail("Account popup fixture failed: "+fixture.error().toString());return;}
            auto cancel=find(settings,"sequoiaAccountCancel"),modify=find(settings,"sequoiaAccountModify"),remove=find(settings,"sequoiaAccountDelete");
            if(!cancel || !modify || !remove || qAbs(cancel->height()-modify->height())>1 || qAbs(cancel->height()-remove->height())>1) {fail("Account action pills missing or inconsistent");return;}
            settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-account-dialog.png");
            click(settings,"sequoiaAccountCancel");
            if(!QTest::qWaitFor([&]{return !dialog->property("visible").toBool();},1500)) {fail("Account Cancel did not close popup");return;}
            fixture.evaluate();QTest::qWait(400);
            QTest::keyClick(settings,Qt::Key_Escape);
            if(!QTest::qWaitFor([&]{return !dialog->property("visible").toBool();},1500) || !settings->isVisible()) {fail(QString("Account Escape: popup visible=%1, settings visible=%2").arg(dialog->property("visible").toBool()).arg(settings->isVisible()));return;}
            auto picker=page->findChild<QObject*>("sequoiaAddAccountDialog");
            if(!picker) {fail("Add Account glass picker missing");return;}
            QMetaObject::invokeMethod(picker,"open");QTest::qWait(400);
            auto sources=find(settings,"sequoiaAccountSources");
            if(!sources || sources->property("count").toInt()==0 || !find(settings,"sequoiaAddAccountClose")) {fail("Account source list or Close pill missing");return;}
            settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-add-account.png");
            click(settings,"sequoiaAddAccountClose");
            if(!QTest::qWaitFor([&]{return !picker->property("visible").toBool();},1500)) {fail("Add Account Close failed");return;}
            QMetaObject::invokeMethod(picker,"open");QTest::qWait(350);QTest::keyClick(settings,Qt::Key_Escape);
            if(!QTest::qWaitFor([&]{return !picker->property("visible").toBool();},1500)) {fail("Add Account Escape failed");return;}

            // Create disposable resources only inside this verifier's private
            // Akonadi/config/data scope. Never load a user's account or credentials.
            QQmlExpression agentsExpr(qmlContext(dialog),dialog,"root._configuration.availableAgents");
            auto agents=qobject_cast<QAbstractItemModel*>(agentsExpr.evaluate().value<QObject*>());
            if(!agents) {fail("Available account model missing");return;}
            for(const auto &type:QStringList{"Google","Birthdays","iCal Calendar File"}) {
                int row=-1;
                for(int i=0;i<agents->rowCount();++i) if(agents->data(agents->index(i,0)).toString().contains(type,Qt::CaseInsensitive)) {row=i;break;}
                if(row<0) {fail("Missing native account source: "+type);return;}
                QMetaObject::invokeMethod(picker,"open");QTest::qWait(350);
                QQmlExpression position(qmlContext(sources),sources,"currentIndex="+QString::number(row)+";positionViewAtIndex(currentIndex,ListView.Center)");position.evaluate();QTest::qWait(200);
                QQmlExpression rowExpr(qmlContext(sources),sources,"itemAtIndex("+QString::number(row)+")");
                auto sourceRow=qobject_cast<QQuickItem*>(rowExpr.evaluate().value<QObject*>());
                if(!sourceRow) {fail("Source picker row missing");return;}
                QTest::mouseClick(settings,Qt::LeftButton,Qt::NoModifier,sourceRow->mapToScene(QPointF(sourceRow->width()/2,sourceRow->height()/2)).toPoint());
                QPointer<QDialog> editor;
                if(!QTest::qWaitFor([&]{for(auto widget:QApplication::topLevelWidgets()) if(widget->isVisible() && widget->inherits("Akonadi::AgentConfigurationDialog")) {editor=qobject_cast<QDialog*>(widget);return true;}return false;},10000)) {fail("Native editor did not open: "+type);return;}
                QTest::qWait(450);
                auto surface=editor->findChild<QQuickItem*>("sequoiaNativeAccountBackground");
                auto buttons=editor->findChild<QDialogButtonBox*>("sequoiaNativeAccountButtons");
                if(!editor->property("sequoiaAccountStyled").toBool() || !surface || !buttons || !buttons->button(QDialogButtonBox::Cancel)) {fail("Native editor glass surface/buttons missing: "+type);return;}
                if(type=="Google") {
                    auto refresh=editor->findChild<QAbstractButton*>("enableRefresh");
                    if(!refresh) {fail("Native refresh checkbox missing");return;}
                    const bool wasChecked=refresh->isChecked();
                    QTest::mouseClick(refresh,Qt::LeftButton,Qt::NoModifier,QPoint(8,refresh->height()/2));
                    if(refresh->isChecked()==wasChecked) {fail("Panel blocks native checkbox input");return;}
                    QTest::mouseClick(refresh,Qt::LeftButton,Qt::NoModifier,QPoint(8,refresh->height()/2));
                }
                editor->grab().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-native-"+type.section(' ',0,0).toLower()+".png");
                if(type=="Google" && qEnvironmentVariableIsSet("SEQUOIA_MERKURO_PANEL_CAPTURE")) {
                    if(!captureAccountComposition("panel-painting-after")) {fail("Panel composition capture failed");return;}
                }
                const auto nativeActions=editor->findChildren<QQuickWidget*>("sequoiaNativeAccountActionHost");
                if(type=="Google" && qEnvironmentVariableIsSet("SEQUOIA_MERKURO_STACKING_COMPARE")) {
                    for(bool top:{false,true}) {
                        for(auto action:nativeActions) action->setAttribute(Qt::WA_AlwaysStackOnTop,top);
                        editor->update();QTest::qWait(350);
                        if(!captureAccountComposition(top?"button-stacking-after":"button-stacking-before")) {fail("Composited button comparison failed");return;}
                    }
                }
                for(auto action:nativeActions) {
                    if(action->status()!=QQuickWidget::Ready || !action->rootObject()) {fail("Native ActionPill failed to load");return;}
                    auto native=action->property("nativeButton").value<QAbstractButton*>();
                    if(!native || action->rootObject()->property("buttonEnabled").toBool()!=native->isEnabled()) {fail("Native action validation state not mirrored");return;}
                    if(type=="Google" && native->text().contains("Configure")) {
                        auto refresh=editor->findChild<QWidget*>("enableRefresh");
                        if(refresh && action->mapTo(editor,QPoint(0,action->height())).y()>refresh->mapTo(editor,QPoint()).y()) {fail("Account Configure pill overlaps the next field");return;}
                    }
                }
                auto scroll=editor->findChild<QScrollArea*>("sequoiaNativeAccountScroll");
                if(!scroll) {fail("Native form scrolling missing");return;}
                editor->resize(480,340);QTest::qWait(150);
                scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QTest::qWait(100);
                editor->grab().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-native-"+type.section(' ',0,0).toLower()+"-compact-bottom.png");
                if(picker->property("opened").toBool()) {fail("Source selection did not dismiss picker");return;}
                if(!activateAccountAction(editor,buttons->button(type=="Birthdays" ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),type=="Google")) {fail("Native footer pill missing");return;}
                if(!QTest::qWaitFor([&]{return !editor || !editor->isVisible();},2000)) {fail("Native account Cancel failed");return;}
                if(type=="Birthdays") {
                    QQmlExpression runningExpr(qmlContext(dialog),dialog,"root._configuration.runningAgents");
                    auto running=qobject_cast<QAbstractItemModel*>(runningExpr.evaluate().value<QObject*>());
                    int existing=-1;
                    if(!QTest::qWaitFor([&]{for(int i=0;running && i<running->rowCount();++i) if(running->data(running->index(i,0)).toString().contains("Birthdays")) {existing=i;return true;}return false;},3000)) {fail("Saved disposable birthday resource missing");return;}
                    QQmlExpression modifyFixture(qmlContext(dialog),dialog,"agentDelegate={name:'Birthdays & Anniversaries',index:"+QString::number(existing)+"};open()");modifyFixture.evaluate();QTest::qWait(350);
                    click(settings,"sequoiaAccountModify");
                    if(!QTest::qWaitFor([&]{for(auto widget:QApplication::topLevelWidgets()) if(widget->isVisible() && widget->inherits("Akonadi::AgentConfigurationDialog")) {editor=qobject_cast<QDialog*>(widget);return true;}return false;},3000)) {fail("Modify did not open native account editor");return;}
                    QTest::qWait(350);
                    buttons=editor->findChild<QDialogButtonBox*>("sequoiaNativeAccountButtons");
                    if(!buttons || !editor->findChild<QQuickItem*>("sequoiaNativeAccountBackground")) {fail("Modify editor makeover missing");return;}
                    if(!activateAccountAction(editor,buttons->button(QDialogButtonBox::Apply))) {fail("Modify Apply pill missing");return;}QTest::qWait(150);
                    if(!editor || !editor->isVisible()) {fail("Apply closed the native editor");return;}
                    editor->grab().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-modify-birthdays.png");
                    if(!activateAccountAction(editor,buttons->button(QDialogButtonBox::Cancel))) {fail("Modify Cancel pill missing");return;}
                    if(!QTest::qWaitFor([&]{return !editor || !editor->isVisible();},2000)) {fail("Modify Cancel failed");return;}
                    if(existing>=running->rowCount() || !running->data(running->index(existing,0)).toString().contains("Birthdays")) {fail("Modify Cancel removed the existing account");return;}
                    QQmlExpression reopen(qmlContext(dialog),dialog,"root._configuration.edit("+QString::number(existing)+")");reopen.evaluate();
                    if(!QTest::qWaitFor([&]{for(auto widget:QApplication::topLevelWidgets()) if(widget->isVisible() && widget->inherits("Akonadi::AgentConfigurationDialog")) {editor=qobject_cast<QDialog*>(widget);return true;}return false;},3000)) {fail("Modify editor did not reopen");return;}
                    QTest::keyClick(editor,Qt::Key_Return);
                    if(!QTest::qWaitFor([&]{return !editor || !editor->isVisible();},2000)) {fail("Native default action lost Return activation");return;}
                    QQmlExpression cleanup(qmlContext(dialog),dialog,"root._configuration.remove("+QString::number(existing)+")");cleanup.evaluate();
                }
            }
        }
        if(n==2) {
            QQuickItem *server=nullptr;
            for(auto item:items(page)) if(item->property("label").toString()=="Server URL") {server=item;break;}
            if(!server) {fail("Availability server input missing");return;}
            server->setProperty("text","https://example.invalid/freebusy");
            QMetaObject::invokeMethod(server,"editingFinished");
            QQmlExpression saved(qmlContext(page),page,"Calendar.CalendarSettings.freeBusyPublishUrl.toString()");
            if(saved.evaluate().toString()!="https://example.invalid/freebusy") {fail("Availability input did not save");return;}
            QQmlExpression scroll(qmlContext(page),page,"flickable.contentY = flickable.contentHeight - flickable.height");scroll.evaluate();
            QTest::qWait(200);
            settings->grabWindow().save(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY")+"/settings-availability-bottom.png");
        }
    }
    settings->resize(960,660);settings->setPosition(75,60);QTest::qWait(350);
    remembered.setValue("size",settings->size());remembered.setValue("position",settings->position());
    for(const auto &id:QStringList{"appearance","freebusy"}) {
        click(settings,"sequoiaSettingsNav-"+id);QTest::qWait(350);
        auto page=find(settings,"sequoiaSettingsPage-"+(id=="freebusy"?QString("FreeBusySettingsPage"):QString("ViewSettingsPage")));
        QQmlExpression scroll(qmlContext(page),page,"flickable.contentY = flickable.originY - flickable.topMargin + "+QString::number(id=="appearance"?450:280));scroll.evaluate();
        QTest::qWait(250);
        QQmlExpression offset(qmlContext(page),page,"flickable.contentY - flickable.originY + flickable.topMargin");
        remembered.setValue(id,offset.evaluate());
    }
    remembered.sync();
    QTest::keyClick(settings,Qt::Key_Escape);QTest::qWait(250);
    open.evaluate();QTest::qWait(900);
    settings=nullptr;
    for(auto candidate:QGuiApplication::allWindows())
        if(candidate->objectName()=="sequoiaMerkuroSettingsWindow" && candidate->isVisible()) settings=qobject_cast<QQuickWindow *>(candidate);
    if(!settings) {fail("Settings did not reopen");return;}
    if(!restored(settings)) return;
    QTest::keyClick(settings,Qt::Key_Escape);QTest::qWait(200);
    qInfo("MAKEOVER SETTINGS PASS");qInfo("MAKEOVER FINISHED");QCoreApplication::quit();
}
static void ready(int attempt=0) {
    auto w=window();
    if(!w) { if(attempt>=40) {fail("No application window after startup");return;} QTimer::singleShot(500,qApp,[attempt]{ready(attempt+1);});return; }
    if(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_SETTINGS_VERIFY")) {verifySettings(w);return;}
    w->resize(1660,1000);QTimer::singleShot(750,qApp,[]{verifySidebar();});
}
static void begin() {
    if(qEnvironmentVariable("SEQUOIA_MERKURO_VERIFY").isEmpty())return;
    if(QFileInfo(QCoreApplication::applicationFilePath()).fileName() != "merkuro-calendar") return;
    qunsetenv("LD_PRELOAD");
    QTimer::singleShot(3000,qApp,[]{ready();});
}
Q_COREAPP_STARTUP_FUNCTION(begin)
