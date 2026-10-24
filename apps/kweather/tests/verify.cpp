// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlExpression>
#include <QTimer>
#include <QTest>
#include <QDir>
#include <QFile>
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
static QQuickItem *findInWindows(const QString &name) {
    const auto visibleItem=[](auto &&self,QQuickItem *item,const QString &target)->QQuickItem* {
        if(!item->isVisible())return nullptr;
        if(item->objectName()==target)return item;
        for(auto child:item->childItems())if(auto found=self(self,child,target))return found;
        return nullptr;
    };
    for(auto w:QGuiApplication::allWindows())if(auto window=qobject_cast<QQuickWindow*>(w))
        if(window->isVisible())if(auto item=visibleItem(visibleItem,window->contentItem(),name))return item;
    return nullptr;
}
static void capture(QQuickWindow *w,const QString &name) {
    QTest::qWait(600);
    check(w->grabWindow().save(qEnvironmentVariable("WEATHER_CAPTURE")+"/"+name+".png"),"screenshot");
}
static void checkForecastRows(QQuickItem *dashboard) {
    QTest::qWait(150);
    const int count=eval(dashboard,"days.length").toInt();
    for(int i=0;i<count;++i) {
        auto day=find(dashboard,"weatherDay"+QString::number(i));
        check(day,"daily row exists");
        check(eval(day,"modelData.minTemp")==eval(dashboard,QString("location.dayForecasts[%1].minTemp").arg(i)),"daily low follows active forecast");
        check(eval(day,"modelData.maxTemp")==eval(dashboard,QString("location.dayForecasts[%1].maxTemp").arg(i)),"daily high follows active forecast");
        auto bar=find(dashboard,"weatherTemperatureBar"+QString::number(i));
        check(bar && bar->width()>0 && bar->x()>=0 && bar->x()+bar->width()<=bar->parentItem()->width()+0.01,"temperature bar stays inside its track");
    }
    auto hour=find(dashboard,"weatherHour0");
    check(hour && eval(hour,"modelData.temperature")==eval(dashboard,"location.hourForecasts[0].temperature"),"hourly row follows active day and location");
    check(eval(dashboard,"selected.maxTemp===location.dayForecasts[location.selectedDay].maxTemp && selected.weatherDescription===location.dayForecasts[location.selectedDay].weatherDescription").toBool(),"selected-day details follow active forecast");
}
static void runWelcome(QQuickWindow *window,QQuickItem *welcome) {
    QTest::qWait(600);
    check(eval(welcome,"WeatherLocationListModel.count===0 && !settingsModel.firstStartup").toBool(),"first launch uses the welcome screen");
    window->resize(680,580);capture(window,"welcome-dynamic");
    click(find(welcome,"weatherWelcomeAddLocation"));
    check(eval(welcome,"applicationWindow().isDialogOpen").toBool(),"welcome action opens native location search");
    capture(window,"welcome-search");
    if(qEnvironmentVariable("WEATHER_RENDERED")=="1")
        check(eval(welcome,"applicationWindow().pageStack.layer.enabled").toBool(),"rendered search uses backdrop blur");
    // Inject only a test list so overflowing result styling is deterministic and
    // does not depend on an external geocoder or change the user's locations.
    auto search=findInWindows("weatherLocationSearch");check(search,"location search list");
    eval(search,"var fixture=Qt.createQmlObject('import QtQml.Models; ListModel { property bool loading: false; property bool networkError: false; function textChanged(text, delay) {} }',this); for(var i=0;i<20;++i) fixture.append({name: i===0 ? 'Wendover Airport, UT | United States' : 'Wendover test result '+i}); model=fixture");
    eval(findInWindows("weatherLocationSearchField"),"text='Wendover';forceActiveFocus()");
    QTest::mouseMove(window,QPoint(20,20));QTest::qWait(200);
    auto firstResult=findInWindows("weatherLocationResult0");check(firstResult,"search fixture result visible");
    capture(window,"search-results");
    check(eval(firstResult,"background.color.a===0 || hovered || mouseArea.pressed || activeFocus || (ListView.isCurrentItem && ListView.view.activeFocus)").toBool(),"search rows only highlight during interaction");
    auto card=findInWindows("weatherSearchResultsCard");
    auto field=findInWindows("weatherLocationSearchField");
    auto searchButton=findInWindows("weatherLocationSearchButton");
    check(card && field && searchButton,"search card and controls are visible");
    check(qAbs(card->mapToScene(QPointF(card->width(),0)).x()-searchButton->mapToScene(QPointF(searchButton->width(),0)).x())<1,"shared results card extends to the search button's right edge");
    check(qAbs(card->mapToScene(QPointF()).x()-field->mapToScene(QPointF()).x())<1,"shared results card aligns with search field");
    check(card->property("radius")==eval(field,"background.radius"),"shared results card matches search field radius");
    auto scrollBar=findInWindows("weatherSearchResultsScrollBar");check(scrollBar,"overflowing search results have a scrollbar");
    check(firstResult->mapToScene(QPointF(firstResult->width(),0)).x()+5<=scrollBar->mapToScene(QPointF()).x(),"search result highlight leaves a gap before the scrollbar");
    QTest::mouseMove(window,firstResult->mapToScene(QPointF(firstResult->width()/2,firstResult->height()/2)).toPoint());
    capture(window,"search-result-hover");
    auto searchPosition=field->mapToScene(QPointF());
    eval(search,"positionViewAtEnd()");capture(window,"search-results-bottom");
    check(field->mapToScene(QPointF())==searchPosition,"search field stays fixed while results scroll");
    auto clearButton=findInWindows("weatherClearLocationSearch");
    check(clearButton,"clear search button is visible for entered text");
    auto clearPosition=clearButton->mapToItem(field,QPointF());
    check(clearPosition.x()>=8 && clearPosition.y()>=8 && clearPosition.x()+clearButton->width()<=field->width()-8 && clearPosition.y()+clearButton->height()<=field->height()-8,"clear highlight has space inside every search field edge");
    QTest::mouseMove(window,clearButton->mapToScene(QPointF(clearButton->width()/2,clearButton->height()/2)).toPoint());
    capture(window,"search-clear-hover");
    click(clearButton);
    check(field->property("text").toString().isEmpty() && field->hasActiveFocus() && !clearButton->isVisible(),"clear search empties text and restores input focus");
    click(findInWindows("weatherCloseLocationSearch"));QTest::qWait(600);
    check(!eval(welcome,"applicationWindow().isDialogOpen").toBool(),"circular search close button closes the dialog");
    check(!eval(welcome,"applicationWindow().pageStack.layer.enabled").toBool(),"closing search releases the blur layer");
    click(find(welcome,"weatherWelcomeSettings"));
    check(eval(welcome,"applicationWindow().isDialogOpen").toBool(),"welcome settings remain accessible");
    eval(window,"settingsDialogLoader.item.close()");QTest::qWait(600);
    eval(welcome,"settingsModel.forecastStyle='Flat';settingsModel.save()");
    capture(window,"welcome-flat");
    window->resize(390,480);capture(window,"welcome-narrow");
    auto button=find(welcome,"weatherWelcomeAddLocation");
    auto pos=button->mapToScene(QPointF(0,0));
    check(pos.x()>=0 && pos.y()>=0 && pos.x()+button->width()<=window->width() && pos.y()+button->height()<=window->height(),"welcome action fits the smallest window");
    click(button);
    check(eval(welcome,"applicationWindow().pageStack.depth>1").toBool(),"compact welcome opens location search page");
    qInfo("PASS: first-launch welcome, wide and compact location search, settings and both welcome styles");
    window->close();
}
static void runCompact(QQuickWindow *window,QQuickItem *dashboard) {
    const auto fits=[window](QQuickItem *item) {
        check(item,"compact control exists");
        const auto p=item->mapToScene(QPointF());
        check(p.x()>=0 && p.y()>=0 && p.x()+item->width()<=window->width()+1 && p.y()+item->height()<=window->height()+1,"compact control fits within the window");
    };
    window->resize(390,480);QTest::qWait(500);
    eval(dashboard,"applicationWindow().openSettings()");QTest::qWait(300);
    check(window->property("sequoiaModalAmount").toDouble()==0,"compact pages do not dim the decoration");
    fits(findInWindows("weatherCompactBack"));
    fits(findInWindows("weatherOpenAbout"));
    if(qEnvironmentVariable("WEATHER_RENDERED")=="1")
        check(eval(window,"background.layer.enabled && !pageStack.layer.enabled").toBool()==(qEnvironmentVariableIntValue("WEATHER_ALPHA")<255),"compact transparent theme blurs scenery while page controls stay sharp");
    capture(window,"settings");
    click(findInWindows("weatherOpenAbout"));QTest::qWait(300);
    auto about=findInWindows("weatherAboutView");check(about,"compact About is reachable");
    capture(window,"about");
    eval(about,"contentItem.contentY=contentItem.contentHeight-contentItem.height");capture(window,"about-bottom");
    click(findInWindows("weatherCompactBack"));QTest::qWait(600);
    check(findInWindows("weatherAnimateScenery"),"compact About returns to Settings");
    eval(window,"pageStack.currentItem.flickable.contentY=pageStack.currentItem.flickable.contentHeight-pageStack.currentItem.flickable.height");
    capture(window,"settings-bottom");
    fits(findInWindows("weatherPrecipitationSetting"));
    click(findInWindows("weatherCompactBack"));QTest::qWait(600);
    eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(300);
    capture(window,"locations");
    auto add=findInWindows("weatherManageAddLocation");check(add,"compact management has an Add Location button");
    fits(add);
    fits(findInWindows("weatherDeleteLocation1"));
    check(qAbs(add->mapToScene(QPointF(add->width()/2,0)).x()-window->width()/2)<1,"compact Add Location button is centered");
    const auto addPosition=add->mapToScene(QPointF());
    auto locations=findInWindows("weatherManagedLocations");
    eval(locations,"positionViewAtEnd()");capture(window,"locations-bottom");
    fits(findInWindows("weatherDeleteLocation7"));
    check(add->mapToScene(QPointF())==addPosition,"compact Add Location stays pinned while saved locations scroll");
    eval(locations,"positionViewAtBeginning()");QTest::qWait(200);
    click(add);QTest::qWait(300);
    capture(window,"search");
    auto search=findInWindows("weatherLocationSearch");check(search,"compact location search visible");
    eval(search,"var fixture=Qt.createQmlObject('import QtQml.Models; ListModel { property bool loading: false; property bool networkError: false; function textChanged(text, delay) {} }',this); for(var i=0;i<20;++i) fixture.append({name:'Wendover Airport, UT | United States — result '+i}); model=fixture");
    auto field=findInWindows("weatherLocationSearchField");check(field,"compact search field visible");
    fits(field);
    fits(findInWindows("weatherLocationSearchButton"));
    eval(field,"text='Wendover';forceActiveFocus()");
    capture(window,"search-results");
    auto position=field->mapToScene(QPointF());
    eval(search,"positionViewAtEnd()");capture(window,"search-bottom");
    check(field->mapToScene(QPointF())==position,"compact search stays pinned while results scroll");
    click(findInWindows("weatherClearLocationSearch"));
    check(field->property("text").toString().isEmpty(),"compact clear search works");
    window->resize(540,480);QTest::qWait(300);capture(window,"search-compact-wide");
    fits(field);
    window->resize(390,480);QTest::qWait(300);
    click(findInWindows("weatherCompactBack"));QTest::qWait(600);
    check(findInWindows("weatherManagedLocations"),"compact search returns to location management");
    click(findInWindows("weatherManagedLocation1"));QTest::qWait(300);
    check(eval(dashboard,"location.name==='Monterey'").toBool(),"compact management selects a saved location");
    check(!eval(window,"background.layer.enabled || pageStack.layer.enabled").toBool(),"returning to the forecast releases compact backdrop blur");
    qInfo("PASS: compact Settings, About, location management, search, scrolling and navigation");
    window->close();
}
static void runChoices(QQuickWindow *window,QQuickItem *dashboard) {
    const bool compact=qEnvironmentVariable("WEATHER_COMPACT")=="1";
    window->resize(compact?390:1180,compact?480:820);QTest::qWait(400);
    eval(dashboard,"applicationWindow().openSettings()");QTest::qWait(300);
    const auto geometry=compact?QVariant():eval(window,"[settingsDialogLoader.item.x,settingsDialogLoader.item.y,settingsDialogLoader.item.width,settingsDialogLoader.item.height].join(',')");
    const auto reveal=[](QQuickItem *item) {
        eval(item,"parent.ensureVisible()");
        QTest::qWait(300);
    };
    struct Choice { const char *name; const char *key; const char *value; int index; };
    const Choice cases[]={{"weatherForecastStyleSetting","forecastStyle","Flat",0},{"weatherTemperatureSetting","temperatureUnits","Fahrenheit",2},{"weatherSpeedSetting","speedUnits","mph",1},{"weatherPressureSetting","pressureUnits","mmHg",1},{"weatherPrecipitationSetting","precipitationUnits","in",1}};
    for(const auto &c:cases) {
        auto row=findInWindows(c.name);check(row,"inline settings row exists");
        auto toggle=findInWindows(QString(c.name)+"Toggle");check(toggle,"inline choice toggle exists");
        const double collapsed=row->height();reveal(toggle);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,toggle->mapToScene(QPointF(toggle->width()/2,toggle->height()/2)).toPoint());
        for(int frame=0;frame<20;++frame) {
            QTest::qWait(20);
            if(!compact)check(eval(window,"[settingsDialogLoader.item.x,settingsDialogLoader.item.y,settingsDialogLoader.item.width,settingsDialogLoader.item.height].join(',')")==geometry,"popup stays steady throughout inline expansion");
        }
        check(row->property("expanded").toBool() && row->height()>collapsed+60,"settings choices expand their card inline");
        if(!compact)check(eval(window,"[settingsDialogLoader.item.x,settingsDialogLoader.item.y,settingsDialogLoader.item.width,settingsDialogLoader.item.height].join(',')")==geometry,"opening a choice keeps popup geometry stable");
        auto option=findInWindows(QString(c.name)+"Option"+QString::number(c.index));check(option,"requested choice is visible");
        auto point=option->mapToScene(QPointF());
        check(point.y()>=0 && point.y()+option->height()<=window->height(),"expanded choice is reachable in the window");
        capture(window,QString("choice-")+c.key);
        click(option);QTest::qWait(500);
        check(eval(dashboard,QString("settingsModel.")+c.key).toString()==c.value,"choice updates the real setting");
        check(!row->property("expanded").toBool() && row->property("currentIndex").toInt()==c.index,"selection collapses and shows the chosen value");
    }
    QFile config(qEnvironmentVariable("XDG_CONFIG_HOME")+"/kweather/kweatherrc");check(config.open(QIODevice::ReadOnly),"saved settings are readable");
    const auto saved=config.readAll();
    for(const auto &c:cases)check(saved.contains(QByteArray(c.key)+"="+c.value),"inline choice is persisted on disk");
    auto temperature=findInWindows("weatherTemperatureSetting");
    auto toggle=findInWindows("weatherTemperatureSettingToggle");reveal(toggle);toggle->forceActiveFocus();
    QTest::keyClick(window,Qt::Key_Down);QTest::qWait(400);
    QTest::keyClick(window,Qt::Key_Up);QTest::keyClick(window,Qt::Key_Return);QTest::qWait(300);
    check(eval(dashboard,"settingsModel.temperatureUnits==='Celsius'").toBool() && !temperature->property("expanded").toBool(),"keyboard arrows and Enter select a unit");
    QTest::keyClick(window,Qt::Key_Down);QTest::qWait(300);QTest::keyClick(window,Qt::Key_Escape);QTest::qWait(300);
    check(!temperature->property("expanded").toBool() && findInWindows("weatherOpenAbout"),"Escape collapses choices without leaving Settings");
    eval(temperature,"expanded=true");QTest::qWait(300);
    auto speed=findInWindows("weatherSpeedSetting");eval(speed,"expanded=true");QTest::qWait(300);
    check(!temperature->property("expanded").toBool() && speed->property("expanded").toBool(),"only one selector expands at a time");
    qInfo("PASS: inline Settings choices, stable popup, compact reachability, saved values and keyboard navigation");
    window->close();
}
static void run(QQuickWindow *window,QQuickItem *dashboard) {
    QTest::qWait(700);
    if(qEnvironmentVariable("WEATHER_CHOICES")=="1") {runChoices(window,dashboard);return;}
    if(qEnvironmentVariable("WEATHER_COMPACT")=="1") {runCompact(window,dashboard);return;}
    if(qEnvironmentVariable("WEATHER_EMPTY")=="1") {
        check(eval(dashboard,"!hasWeather && !location.loading && location.errorMessage.length>0").toBool(),"empty/offline forecast has an honest error state");
        capture(window,"unavailable");
        qInfo("PASS: empty and offline forecast remains usable without invented weather");window->close();return;
    }
    check(eval(dashboard,"hasWeather && days.length===7 && location.hourForecasts.length===24").toBool(),"real weather model loaded fixture cache");
    check(eval(dashboard,"location.currentDate").toString()==QLocale().toString(QDateTime::currentDateTimeUtc().date(),QLocale::LongFormat),"header date uses the forecast location timezone");
    window->resize(1180,820);QTest::qWait(200);
    checkForecastRows(dashboard);
    capture(window,"dynamic");
    // Exercise shader-backed dialogs directly on Wayland. The full forecast
    // interaction suite below uses the deterministic offscreen renderer.
    if(qEnvironmentVariable("WEATHER_RENDERED")=="1") {
        qInfo("Checking Settings and About navigation");
        eval(dashboard,"applicationWindow().openSettings()");QTest::qWait(300);
        check(qAbs(window->property("sequoiaModalAmount").toDouble()-.3)<.001,"popup reports its decoration dim amount");
        const bool transparent=qEnvironmentVariableIntValue("WEATHER_ALPHA")<255;
        check(eval(window,"pageStack.layer.enabled && background.layer.enabled").toBool()==transparent,"settings backdrop blur follows theme transparency");
        check(qAbs(eval(window,"settingsDialogLoader.item.background.color.a").toDouble()*255-qEnvironmentVariableIntValue("WEATHER_ALPHA"))<=1,"settings surface uses the theme opacity");
        capture(window,"settings");
        const auto settingsGeometry=eval(window,"[settingsDialogLoader.item.x,settingsDialogLoader.item.y,settingsDialogLoader.item.width,settingsDialogLoader.item.height].join(',')");
        const auto desktopDepth=eval(window,"pageStack.depth");
        auto motion=findInWindows("weatherAnimateScenery");check(motion,"settings switch visible before About");
        const auto motionBefore=motion->property("checked");
        click(motion);
        qInfo("Opening About");
        click(findInWindows("weatherOpenAbout"));QTest::qWait(500);
        auto about=findInWindows("weatherAboutView");check(about,"custom About view opens inside settings");
        check(eval(window,"pageStack.depth")==desktopDepth,"About does not push a desktop page");
        check(eval(window,"[settingsDialogLoader.item.x,settingsDialogLoader.item.y,settingsDialogLoader.item.width,settingsDialogLoader.item.height].join(',')")==settingsGeometry,"About retains settings popup geometry");
        check(eval(about,"aboutData.version.length > 0 && aboutData.authors.length > 0 && aboutData.licenses.length > 0").toBool(),"About preserves real version, credits and license data");
        check(eval(window,"pageStack.layer.enabled").toBool()==transparent,"About retains theme-aware backdrop blur");
        capture(window,"about");
        auto license=findInWindows("weatherAboutLicense");check(license,"application license is available");
        eval(about,QString("contentItem.contentY += %1").arg(license->mapToItem(about,QPointF()).y()-80));QTest::qWait(200);
        click(license);
        check(license->property("checked").toBool(),"license text expands inside About");
        capture(window,"about-license");
        click(license);
        eval(about,"contentItem.contentY=contentItem.contentHeight-contentItem.height");capture(window,"about-details");
        qInfo("Returning from About");
        click(findInWindows("weatherAboutBack"));QTest::qWait(500);
        check(findInWindows("weatherOpenAbout") && motion->property("checked")!=motionBefore,"Back restores Settings and preserves edits");
        click(motion);
        click(findInWindows("weatherOpenAbout"));QTest::qWait(500);
        click(findInWindows("weatherCloseSettings"));QTest::qWait(600);
        check(!eval(window,"settingsDialogLoader.isOpen").toBool(),"circular settings close button closes the dialog");
        check(window->property("sequoiaModalAmount").toDouble()==0,"closing popup releases decoration dimming");
        check(!eval(window,"pageStack.layer.enabled || background.layer.enabled").toBool(),"closing settings releases backdrop blur");
        eval(dashboard,"applicationWindow().openSettings()");QTest::qWait(300);
        check(findInWindows("weatherOpenAbout") && !findInWindows("weatherAboutBack"),"reopening settings starts at Settings");
        click(findInWindows("weatherCloseSettings"));QTest::qWait(600);
        qInfo("Checking location dialogs after About");
        eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(300);
        check(eval(window,"locationsListDialogLoader.item.background.color.a < 0.9 && pageStack.layer.enabled && background.layer.enabled").toBool(),"rendered management dialog has translucency and backdrop blur");
        capture(window,"locations");
        click(findInWindows("weatherManageAddLocation"));QTest::qWait(400);
        check(findInWindows("weatherLocationSearchField") && eval(window,"pageStack.layer.enabled").toBool(),"switching from management to search retains blur");
        click(findInWindows("weatherCloseLocationSearch"));QTest::qWait(600);
        check(!eval(window,"pageStack.layer.enabled || background.layer.enabled").toBool(),"closing location dialogs releases all blur layers");
        qInfo("PASS: rendered theme-aware settings, location management translucency, blur, search transition and close");
        window->close();return;
    }
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
    checkForecastRows(dashboard);
    eval(scroll,"contentItem.contentY=0");QTest::qWait(200);
    click(find(dashboard,"weatherFlatStyle"));
    check(!dashboard->property("dynamic").toBool(),"flat style switch");
    capture(window,"flat");
    check(window->color().alpha()==(qEnvironmentVariableIntValue("WEATHER_ALPHA")<255?0:255),"flat window uses transparent clear color only for the transparent palette");
    check(qAbs(window->grabWindow().pixelColor(5,100).alpha()-qEnvironmentVariableIntValue("WEATHER_ALPHA"))<=1,"rendered flat margin preserves palette transparency");
    // Native settings and location management remain reachable.
    click(find(dashboard,"weatherSettings"));
    check(eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"native settings dialog opens");
    check(qAbs(eval(window,"settingsDialogLoader.item.background.color.a").toDouble()*255-qEnvironmentVariableIntValue("WEATHER_ALPHA"))<=1,"settings surface matches theme opacity");
    auto motion=findInWindows("weatherAnimateScenery");
    check(motion,"settings motion switch visible");
    check(find(motion,"sequoiaSwitchTrack"),"settings uses the shared Sequoia switch indicator");
    capture(motion->window(),"settings");
    click(motion);
    check(!eval(dashboard,"settingsModel.animateWeather").toBool(),"settings switch saves mouse changes");
    motion->forceActiveFocus();QTest::keyClick(motion->window(),Qt::Key_Space);QTest::qWait(200);
    check(eval(dashboard,"settingsModel.animateWeather").toBool(),"settings switch saves keyboard changes");
    eval(window,"settingsDialogLoader.item.close()");QTest::qWait(600);
    eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(200);
    check(eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"native location dialog opens");
    check(eval(window,"locationsListDialogLoader.item.background.color.a < 0.9").toBool(),"management dialog uses a translucent surface");
    if(qEnvironmentVariable("WEATHER_RENDERED")=="1")
        check(eval(window,"pageStack.layer.enabled && background.layer.enabled").toBool(),"management dialog blurs the forecast and scenery");
    auto managed=findInWindows("weatherManagedLocations");check(managed,"location management list visible");
    auto addLocation=findInWindows("weatherManageAddLocation");check(addLocation,"management add location button visible");
    addLocation->forceActiveFocus();
    capture(managed->window(),"locations");
    click(addLocation);QTest::qWait(400);
    check(findInWindows("weatherLocationSearchField"),"management add button opens location search");
    click(findInWindows("weatherCloseLocationSearch"));QTest::qWait(600);
    eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(200);
    click(findInWindows("weatherManagedLocation1"));QTest::qWait(600);
    check(!eval(dashboard,"applicationWindow().isDialogOpen").toBool(),"dialogs close");
    check(!eval(window,"pageStack.layer.enabled && background.layer.enabled").toBool(),"closing location management releases backdrop blur");
    check(eval(dashboard,"location.name==='Monterey'").toBool(),"management row selects its location");
    eval(dashboard,"switchPageIndex(1)");
    check(eval(dashboard,"location.name==='Monterey'").toBool(),"switch saved location");
    check(eval(find(dashboard,"weatherDay0"),"modelData.minTemp").toDouble()==29,"daily rows switch to the new location's temperatures");
    checkForecastRows(dashboard);
    click(find(dashboard,"weatherDynamicStyle"));capture(window,"night");
    eval(dashboard,"settingsModel.animateWeather=false;settingsModel.save()");
    check(!eval(dashboard,"settingsModel.animateWeather").toBool(),"motion can be disabled");
    eval(dashboard,"switchPageIndex(0)");
    checkForecastRows(dashboard);
    window->resize(760,760);capture(window,"compact");
    checkForecastRows(dashboard);
    window->resize(390,760);capture(window,"narrow");
    checkForecastRows(dashboard);
    check(!dashboard->property("sidebar").toBool(),"responsive sidebar");
    eval(dashboard,"applicationWindow().openSettings()");QTest::qWait(250);
    auto compactMotion=find(window->contentItem(),"weatherAnimateScenery");
    check(compactMotion,"compact settings page has the Sequoia switch");
    check(eval(compactMotion,"height >= contentItem.implicitHeight + topPadding + bottomPadding - 1").toBool(),"compact motion description fits without overlapping the next setting");
    capture(window,"settings-narrow");
    click(findInWindows("weatherOpenAbout"));QTest::qWait(300);
    check(findInWindows("weatherAboutView"),"compact settings opens the same custom About content");
    capture(window,"about-narrow");
    eval(dashboard,"applicationWindow().pageStack.pop()");QTest::qWait(250);
    eval(dashboard,"applicationWindow().pageStack.pop()");QTest::qWait(250);
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
    window->resize(1180,820);QTest::qWait(200);
    eval(dashboard,"applicationWindow().openLocationsList()");QTest::qWait(250);
    click(findInWindows("weatherDeleteLocation0"));
    check(eval(dashboard,"WeatherLocationListModel.count===1").toBool(),"management delete removes one location");
    click(findInWindows("weatherDeleteLocation0"));
    QTest::qWait(200);
    auto welcome=find(window->contentItem(),"sequoiaWeatherWelcome");
    check(welcome && welcome->isVisible(),"removing the last location returns to welcome");
    qInfo("PASS: forecasts, units, day/location selection, temperature-bar bounds, styles, palette alpha, settings, location management, reduced motion, responsive layouts and refresh");
    window->close();
}
static void start() {
    qunsetenv("LD_PRELOAD");
    if(qEnvironmentVariable("WEATHER_EMPTY")=="1")QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,"127.0.0.1",9));
    auto timer=new QTimer(qApp);
    QObject::connect(timer,&QTimer::timeout,[timer]{
        for(auto w:QGuiApplication::allWindows())if(auto window=qobject_cast<QQuickWindow*>(w)) {
            if(qEnvironmentVariable("WEATHER_WELCOME")=="1") {
                if(auto welcome=find(window->contentItem(),"sequoiaWeatherWelcome")) {
                    timer->stop();runWelcome(window,welcome);return;
                }
                continue;
            }
            if(auto dashboard=find(window->contentItem(),"sequoiaWeatherDashboard")) {
                timer->stop();run(window,dashboard);return;
            }
        }
    });timer->start(100);
    QTimer::singleShot(24000,qApp,[]{check(false,"weather test timed out");});
}
Q_COREAPP_STARTUP_FUNCTION(start)
