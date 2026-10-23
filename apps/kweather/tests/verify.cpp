// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlExpression>
#include <QTimer>
#include <QTest>
#include <QDir>
#include <QNetworkProxy>
#include <cstdlib>
static void check(bool ok,const char *message) { if(!ok) {qCritical("FAIL: %s",message);std::_Exit(2);} }
static QVariant eval(QObject *object,const QString &code) {
    QQmlExpression e(qmlContext(object),object,code);auto result=e.evaluate();
    if(e.hasError()) qWarning()<<e.error();check(!e.hasError(),"QML expression");return result;
}
static QQuickItem *find(QQuickItem *item,const QString &name) {
    if(item->objectName()==name)return item;
    for(auto child:item->childItems())if(auto found=find(child,name))return found;
    return nullptr;
}
static void click(QQuickItem *item) {
    check(item && item->isVisible(),"click target visible");
    QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(200);
}
static void capture(QQuickWindow *w,const QString &name) {
    QTest::qWait(600);
    check(w->grabWindow().save(qEnvironmentVariable("WEATHER_CAPTURE")+"/"+name+".png"),"screenshot");
}
static void run(QQuickWindow *window,QQuickItem *dashboard) {
    QTest::qWait(700);
    if(qEnvironmentVariable("WEATHER_EMPTY")=="1") {
        check(eval(dashboard,"!hasWeather && !location.loading && location.errorMessage.length>0").toBool(),"empty/offline forecast has an honest error state");
        capture(window,"unavailable");
        qInfo("PASS: empty and offline forecast remains usable without invented weather");window->close();return;
    }
    check(eval(dashboard,"hasWeather && days.length===7 && location.hourForecasts.length===24").toBool(),"real weather model loaded fixture cache");
    check(eval(dashboard,"location.currentDate").toString()==QLocale().toString(QDateTime::currentDateTimeUtc().date(),QLocale::LongFormat),"header date uses the forecast location timezone");
    window->resize(1180,820);QTest::qWait(200);
    capture(window,"dynamic");
    auto temperature=find(dashboard,"weatherTemperature");check(temperature,"temperature label");
    auto before=temperature->property("text").toString();
    eval(dashboard,"settingsModel.temperatureUnits='Fahrenheit'");QTest::qWait(100);
    check(temperature->property("text").toString()!=before,"temperature unit conversion");
    eval(dashboard,"settingsModel.temperatureUnits='Celsius'");
    // Select a day using its actual button, then check that hourly and detail models follow.
    auto day=find(dashboard,"weatherDay2");
    auto scroll=find(dashboard,"weatherScroll");
    eval(scroll,"contentItem.contentY=400");QTest::qWait(200);click(day);
    check(eval(dashboard,"location.selectedDay===2 && selected.date.getDate()===days[2].date.getDate()").toBool(),"daily selection updates details");
    eval(scroll,"contentItem.contentY=0");QTest::qWait(200);
    click(find(dashboard,"weatherFlatStyle"));
    check(!dashboard->property("dynamic").toBool(),"flat style switch");
    capture(window,"flat");
    check(window->color().alpha()==(qEnvironmentVariableIntValue("WEATHER_ALPHA")<255?0:255),"flat window uses transparent clear color only for the transparent palette");
    check(qAbs(window->grabWindow().pixelColor(5,100).alpha()-qEnvironmentVariableIntValue("WEATHER_ALPHA"))<=1,"rendered flat margin preserves palette transparency");
    // Native settings and location management remain reachable.
    click(find(dashboard,"weatherSettings"));
    check(eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"native settings dialog opens");
    capture(window,"settings");
    eval(window,"settingsDialogLoader.item.close()");QTest::qWait(600);
    eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(200);
    check(eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"native location dialog opens");
    eval(window,"locationsListDialogLoader.item.close()");QTest::qWait(600);
    check(!eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"dialogs close");
    eval(dashboard,"switchPageIndex(1)");
    check(eval(dashboard,"location.name==='Monterey'").toBool(),"switch saved location");
    click(find(dashboard,"weatherDynamicStyle"));capture(window,"night");
    eval(dashboard,"settingsModel.animateWeather=false;settingsModel.save()");
    check(!eval(dashboard,"settingsModel.animateWeather").toBool(),"motion can be disabled");
    eval(dashboard,"switchPageIndex(0)");
    window->resize(760,760);capture(window,"compact");
    window->resize(390,760);capture(window,"narrow");
    check(!dashboard->property("sidebar").toBool(),"responsive sidebar");
    click(find(dashboard,"weatherRefresh"));
    QTest::qWait(300);
    check(eval(dashboard,"hasWeather && !location.loading && location.errorMessage.length===0").toBool(),"refresh completes without discarding forecast");
    check(qEnvironmentVariable("XDG_CACHE_HOME").startsWith("/tmp/sequoia-weather-test-"),"cache deletion restricted to test");
    QDir(qEnvironmentVariable("XDG_CACHE_HOME")+"/kweather/cache").removeRecursively();
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,"127.0.0.1",9));
    eval(dashboard,"location.update()");
    for(int i=0;i<50 && eval(dashboard,"location.loading").toBool();++i)QTest::qWait(100);
    check(eval(dashboard,"hasWeather && !location.loading && location.errorMessage.length>0").toBool(),"failed refresh preserves the last forecast");
    capture(window,"offline");
    qInfo("PASS: forecasts, units, day/location selection, styles, palette alpha, settings, location management, reduced motion, responsive layouts and refresh");
    window->close();
}
static void start() {
    qunsetenv("LD_PRELOAD");
    if(qEnvironmentVariable("WEATHER_EMPTY")=="1")QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,"127.0.0.1",9));
    auto timer=new QTimer(qApp);
    QObject::connect(timer,&QTimer::timeout,[timer]{
        for(auto w:QGuiApplication::allWindows())if(auto window=qobject_cast<QQuickWindow*>(w)) {
            if(auto dashboard=find(window->contentItem(),"sequoiaWeatherDashboard")) {
                timer->stop();run(window,dashboard);return;
            }
        }
    });timer->start(100);
    QTimer::singleShot(24000,qApp,[]{check(false,"weather test timed out");});
}
Q_COREAPP_STARTUP_FUNCTION(start)
