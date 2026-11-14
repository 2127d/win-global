// SPDX-License-Identifier: MIT
// Included by the actual Settings app's isolated verification hook.
static void verifyAppearancePanels(QWidget *window, const std::function<void(const QString&)> &capture)
{
    require(qEnvironmentVariable("WAYLAND_DISPLAY")=="sequoia-settings-test", "Panel tests require private compositor");
    auto script=[](const QString &source) {
        auto message=QDBusMessage::createMethodCall("org.kde.plasmashell","/PlasmaShell","org.kde.PlasmaShell","evaluateScript");
        message << source;
        auto reply=QDBusConnection::sessionBus().call(message,QDBus::Block,5000);
        require(reply.type()!=QDBusMessage::ErrorMessage,"Plasma test script failed");
        return reply.arguments().first().toString().trimmed();
    };
    auto backend=qApp->findChild<QObject*>("panelSettings");
    if (!backend) for(auto widget:window->findChildren<QQuickWidget*>()) {
        backend=widget->engine()->rootContext()->contextProperty("panelSettings").value<QObject*>();
        if(backend) break;
    }
    require(backend,"Panel backend missing");
    auto settle=[&] { for(int i=0;i<300 && backend->property("busy").toBool();++i)QTest::qWait(20); require(!backend->property("busy").toBool(),"Panel operation timed out"); QTest::qWait(60); };
    auto refresh=[&] { settle();QMetaObject::invokeMethod(backend,"refresh");settle(); };
    auto find=[&](const QString &name) { for(auto widget:window->findChildren<QQuickWidget*>())if(widget->rootObject())for(auto item:items(widget->rootObject()))if(item->objectName()==name)return item;return static_cast<QQuickItem*>(nullptr); };
    script("panels().forEach(function(p){p.remove();}); var a=new Panel; a.location='top'; a.height=36; var b=new Panel; b.location='bottom'; b.height=60; b.floating=true; b.lengthMode='fit';");
    QTest::qWait(700);refresh();
    auto panels=backend->property("panels").toList();require(panels.size()==2,"Panel discovery did not find two fixtures");
    auto selector=find("appearancePanelSelector");auto grid=find("appearancePanelSettingsGrid");auto icons=find("appearanceComponentsGrid");
    require(selector && grid && icons,"Panel grid or selector missing");
    auto panelIsland=find("appearancePanelSettingsPanel");auto preview=find("appearanceDesktopFrame");auto options=find("appearanceOptionsRow");
    require(panelIsland && preview && options,"Preview or panel island missing");
    require(panelIsland->mapToScene(QPointF()).y()>=options->mapToScene(QPointF(0,options->height())).y(),"Panel settings must follow the desktop preview options");
    require(qAbs(panelIsland->mapToScene(QPointF()).x()-preview->mapToScene(QPointF()).x())<2,"Panel settings must align with the preview's left edge");
    auto selected=[&]{return backend->property("selected").toMap();};
    selector->forceActiveFocus();QTest::keyClick(selector->window(),Qt::Key_End);QTest::qWait(100);
    const int second=selected().value("id").toInt();require(second==panels.last().toMap().value("id").toInt(),"Panel dropdown did not select second panel");
    auto set=[&](QString key,QVariant value){require(QMetaObject::invokeMethod(backend,"setSetting",Q_ARG(QString,key),Q_ARG(QVariant,value)),"Missing panel setter");settle();require(backend->property("error").toString().isEmpty(),"Panel edit failed");};
    // Exercise actual grid keyboard editors, then verify the independent shell.
    auto location=find("setting-location");require(location,"Position editor missing");
    location->forceActiveFocus();QTest::keyClick(location->window(),Qt::Key_End);settle();
    require(selected().value("location")=="right","Grid position choice did not reach Plasma");
    require(script(QString("print(panelById(%1).location);").arg(second))=="right","Live panel position not changed");
    set("location","bottom");
    set("alignment","left");set("lengthMode","custom");set("minimumLength",240);set("maximumLength",480);set("offset",16);set("height",68);
    require(selected().value("height").toInt()==68 && selected().value("minimumLength").toInt()==240 && selected().value("maximumLength").toInt()==480,"Panel dimensions did not persist");
    for(const QString &value:{"none","autohide","dodgewindows","windowsgobelow"}){set("hiding",value);require(selected().value("hiding")==value,"Visibility round-trip failed");}
    for(const QString &value:{"disabled","applets","panel"}){set("floatingMode",value);require(selected().value("floatingMode")==value,"Floating round-trip failed");}
    if(backend->property("opacitySupported").toBool())for(const QString &value:{"opaque","translucent","adaptive"}){set("opacity",value);require(selected().value("opacity")==value,"Opacity round-trip failed");}
    // Invalid input must not reach the shell; queueing must target the captured ID.
    const auto before=script(QString("print(panelById(%1).height);").arg(second));
    set("height","not a number");set("location","invalid");
    require(script(QString("print(panelById(%1).height);").arg(second))==before,"Invalid input changed panel");
    require(QMetaObject::invokeMethod(backend,"setSetting",Q_ARG(QString,QString("height")),Q_ARG(QVariant,QVariant(72))),"Setter missing");
    backend->setProperty("selectedId",panels.first().toMap().value("id"));settle();
    require(script(QString("print(panelById(%1).height);").arg(second))=="72","Queued edit targeted wrong panel");
    require(selected().value("height").toInt()==36,"Editing second panel changed first panel");
    // A wide panel island lives in the main page scroll, below the preview.
    window->resize(1500,1000);QTest::qWait(300);
    require(grid->property("columns").toInt()>=4,"Settings must form a compact grid at desktop size");
    auto side=find("appearanceMainContent");require(side,"Main page scroller missing");
    auto flick=side->property("contentItem").value<QObject*>();require(flick,"Main page flickable missing");
    require(flick->property("contentHeight").toReal()>side->height(),"Preview and panel settings must scroll together");
    require(panelIsland->width()>panelIsland->height() && panelIsland->width()>=preview->width()-2,"Panel settings must be wide and match the preview width");
    capture("panel-settings-top");
    flick->setProperty("contentY",qMax(0.0,flick->property("contentHeight").toReal()-side->height()));QTest::qWait(200);capture("panel-settings-grid");
    window->showMaximized();QTest::qWait(350);
    for(const QString &name:{"appearanceComponentsPanel","appearanceAccentBar","appearanceIconPackCard"}) {
        auto card=find(name);require(card && qAbs(card->width()-card->implicitWidth())<1,"Appearance option did not fit its content");
        require(qAbs(card->mapToScene(QPointF()).x()-preview->mapToScene(QPointF()).x())<2,"Content-sized options lost left alignment");
    }
    require(panelIsland->width()<=1385 && selector->width()<=241,"Panel settings exceeded their width limits");
    flick->setProperty("contentY",qMax(0.0,flick->property("contentHeight").toReal()-side->height()));QTest::qWait(200);capture("appearance-options-fit-maximized");
    window->showNormal();QTest::qWait(250);window->resize(930,790);QTest::qWait(250);
    require(window->width()<=950,"Compact appearance fixture did not resize");
    for(const QString &name:{"appearanceComponentsPanel","appearanceAccentBar","appearanceIconPackCard","appearancePanelSettingsPanel"})require(find(name)->width()<=preview->width()+1,"Appearance options overflowed the narrow page");
    flick->setProperty("contentY",qMax(0.0,flick->property("contentHeight").toReal()-side->height()));QTest::qWait(150);capture("appearance-options-fit-compact");
    auto floating=find("setting-floatingMode");require(floating,"Floating selector missing");
    window->resize(1500,790);QTest::qWait(150);
    const qreal bottom=side->mapToScene(QPointF()).y()+side->height();
    flick->setProperty("contentY",flick->property("contentY").toReal()+floating->mapToScene(QPointF()).y()-(bottom-floating->height()-8));QTest::qWait(120);
    floating->forceActiveFocus();QTest::keyClick(floating->window(),Qt::Key_Space);QTest::qWait(200);
    auto popup=floating->property("popup").value<QObject*>();
    require(popup && popup->property("visible").toBool(),"Panel dropdown did not open");
    require(popup->property("width").toReal()>=240 && popup->property("y").toReal()<0,"Panel dropdown did not fit readable options above bottom-edge control");
    capture("panel-settings-menu");QTest::keyClick(floating->window(),Qt::Key_Escape);QTest::qWait(150);
    flick->setProperty("contentY",qMax(0.0,flick->property("contentHeight").toReal()-side->height()));QTest::qWait(150);
    floating->forceActiveFocus();QTest::keyClick(floating->window(),Qt::Key_Space);QTest::qWait(180);
    require(popup->property("visible").toBool() && popup->property("y").toReal()>0,"Dropdown reused stale placement after scrolling");
    QTest::keyClick(floating->window(),Qt::Key_Escape);
    auto hub=find("appearanceHub");require(hub,"Appearance hub missing");
    hub->setProperty("showComponents",false);QTest::qWait(100);
    require(panelIsland->isVisible() && backend->property("active").toBool(),"Hiding the icon panel must not disable panel settings");
    hub->setProperty("showComponents",true);QTest::qWait(100);
    // External changes and panel removal are reflected without reopening the hub.
    script(QString("panelById(%1).height=44;").arg(selected().value("id").toInt()));refresh();require(selected().value("height").toInt()==44,"External edit not reflected");
    script(QString("panelById(%1).remove();").arg(selected().value("id").toInt()));refresh();require(backend->property("panels").toList().size()==1 && selected().value("id").toInt()==second,"Removed panel selection not repaired");
    script("panels().forEach(function(p){p.remove();});");refresh();require(selected().isEmpty(),"Empty panel state not handled");
    fprintf(stderr,"PASS: panel settings grid, real Plasma edits, panel selection, bounds, visibility/floating round trips, external refresh and removal\n");
}
