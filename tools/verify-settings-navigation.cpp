// SPDX-License-Identifier: MIT
#include <QQuickPaintedItem>
#include <QSignalSpy>
#include <QAbstractItemModel>
// Test-only startup hook for the real application, on a private session bus.
#include <QBitArray>
#include <QApplication>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QQmlContext>
#include <functional>
#include <QTest>
#include <QTimer>
#include <QFile>
#include <QDirIterator>
#include <KConfig>
#include <KConfigGroup>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDialog>
#include <QAbstractButton>
#include <QSet>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusVirtualObject>
#include <QDBusPendingCall>
#include <QDBusUnixFileDescriptor>
#include <QTemporaryFile>
#include <QPainter>
#include <QScreen>
#include <KWindowEffects>
#include <Plasma/Theme>
#include <QProcess>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

// Observe the actual region sent to the compositor, including updates from the
// widget style, rather than checking only the preview controller's own state.
namespace KWindowEffects {
void enableBlurBehind(QWindow *window, bool enable, const QRegion &region)
{
    using EnableBlur = void (*)(QWindow *, bool, const QRegion &);
    static const auto real = reinterpret_cast<EnableBlur>(dlsym(RTLD_NEXT, "_ZN14KWindowEffects16enableBlurBehindEP7QWindowbRK7QRegion"));
    if (!real) std::abort();
    if (window) {
        window->setProperty("verifiedBlurEnabled", enable);
        window->setProperty("verifiedBlurRegion", QVariant::fromValue(region));
    }
    real(window, enable, region);
}
}

static void require(bool condition, const char *message)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static QList<QQuickItem *> items(QQuickItem *root)
{
    QList<QQuickItem *> list{root};
    for (int i = 0; i < list.size(); ++i) list.append(list[i]->childItems());
    return list;
}
static bool call(QObject *nav, const char *method, const QString &value)
{
    bool ok = false;
    require(QMetaObject::invokeMethod(nav, method, Qt::DirectConnection, Q_RETURN_ARG(bool, ok), Q_ARG(QString, value)), "Missing navigation method");
    QTest::qWait(100);
    return ok;
}
static bool back(QObject *nav)
{
    bool ok = false;
    require(QMetaObject::invokeMethod(nav, "back", Qt::DirectConnection, Q_RETURN_ARG(bool, ok)), "Missing back method");
    QTest::qWait(100);
    return ok;
}
static void open(QObject *nav, const QString &id)
{
    require(QMetaObject::invokeMethod(nav, "open", Qt::DirectConnection, Q_ARG(QString, id)), "Missing open method");
    QTest::qWait(600);
}
#include "verify-appearance-panels.h"
class HubWindowProbe : public QDBusVirtualObject {
public:
    bool received=false;
    QString introspect(const QString &) const override { return {}; }
    bool handleMessage(const QDBusMessage &message,const QDBusConnection &connection) override {
        fprintf(stderr,"HUB_COMPOSITOR_WINDOWS %s\n",qPrintable(message.arguments().first().toString()));
        received=true;connection.send(message.createReply());return true;
    }
};
class HubBackdrop : public QWidget {
public:
    HubBackdrop() : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::WindowDoesNotAcceptFocus) { setWindowTitle(QStringLiteral("Hub Test Backdrop")); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        QImage wallpaper(qEnvironmentVariable("SEQUOIA_HUB_WALLPAPER"));
        if (!wallpaper.isNull()) painter.drawImage(rect(), wallpaper);
        else {
            QLinearGradient gradient(0,0,width(),height()); gradient.setColorAt(0,QColor("#76b7a2")); gradient.setColorAt(1,QColor("#61a4c6")); painter.fillRect(rect(),gradient);
            painter.setPen(QColor("#aaffffff")); for(int x=0;x<width();x+=24) painter.drawLine(x,0,x,height());
        }
    }
};
static QImage composedCapture(const QString &name) {
    QTemporaryFile file; require(file.open(), "Cannot create screenshot buffer");
    auto message=QDBusMessage::createMethodCall("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2", "org.kde.KWin.ScreenShot2", "CaptureWorkspace");
    message << QVariantMap{{"hide-caller-windows",false},{"include-cursor",false}} << QVariant::fromValue(QDBusUnixFileDescriptor(file.handle()));
    const auto reply=QDBusConnection::sessionBus().call(message,QDBus::Block,5000);
    require(reply.type()!=QDBusMessage::ErrorMessage,"Compositor capture failed");
    const auto map=qdbus_cast<QVariantMap>(reply.arguments().first());
    const auto bytes=map["height"].toInt()*map["stride"].toInt();
    for(int i=0;file.size()<bytes && i<200;++i) QTest::qWait(10);
    file.seek(0); const auto data=file.readAll(); require(data.size()==bytes,"Incomplete screenshot");
    QImage image(reinterpret_cast<const uchar*>(data.constData()),map["width"].toInt(),map["height"].toInt(),map["stride"].toInt(),QImage::Format(map["format"].toInt()));
    auto copy=image.copy(); require(copy.save(qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY")+'/'+name+".png"),"Saving compositor capture failed"); return copy;
}
static QStringList loadedProviders() {
    const auto message=QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.freedesktop.DBus.Properties", "Get");
    auto request=message; request << QStringLiteral("org.kde.kwin.Effects") << QStringLiteral("loadedEffects");
    const auto reply=QDBusConnection::sessionBus().call(request,QDBus::Block,5000);
    require(reply.type()!=QDBusMessage::ErrorMessage,"Cannot read compositor providers");
    return qdbus_cast<QDBusVariant>(reply.arguments().first()).variant().toStringList();
}
static void startup()
{
    if (qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY").isEmpty()) return;
    // Native modules may launch Qt helpers that inherit LD_PRELOAD.
    if (QFileInfo(QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget()).fileName() != QStringLiteral("systemsettings")) return;
    QTimer::singleShot(1000, qApp, [] {
        QWidget *window = nullptr;
        for (auto candidate : QApplication::topLevelWidgets()) {
            if (candidate->inherits("SettingsBase")) window = candidate;
        }
        require(window, "Missing System Settings window");
        auto nav = window->findChild<QObject *>(QStringLiteral("settingsNavigation"));
        require(nav, "Missing navigation controller");
        if (qEnvironmentVariableIsSet("SEQUOIA_SETTINGS_RTL")) qApp->setLayoutDirection(Qt::RightToLeft);
        if (qEnvironmentVariableIsSet("SEQUOIA_HUB_COMPOSITED")) {
            auto backdrop=new HubBackdrop;
            backdrop->setGeometry(QGuiApplication::primaryScreen()->geometry());backdrop->show();
            QTest::qWait(400);
            require(qEnvironmentVariable("WAYLAND_DISPLAY")==QStringLiteral("sequoia-settings-test"),"Backdrop fixture must use the private compositor");
            QTemporaryFile script; require(script.open(),"Cannot create backdrop positioning fixture");
            script.write("for (const w of workspace.windowList()) { print('HUB_WINDOW',w.caption,JSON.stringify(w.frameGeometry),w.minimized,w.keepBelow); if (w.caption.startsWith('Hub Test Backdrop')) { w.keepBelow=true; } else if (w.caption.includes('System Settings')) { workspace.activeWindow=w; } }");script.flush();
            auto bus=QDBusConnection::sessionBus(); HubWindowProbe probe;
            require(bus.registerVirtualObject("/HubWindowProbe",&probe),"Register fixture probe");
            script.write(QString("callDBus('%1','/HubWindowProbe','org.sequoia.Test','windows',JSON.stringify(workspace.windowList().map(w=>({caption:w.caption,geometry:w.frameGeometry,below:w.keepBelow,minimized:w.minimized,opacity:w.opacity}))));").arg(bus.baseService()).toUtf8());script.flush();
            auto load=QDBusMessage::createMethodCall("org.kde.KWin","/Scripting","org.kde.kwin.Scripting","loadScript");
            load << script.fileName() << QStringLiteral("hub-backdrop-position");
            auto reply=QDBusConnection::sessionBus().call(load);
            require(reply.type()!=QDBusMessage::ErrorMessage,"Loading backdrop fixture failed");
            auto run=QDBusMessage::createMethodCall("org.kde.KWin","/Scripting/Script"+QString::number(reply.arguments().first().toInt()),"org.kde.kwin.Script","run");
            bus.asyncCall(run);
            for(int i=0;i<100 && !probe.received;++i) QTest::qWait(20);
            require(probe.received,"Compositor fixture probe did not return");
            bus.unregisterObject("/HubWindowProbe");
            QTest::qWait(400);
        }
        window->resize(1100, 790);
        QTest::qWait(200);
        auto capture = [window](const QString &name) {
            const auto directory = qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY");
            if (qEnvironmentVariableIsSet("SEQUOIA_HUB_COMPOSITED")) { window->grab().save(directory + QLatin1Char('/') + name + QStringLiteral("-client.png")); composedCapture(name); }
            else require(window->grab().save(directory + QLatin1Char('/') + name + QStringLiteral(".png")), "Screenshot failed");
        };
        for (auto quick : window->findChildren<QQuickWidget *>()) require(quick->status() != QQuickWidget::Error && quick->rootObject(), "QML did not load");
        if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_CORNERS")) {
            require(!window->windowFlags().testFlag(Qt::FramelessWindowHint), "Native decoration was removed");
            auto checkCorners = [&] {
                QTest::qWait(350);
                auto handle = window->windowHandle();
                require(handle && handle->property("verifiedBlurEnabled").toBool(), "Glass backdrop was not requested");
                const auto region = handle->property("verifiedBlurRegion").value<QRegion>();
                require(region.isEmpty() || (region.contains(QPoint(0, 0)) && region.contains(QPoint(window->width()-1, 0))),
                        "Application top corners are rounded below the native decoration");
                if (!region.isEmpty() && !window->isMaximized() && !window->isFullScreen())
                    require(!region.contains(QPoint(0,window->height()-1)) && !region.contains(QPoint(window->width()-1,window->height()-1)),
                            "Lower application corners lost their rounding");
            };
            for (int pass = 0; pass < 2; ++pass) {
                for (const auto &group : nav->property("groups").toList()) {
                    require(call(nav,"browse",group.toMap().value("id").toString()),"Custom page navigation failed");
                    window->resize(1100 + pass, 790);
                    checkCorners();
                }
                open(nav,QStringLiteral("kcm_landingpage"));
                checkCorners();
                require(back(nav),"Cannot return from Quick Settings");
                checkCorners();
            }
            require(call(nav,"browse",QStringLiteral("appearance")),"Cannot reopen Appearance");
            window->showMaximized(); checkCorners();
            window->showNormal(); checkCorners();
            capture(QStringLiteral("client-top-corners"));
            fprintf(stderr,"PASS: square application top corners across custom pages, Quick Settings, resize and maximize/restore\n");
            QCoreApplication::exit(0); return;
        }
#include "verify-colors-page.inc"
        if (qEnvironmentVariableIsSet("SEQUOIA_PLASMA_ONLY")) {
            open(nav,QStringLiteral("kcm_desktoptheme"));QTest::qWait(800);
            auto find=[window](const QString &name)->QQuickItem* {
                for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->quickWindow()->contentItem()))if(item->objectName()==name)return item;
                return nullptr;
            };
            capture("plasma-style");
            auto page=find("plasmaStyleHub");require(page,"Plasma Style presentation missing");
            require(find("plasmaStyleBack") && find("settingsBack"),"Back controls missing");
            require(find("plasmaStyleCard-sequoia-eleven-dark") && find("plasmaStyleCard-sequoia-test-removable"),"Native Plasma style cards missing");
            auto settings=page->property("settings").value<QObject*>(), backend=page->property("backend").value<QObject*>();
            require(settings && backend,"Native Plasma backend missing");
            auto model=qobject_cast<QAbstractItemModel*>(backend->property("filteredModel").value<QObject*>());require(model,"Native filtered theme model missing");
            auto viewport=page->property("flickable").value<QQuickItem*>();require(viewport,"Plasma style viewport missing");
            auto reveal=[&](QQuickItem *item){
                auto content=viewport->property("contentItem").value<QQuickItem*>();bool within=false;
                for(auto parent=item;parent;parent=parent->parentItem())if(parent==content)within=true;
                if(within)viewport->setProperty("contentY",qBound(0.,item->mapToItem(content,QPointF()).y()-20.,qMax(0.,viewport->property("contentHeight").toReal()-viewport->height())));
                QTest::qWait(100);
            };
            auto click=[&](QQuickItem *item){require(item,"Plasma style control missing");reveal(item);QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(250);};
            const int allCount=model->rowCount();require(allCount>=5,"Native themes missing");
            auto search=find("plasmaStyleSearch");search->setProperty("text","No matching Plasma fixture");QTest::qWait(250);
            require(model->rowCount()==0 && find("plasmaStyleEmpty")->isVisible(),"Search did not filter native themes");
            capture("plasma-style-empty");click(find("plasmaStyleClearFilters"));require(model->rowCount()==allCount,"Clear filters did not restore native themes");
            search->setProperty("text","Sequoia");QTest::qWait(200);require(model->rowCount()>0 && model->rowCount()<allCount,"Search did not find matching styles");
            search->setProperty("text","");auto filter=find("plasmaStyleFilter");filter->forceActiveFocus();QTest::keyClick(filter->window(),Qt::Key_End);QTest::qWait(250);
            require(filter->property("currentIndex").toInt()==3 && model->rowCount()>0 && model->rowCount()<allCount,"Color-compatible filter did not apply");capture("plasma-style-filtered");
            QTest::keyClick(filter->window(),Qt::Key_Home);QTest::qWait(200);require(model->rowCount()==allCount,"All themes filter did not restore gallery");
            auto original=settings->property("name").toString();
            click(find("plasmaStyleCard-sequoia-eleven-dark"));require(settings->property("name").toString()=="sequoia-eleven-dark" && find("plasmaStyleApply")->isEnabled(),"Card selection did not stage native theme");
            click(find("plasmaStyleReset"));require(settings->property("name").toString()==original && !backend->property("needsSave").toBool(),"Reset did not restore native theme");
            click(find("plasmaStyleCard-sequoia-eleven-dark"));click(find("plasmaStyleApply"));QTest::qWait(400);
            KConfig applied("plasmarc");applied.reparseConfiguration();require(applied.group("Theme").readEntry("name",QString())=="sequoia-eleven-dark" && !backend->property("needsSave").toBool(),"Native Plasma Apply did not persist");
            auto openActions=[&](const QString &id)->QObject* {auto card=find("plasmaStyleCard-"+id);require(card,"Theme actions card missing");click(card->property("actionsButton").value<QQuickItem*>());auto menu=card->findChild<QObject*>(card->objectName()+"Menu");require(menu && menu->property("visible").toBool(),"Style actions did not open");return menu;};
            auto action=[](QObject *menu,const QString &text)->QQuickItem* {for(int i=0;i<menu->property("count").toInt();++i){QQuickItem *item=nullptr;QMetaObject::invokeMethod(menu,"itemAt",Q_RETURN_ARG(QQuickItem*,item),Q_ARG(int,i));if(item && item->property("text").toString()==text)return item;}return nullptr;};
            auto activeMenu=openActions("sequoia-eleven-dark");require(!action(activeMenu,"Remove Plasma style")->isEnabled(),"Active Plasma style can be removed");QMetaObject::invokeMethod(activeMenu,"close");QTest::qWait(200);
            auto menu=openActions("sequoia-test-removable");capture("plasma-style-actions");
            auto trigger=find("plasmaStyleCard-sequoia-test-removable")->property("actionsButton").value<QQuickItem*>();click(trigger);require(!menu->property("visible").toBool(),"Style actions flashed open on second click");menu=openActions("sequoia-test-removable");
            click(action(menu,"Remove Plasma style"));require(find("plasmaStyleCard-sequoia-test-removable")->property("pendingRemoval").toBool() && find("plasmaStyleApply")->isEnabled(),"Removal did not stage");
            const auto fixture=qEnvironmentVariable("XDG_DATA_HOME")+"/plasma/desktoptheme/sequoia-test-removable";require(QFileInfo::exists(fixture),"Removal happened before Apply");
            menu=openActions("sequoia-test-removable");click(action(menu,"Don’t delete this Plasma style"));require(!find("plasmaStyleCard-sequoia-test-removable")->property("pendingRemoval").toBool(),"Undo removal failed");
            menu=openActions("sequoia-test-removable");click(action(menu,"Remove Plasma style"));click(find("plasmaStyleApply"));QTest::qWait(500);require(!QFileInfo::exists(fixture),"Native removal did not delete private fixture");
            require(QMetaObject::invokeMethod(backend,"installThemeFromFile",Q_ARG(QUrl,QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_PLASMA_INSTALL_FIXTURE")))),"Native install method missing");
            for(int i=0;i<100 && !find("plasmaStyleCard-sequoia-test-installed");++i)QTest::qWait(100);
            require(find("plasmaStyleCard-sequoia-test-installed"),"Native archive installation did not refresh gallery");require(find("plasmaStyleMessage")->isVisible(),"Install feedback missing");
            // A back navigation request must retain the native unsaved-change protection.
            click(find("plasmaStyleCard-sequoia-eleven-light"));QTimer::singleShot(150,[]{for(auto widget:QApplication::topLevelWidgets())if(auto dialog=qobject_cast<QDialog*>(widget))dialog->reject();});click(find("plasmaStyleBack"));
            require(nav->property("moduleOpen").toBool() && backend->property("needsSave").toBool(),"Back bypassed unsaved changes");click(find("plasmaStyleReset"));
            viewport->setProperty("contentY",0);auto theme=page->property("theme").value<QObject*>();require(theme,"Shared Plasma theme missing");
            for(const QString &material:{"solid","transparent","glass"})for(const QString &mode:{"light","dark"}){theme->setProperty("material",material);theme->setProperty("mode",mode);QTest::qWait(150);capture("plasma-style-"+material+"-"+mode);}
            for(int width:{1450,930}) {
                window->resize(width,790);QTest::qWait(250);auto grid=find("plasmaStyleGrid");QQuickItem *first=nullptr;
                for(auto item:grid->childItems())if(item->objectName().startsWith("plasmaStyleCard-")){if(!first)first=item;require(item->width()<=361,"Plasma card exceeded width cap");}
                require(first && qAbs(first->x())<1 && qAbs(first->y())<1,"Plasma cards skipped the first cell");capture("plasma-style-width-"+QString::number(width));
            }
            click(find("plasmaStyleBack"));require(!nav->property("moduleOpen").toBool(),"Round Back did not leave safely");
            fprintf(stderr,"PASS: Plasma Style native previews, search/filter/empty state, staging/Apply/Reset/persistence, active-theme removal protection, staged removal/undo, archive install, menu toggle, unsaved navigation, six variants and responsive cards\n");
            QCoreApplication::exit(0);return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_STYLE_ONLY")) {
            open(nav,QStringLiteral("kcm_style"));QTest::qWait(800);
            auto find=[window](const QString &name)->QQuickItem* {
                for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->quickWindow()->contentItem()))if(item->objectName()==name)return item;
                return nullptr;
            };
            capture("application-style");
            auto page=find("applicationStyleHub");require(page,"Application Style presentation missing");
            require(find("applicationStyleBack") && find("settingsBack"),"Back controls missing");
            auto grid=find("applicationStyleGrid");require(grid,"Style grid missing");
            // Exercise the column counts that exposed invisible helpers consuming grid cells.
            for(const int width:{1450,3438,930,1100}) {
                window->resize(width,width>2000 ? 1200 : 790);QTest::qWait(300);
                QList<QQuickItem*> cards;
                for(auto item:grid->childItems())if(item->objectName().startsWith("applicationStyleCard-") && item->isVisible())cards.append(item);
                require(cards.size()>=7,"Missing style cards during reflow");
                const int columns=grid->property("columns").toInt();
                require(qAbs(cards.first()->x())<1. && qAbs(cards.first()->y())<1.,"Style grid did not start in its upper-left cell");
                for(int i=1;i<cards.size();++i) {
                    if(i%columns==0)require(qAbs(cards[i]->x())<1. && cards[i]->y()>cards[i-1]->y(),"Style row did not wrap to the left edge");
                    else require(cards[i]->x()>cards[i-1]->x() && qAbs(cards[i]->y()-cards[i-1]->y())<2.,"Style grid skipped a cell or displaced a card");
                }
                if(width==1450 || width==3438)capture("application-style-packed-"+QString::number(width));
            }
            auto settings=page->property("settings").value<QObject*>(), backend=page->property("backend").value<QObject*>();
            require(settings && backend,"Native style backend missing");
            require(find("applicationStyleCard-SequoiaElevenGlass"),"Liquid Glass application style was removed");
            auto viewport=page->property("flickable").value<QQuickItem*>();require(viewport,"Style viewport missing");
            auto reveal=[&](QQuickItem *item) {
                auto content=viewport->property("contentItem").value<QQuickItem*>();bool within=false;
                for(auto parent=item;parent;parent=parent->parentItem())if(parent==content)within=true;
                if(within)viewport->setProperty("contentY",qBound(0.,item->mapToItem(content,QPointF()).y()-30.,qMax(0.,viewport->property("contentHeight").toReal()-viewport->height())));
                QTest::qWait(100);
            };
            auto click=[&](QQuickItem *item){require(item,"Missing style control");reveal(item);QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(200);};
            // Hidden Union-only and undo actions must not leave blank menu rows.
            auto plainCard=find("applicationStyleCard-SequoiaEleven");require(plainCard,"Sequoia style card missing");
            auto plainMenu=plainCard->findChild<QObject*>(plainCard->objectName()+"Menu");require(plainMenu,"Style menu missing");
            click(plainCard->property("actionsButton").value<QQuickItem*>());
            capture("application-style-fit-menu");
            require(qAbs(plainMenu->property("height").toReal()-84.-plainMenu->property("topPadding").toReal()-plainMenu->property("bottomPadding").toReal())<=1.,"Style menu retained hidden action rows");
            click(plainCard->property("actionsButton").value<QQuickItem*>());
            const auto originalStyle=settings->property("widgetStyle").toString();
            click(find("applicationStyleCard-Fusion"));
            require(settings->property("widgetStyle").toString()=="Fusion" && find("applicationStyleApply")->isEnabled(),"Preview click did not stage style");
            click(find("applicationStyleReset"));require(settings->property("widgetStyle").toString()==originalStyle && !backend->property("needsSave").toBool(),"Style Reset did not restore selection");
            click(find("applicationStyleCard-sequoia-test-removable"));require(settings->property("widgetStyle").toString()=="Union" && settings->property("unionStyle").toString()=="sequoia-test-removable","Union selection did not stage native style id");
            click(find("applicationStyleReset"));
            auto fixture=find("applicationStyleCard-sequoia-test-removable");
            auto actions=fixture->findChild<QObject*>(fixture->objectName()+"Menu");require(actions,"Style actions missing");
            click(fixture->property("actionsButton").value<QQuickItem*>());require(actions->property("visible").toBool(),"Style actions did not open");
            auto action=[&](const QString &text){for(int i=0;i<actions->property("count").toInt();++i){QQuickItem *item=nullptr;QMetaObject::invokeMethod(actions,"itemAt",Q_RETURN_ARG(QQuickItem*,item),Q_ARG(int,i));if(item && item->property("text").toString()==text)return item;}return static_cast<QQuickItem*>(nullptr);};
            require(qAbs(actions->property("height").toReal()-126.-actions->property("topPadding").toReal()-actions->property("bottomPadding").toReal())<=1.,"Union menu retained hidden undo row");
            click(action("Uninstall…"));require(backend->property("stylesToUninstall").toStringList().contains("sequoia-test-removable") && fixture->property("pendingRemoval").toBool(),"Uninstall did not stage removal");
            require(QFileInfo::exists(qEnvironmentVariable("XDG_DATA_HOME")+"/union/styles/sequoia-test-removable"),"Uninstall removed fixture before Apply");
            click(fixture->property("actionsButton").value<QQuickItem*>());click(action("Do not uninstall this style"));require(backend->property("stylesToUninstall").toStringList().isEmpty(),"Undo uninstall failed");
            click(find("applicationStyleReset"));
            auto dialog=page->findChild<QObject*>("applicationStyleEffectsDialog");require(dialog,"Options dialog missing");
            click(find("applicationStyleEffects"));require(dialog->property("visible").toBool(),"Options did not open");capture("application-style-options-open");
            click(find("applicationStyleEffects"));QTest::qWait(250);require(!dialog->property("visible").toBool(),"Options trigger reopened on second click");
            click(find("applicationStyleEffects"));
            const bool beforeButtons=settings->property("iconsOnButtons").toBool(), beforeMenus=settings->property("iconsInMenus").toBool();
            click(find("applicationStyleButtonIcons"));click(find("applicationStyleMenuIcons"));
            require(settings->property("iconsOnButtons").toBool()!=beforeButtons && settings->property("iconsInMenus").toBool()!=beforeMenus,"Icon pills did not update native settings");
            for(const QString &name:{"applicationStyleMainToolbar","applicationStyleSecondaryToolbar"}){
                auto selector=find(name);selector->forceActiveFocus();QTest::keyClick(selector->window(),Qt::Key_End);QTest::qWait(200);
                require(selector->property("currentIndex").toInt()==3,"Toolbar keyboard selection failed");
            }
            require(settings->property("toolButtonStyle").toString()=="TextUnderIcon" && settings->property("toolButtonStyleOtherToolbars").toString()=="TextUnderIcon","Toolbar enum mapping changed");
            capture("application-style-options");click(find("applicationStyleEffectsDone"));click(find("applicationStyleCard-Fusion"));click(find("applicationStyleApply"));
            require(!backend->property("needsSave").toBool(),"Style Apply remained dirty");
            KConfig applied("kdeglobals");applied.reparseConfiguration();
            require(applied.group("KDE").readEntry("widgetStyle",QString())=="Fusion","Style Apply did not persist native style");
            require(applied.group("KDE").readEntry("ShowIconsOnPushButtons",beforeButtons)!=beforeButtons && applied.group("KDE").readEntry("ShowIconsInMenuItems",beforeMenus)!=beforeMenus,"Icon options did not persist");
            require(applied.group("Toolbar style").readEntry("ToolButtonStyle",QString())=="TextUnderIcon" && applied.group("Toolbar style").readEntry("ToolButtonStyleOtherToolbars",QString())=="TextUnderIcon","Toolbar options did not persist");
            click(find("applicationStyleEffects"));click(find("applicationStyleButtonIcons"));QTest::keyClick(find("applicationStyleButtonIcons")->window(),Qt::Key_Escape);QTest::qWait(450);
            require(!dialog->property("visible").toBool(),"Options Escape failed");click(find("applicationStyleReset"));require(settings->property("iconsOnButtons").toBool()!=beforeButtons,"Options Reset failed");
            require(find("applicationStyleGtk")->isVisible()==backend->property("gtkConfigKdedModuleLoaded").toBool(),"GTK availability gate changed");
            QMetaObject::invokeMethod(page,"openGtkStyleSettings");QTest::qWait(300);
            if(backend->property("gtkConfigKdedModuleLoaded").toBool()) {
                require(find("applicationStyleGtkSelector") && find("applicationStyleGtkRemove"),"GTK subpage controls did not load");
                capture("application-style-gtk");click(find("applicationStyleBack"));require(!page->property("gtkMode").toBool() && nav->property("moduleOpen").toBool(),"GTK local Back left native module");
            } else require(!page->property("gtkMode").toBool(),"Unavailable GTK backend was opened");
            auto theme=page->property("theme").value<QObject*>();require(theme,"Style theme missing");viewport->setProperty("contentY",0);
            for(const QString &material:{"solid","transparent","glass"})for(const QString &mode:{"light","dark"}){theme->setProperty("material",material);theme->setProperty("mode",mode);QTest::qWait(180);capture("application-style-"+material+"-"+mode);}
            window->resize(930,650);QTest::qWait(200);capture("application-style-compact");require(find("applicationStyleGrid")->property("columns").toInt()==1,"Style grid did not reflow");
            click(find("applicationStyleBack"));require(!nav->property("moduleOpen").toBool(),"Style round Back did not leave safely");
            fprintf(stderr,"PASS: Application Style native Qt/Union selection, Apply/persistence/Reset, staged uninstall and undo, icons/toolbars options and keyboard mapping, repeated dialog toggle/Escape, GTK availability gate, all materials, compact layout and circular Back\n");
            QCoreApplication::exit(0);return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_GLOBAL_ONLY")) {
            open(nav,QStringLiteral("kcm_lookandfeel")); QTest::qWait(800);
            auto find = [window](const QString &name) -> QQuickItem * {
                for (auto quick : window->findChildren<QQuickWidget *>()) if (quick->rootObject())
                    for (auto item : items(quick->quickWindow()->contentItem())) if (item->objectName()==name) return item;
                return nullptr;
            };
            capture("global-theme");
            auto page=find("globalThemeHub"); require(page,"Global Theme presentation did not load");
            require(find("globalThemeCard-org.sequoia.eleven.dark.transparent"),"Installed theme card missing");
            require(find("globalThemeBack") && find("settingsBack"),"Back controls missing");
            auto settings=page->property("settings").value<QObject*>();auto backend=page->property("backend").value<QObject*>();
            require(settings && backend,"Missing native global-theme backend");
            auto viewport=page->property("flickable").value<QQuickItem*>();require(viewport,"Global Theme viewport missing");
            auto reveal=[&](QQuickItem *item) {
                auto content=viewport->property("contentItem").value<QQuickItem*>();
                bool within=false;for(auto parent=item;parent;parent=parent->parentItem())if(parent==content)within=true;
                if(within)viewport->setProperty("contentY",qBound(0.,item->mapToItem(content,QPointF()).y()-30.,qMax(0.,viewport->property("contentHeight").toReal()-viewport->height())));
                QTest::qWait(100);
            };
            auto click=[&](QQuickItem *item) { require(item,"Cannot click missing global-theme control");reveal(item);QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(180); };
            auto findObject=[&](const QString &name) -> QObject* {
                if(auto result=page->findChild<QObject*>(name))return result;
                for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())if(auto result=quick->rootObject()->findChild<QObject*>(name))return result;
                return nullptr;
            };
            auto sheet=findObject("globalThemeConfirmSheet");require(sheet,"Global apply dialog missing");
            click(find("globalThemeCard-org.sequoia.eleven.dark.transparent"));
            require(sheet->property("visible").toBool() && settings->property("lookAndFeelPackage").toString()=="org.sequoia.eleven.dark.transparent","Theme click did not select package and ask what to apply");
            capture("global-theme-confirm");click(find("globalThemeMoreOptions"));require(sheet->property("detailed").toBool(),"Detailed theme options did not open");
            QList<QQuickItem*> options;QQuickItem *colors=nullptr;
            for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->quickWindow()->contentItem()))if(item->objectName().startsWith("globalThemeOption-") && item->isVisible()) { options.append(item); if(item->property("text").toString()=="Colors")colors=item; }
            require(colors && options.size()>=4,"Native appearance options missing");
            const int colorFlag=colors->objectName().section('-',-1).toInt();
            const int before=backend->property("selectedContents").toInt();click(colors);
            require(backend->property("selectedContents").toInt()==(before ^ colorFlag),"Checkbox did not update native selected contents");
            click(colors);capture("global-theme-more-options");click(find("globalThemeConfirmCancel"));require(!sheet->property("visible").toBool(),"Cancel did not dismiss apply dialog");
            click(find("globalThemeCard-org.sequoia.test.removable"));
            auto layoutToggle=find("globalThemeLayoutToggle");require(layoutToggle && layoutToggle->isVisible(),"Desktop layout option missing from layout-capable theme");
            if(!layoutToggle->property("checked").toBool())click(layoutToggle);
            require(find("globalThemeLayoutWarning")->isVisible(),"Desktop replacement warning missing");click(find("globalThemeConfirmCancel"));
            // Exercise native application, but only Colors: the test never applies a desktop layout.
            click(find("globalThemeCard-org.sequoia.eleven.dark.transparent"));click(find("globalThemeMoreOptions"));
            for(auto option:options)if(option->isVisible() && option->property("checked").toBool() != (option==colors))click(option);
            require(backend->property("selectedContents").toInt()==colorFlag,"Could not select only Colors for isolated apply");
            click(find("globalThemeConfirmApply"));QTest::qWait(800);
            require(!sheet->property("visible").toBool(),"Apply dialog did not close");
            KConfig applied("kdeglobals");applied.reparseConfiguration();
            require(applied.group("General").readEntry("ColorScheme",QString())=="SequoiaElevenDarkTransparent","Native Global Theme Apply did not apply selected colors");
            click(find("globalThemeAutomatic"));require(settings->property("automaticLookAndFeel").toBool(),"Automatic switching did not enable");capture("global-theme-automatic");
            click(find("globalThemeLight"));require(page->property("subpage").toString()=="light","Light selector missing");
            click(find("globalThemeCard-org.sequoia.eleven.light.transparent"));require(settings->property("defaultLightLookAndFeel").toString()=="org.sequoia.eleven.light.transparent" && page->property("subpage").toString().isEmpty(),"Light assignment did not return to automatic settings");
            click(find("globalThemeDark"));click(find("globalThemeCard-org.sequoia.eleven.dark.transparent"));require(settings->property("defaultDarkLookAndFeel").toString()=="org.sequoia.eleven.dark.transparent","Dark assignment did not stage");
            auto idle=find("globalThemeWaitIdle");if(!idle->property("checked").toBool())click(idle);
            auto interval=find("globalThemeIdleInterval");require(interval && interval->isEnabled(),"Idle interval did not enable");reveal(interval);interval->forceActiveFocus();
            int initialInterval=settings->property("automaticLookAndFeelIdleInterval").toInt();QTest::keyClick(interval->window(),Qt::Key_Up);QTest::qWait(150);
            require(settings->property("automaticLookAndFeelIdleInterval").toInt()==initialInterval+1,"Idle interval keyboard edit failed");
            require(find("globalThemeApply")->isEnabled(),"Automatic settings did not enable Apply");click(find("globalThemeApply"));QTest::qWait(600);
            applied.reparseConfiguration();require(applied.group("KDE").readEntry("AutomaticLookAndFeel",false) && applied.group("KDE").readEntry("DefaultLightLookAndFeel",QString())=="org.sequoia.eleven.light.transparent" && applied.group("KDE").readEntry("DefaultDarkLookAndFeel",QString())=="org.sequoia.eleven.dark.transparent","Automatic theme assignments did not persist");
            const bool idleBefore=settings->property("automaticLookAndFeelOnIdle").toBool();click(idle);click(find("globalThemeReset"));require(settings->property("automaticLookAndFeelOnIdle").toBool()==idleBefore,"Reset did not restore idle setting");
            click(find("globalThemeLight"));click(find("globalThemeBack"));require(page->property("subpage").toString().isEmpty() && nav->property("moduleOpen").toBool(),"In-page Back left the Global Theme module");
            click(find("globalThemeSaveCurrent"));auto name=find("globalThemeSnapshotName");require(name && name->isVisible(),"Save Current Theme page missing");
            name->setProperty("text","Breeze");QTest::qWait(200);require(!find("globalThemeSnapshotConfirm")->isEnabled(),"Duplicate theme name was accepted");
            name->setProperty("text","Sequoia Integration Snapshot");QTest::qWait(200);require(find("globalThemeSnapshotConfirm")->isEnabled(),"Valid snapshot name was rejected");capture("global-theme-save");
            click(find("globalThemeSnapshotConfirm"));QTest::qWait(600);require(page->property("subpage").toString().isEmpty(),"Saving theme did not return");
            bool snapshotFound=false;QDirIterator packages(qEnvironmentVariable("XDG_DATA_HOME")+"/plasma/look-and-feel",QStringList{"metadata.json"},QDir::Files,QDirIterator::Subdirectories);
            while(packages.hasNext()){QFile metadata(packages.next());if(metadata.open(QIODevice::ReadOnly))snapshotFound |= metadata.readAll().contains("Sequoia Integration Snapshot");}
            require(snapshotFound,"Native Save Current Theme did not create a package in the isolated data home");
            click(find("globalThemeAutomatic"));require(!settings->property("automaticLookAndFeel").toBool(),"Automatic mode did not disable");
            auto removable=find("globalThemeCard-org.sequoia.test.removable");auto trigger=removable->property("changeButton").value<QQuickItem*>();click(trigger);
            QObject *actions=nullptr;for(auto child:removable->findChildren<QObject*>())if(child->property("trigger").value<QQuickItem*>()==trigger)actions=child;
            require(actions && actions->property("visible").toBool(),"Theme actions menu did not open");
            QQuickItem *remove=nullptr;for(int i=0;i<actions->property("count").toInt();++i){QQuickItem *item=nullptr;QMetaObject::invokeMethod(actions,"itemAt",Q_RETURN_ARG(QQuickItem*,item),Q_ARG(int,i));if(item && item->property("text").toString()=="Remove theme")remove=item;}
            require(remove && remove->isEnabled(),"Removable fixture cannot be removed");click(remove);
            auto deletion=findObject("globalThemeDeleteSheet");require(deletion && deletion->property("visible").toBool(),"Removal confirmation missing");capture("global-theme-remove");
            click(find("globalThemeDeleteConfirm"));QTest::qWait(600);require(!QFileInfo::exists(qEnvironmentVariable("XDG_DATA_HOME")+"/plasma/look-and-feel/org.sequoia.test.removable"),"Native theme removal did not delete isolated fixture");
            const auto previewUrl=find("globalThemeCard-org.sequoia.eleven.dark.transparent")->property("previewSource").toUrl();
            require(QMetaObject::invokeMethod(page,"showThemePreview",Q_ARG(QVariant,previewUrl.toString())),"Full-screen preview action missing");QTest::qWait(250);
            auto previewWindow=qobject_cast<QQuickWindow*>(findObject("globalThemePreviewWindow"));require(previewWindow && previewWindow->isVisible(),"Full-screen theme preview did not open");
            QTest::keyClick(previewWindow,Qt::Key_Escape);QTest::qWait(180);require(!previewWindow->isVisible(),"Full-screen preview Escape did not close");
            window->resize(930,650);viewport->setProperty("contentY",0);QTest::qWait(250);capture("global-theme-compact");
            require(find("globalThemeGrid")->property("columns").toInt()==2,"Global theme grid did not reflow");
            click(find("globalThemeBack"));require(!nav->property("moduleOpen").toBool(),"Global Theme round Back did not leave safely");
            fprintf(stderr,"PASS: Global Theme native colors apply, component selection, layout warning, day/night assignments and persistence, idle edits/reset, save/validation, removal confirmation, fullscreen preview, compact layout and safe back navigation\n");
            QCoreApplication::exit(0); return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_QUICK_ONLY")) {
            open(nav,QStringLiteral("kcm_landingpage")); QTest::qWait(700);
            auto find = [window](const QString &name) -> QQuickItem * {
                for (auto quick : window->findChildren<QQuickWidget *>()) if (quick->rootObject())
                    for (auto item : items(quick->rootObject())) if (item->objectName()==name) return item;
                return nullptr;
            };
            capture(QStringLiteral("quick-settings"));
            auto page=find("quickSettingsHub"); require(page,"Redesigned Quick Settings root missing");
            for(const QString &name:{"quickSettingsBack","quickSettingsLight","quickSettingsDark","quickSettingsAutomatic","quickSettingsAnimation","quickSettingsDoubleClick","quickSettingsSingleClick","quickSettingsReset","quickSettingsApply"}) require(find(name),qPrintable("Missing control: "+name));
            require(find("settingsBack") && find("settingsBack")->isVisible(),"Sidebar back button removed");
            auto settings=page->property("settings").value<QObject*>();
            auto backend=page->property("backend").value<QObject*>();
            require(settings && backend,"Native Quick Settings backend missing");
            auto viewport=page->property("flickable").value<QQuickItem*>();
            require(viewport,"Quick Settings scroll viewport missing");
            auto reveal=[&](QQuickItem *item) {
                auto content=viewport->property("contentItem").value<QQuickItem*>();
                if(content && item!=find("quickSettingsApply") && item!=find("quickSettingsReset") && item!=find("quickSettingsBack")) {
                    const auto pos=item->mapToItem(content,QPointF());
                    viewport->setProperty("contentY",qBound(0.,pos.y()-30.,qMax(0.,viewport->property("contentHeight").toReal()-viewport->height())));
                }
                QTest::qWait(100);
            };
            auto click=[&](QQuickItem *item) {
                reveal(item);
                QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());
                QTest::qWait(200);
            };
            require(!find("quickSettingsApply")->isEnabled(),"Clean native page unexpectedly dirty");
            const bool initialSingle=settings->property("singleClick").toBool();
            click(find(initialSingle ? "quickSettingsDoubleClick" : "quickSettingsSingleClick"));
            require(settings->property("singleClick").toBool()!=initialSingle && backend->property("needsSave").toBool() && find("quickSettingsApply")->isEnabled(),"File-click edit did not stage natively");
            click(find("quickSettingsReset"));
            require(settings->property("singleClick").toBool()==initialSingle && !backend->property("needsSave").toBool(),"Reset did not restore native config");
            auto slider=find("quickSettingsAnimation");reveal(slider);slider->forceActiveFocus();
            QTest::keyClick(slider->window(),Qt::Key_End);QTest::qWait(150);
            require(settings->property("animationDurationFactor").toReal()==0 && find("quickSettingsApply")->isEnabled(),"Slider End did not stage Instant");
            QTest::keyClick(slider->window(),Qt::Key_Home);QTest::qWait(150);
            require(settings->property("animationDurationFactor").toReal()==4,"Slider Home did not stage Slow");
            QTest::keyClick(slider->window(),Qt::Key_Right);QTest::qWait(150);
            require(settings->property("animationDurationFactor").toReal()==2,"Slider arrow did not use KDE value mapping");
            click(find("quickSettingsApply"));
            require(!backend->property("needsSave").toBool(),"Apply did not clear native dirty state");
            KConfig written("kdeglobals"); written.reparseConfiguration();
            require(written.group("KDE").readEntry("AnimationDurationFactor",-1.)==2,"Apply did not persist animation to isolated config");
            click(find("quickSettingsDark"));
            require(!settings->property("automaticLookAndFeel").toBool() && settings->property("lookAndFeelPackage")==settings->property("defaultDarkLookAndFeel"),"Dark card did not select assigned dark pack");
            click(find("quickSettingsAutomatic"));
            require(settings->property("automaticLookAndFeel").toBool(),"Automatic card did not enable native automatic switching");
            click(find("quickSettingsLight"));
            require(!settings->property("automaticLookAndFeel").toBool() && settings->property("lookAndFeelPackage")==settings->property("defaultLightLookAndFeel"),"Light card did not disable automatic and choose assigned pack");
            click(find("quickSettingsReset"));
            for(const QString &name:{"quickSettingsLight","quickSettingsDark"}) {
                auto card=find(name);reveal(card);
                auto trigger=card->property("changeButton").value<QQuickItem*>();require(trigger,"Theme chooser trigger missing");
                click(trigger);
                QObject *menu=nullptr;
                for(auto quick:window->findChildren<QQuickWidget*>()) if(quick->rootObject()) {
                    if(auto found=quick->rootObject()->findChild<QObject*>(name+"Menu"))menu=found;
                }
                if(!menu) menu=card->findChild<QObject*>(name+"Menu");
                require(menu && menu->property("visible").toBool() && menu->property("count").toInt()>0,"Native theme model did not populate picker");
                capture(QStringLiteral("quick-settings-")+name+"-menu");
                click(trigger);require(!menu->property("visible").toBool(),"Theme picker flashed open again on second click");
                click(trigger);
                const QString targetPack=name=="quickSettingsLight" ? "org.sequoia.eleven.light.transparent" : "org.sequoia.eleven.dark.transparent";
                QQuickItem *option=nullptr;
                for(int index=0;index<menu->property("count").toInt();++index) {
                    QQuickItem *candidate=nullptr;
                    require(QMetaObject::invokeMethod(menu,"itemAt",Q_RETURN_ARG(QQuickItem*,candidate),Q_ARG(int,index)),"Cannot inspect native theme options");
                    if(candidate && candidate->property("packageId").toString()==targetPack) option=candidate;
                }
                require(option,"Assigned theme pack missing from its native variant picker");
                // Keyboard selection scrolls the menu's real ListView into view.
                for(int index=0;index<menu->property("count").toInt()+1;++index) {
                    QTest::keyClick(trigger->window(),Qt::Key_Down);QTest::qWait(30);
                    if(option->property("highlighted").toBool()) break;
                }
                require(option->property("highlighted").toBool(),"Theme picker keyboard navigation failed");
                QTest::keyClick(trigger->window(),Qt::Key_Return);QTest::qWait(200);
                require(settings->property(name=="quickSettingsLight" ? "defaultLightLookAndFeel" : "defaultDarkLookAndFeel").toString()==targetPack,"Picker did not stage the selected native theme assignment");
                click(find("quickSettingsReset"));
            }
            // Leave with a staged change; cancel must keep the native KCM and draft.
            click(find(initialSingle ? "quickSettingsDoubleClick" : "quickSettingsSingleClick"));
            QTimer::singleShot(150, [] { for(auto widget:QApplication::topLevelWidgets()) if(auto dialog=qobject_cast<QDialog*>(widget)) dialog->reject(); });
            click(find("quickSettingsBack"));
            require(nav->property("moduleOpen").toBool() && backend->property("needsSave").toBool(),"Back bypassed native unsaved-change protection");
            click(find("quickSettingsReset"));
            viewport->setProperty("contentY",0);window->resize(1100,870);QTest::qWait(250);capture(QStringLiteral("quick-settings"));
            auto theme=page->property("theme").value<QObject*>();require(theme,"Shared page theme missing");
            for(const QString &material:{"solid","transparent","glass"}) for(const QString &mode:{"light","dark"}) {
                theme->setProperty("material",material);theme->setProperty("mode",mode);QTest::qWait(180);
                capture("quick-settings-"+material+"-"+mode);
            }
            window->showMaximized();viewport->setProperty("contentY",0);QTest::qWait(450);
            for(const QString &name:{"quickSettingsLight","quickSettingsDark","quickSettingsAutomatic"})require(find(name)->width()<=361,"Maximized theme card exceeded width cap");
            for(const QString &name:{"quickSettingsWallpaper","quickSettingsGlobalTheme"})require(find(name)->width()<=281,"Maximized shortcut exceeded width cap");
            for(const QString &name:{"quickSettingsDoubleClick","quickSettingsSingleClick"})require(find(name)->width()<=401,"Maximized file-click card exceeded width cap");
            require(find("quickSettingsAnimation")->width()<=813,"Maximized animation slider exceeded width cap");
            require(qAbs(find("quickSettingsDark")->x()-find("quickSettingsLight")->x()-find("quickSettingsLight")->width()-12)<1,"Capped theme cards spread across the page");
            for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->rootObject()))if(item->objectName().startsWith("quickSettingsRecent-"))require(item->width()<=281,"Recent-page shortcut exceeded width cap");
            capture("quick-settings-maximized");
            window->showNormal();QTest::qWait(200);
            window->resize(790,650);QTest::qWait(250);capture("quick-settings-compact");
            require(find("quickSettingsLight")->width()>240 && find("quickSettingsAutomatic")->y()>find("quickSettingsLight")->y(),"Theme cards did not reflow in a narrow window");
            viewport->setProperty("contentY",qMax(0.,viewport->property("contentHeight").toReal()-viewport->height()));QTest::qWait(150);capture("quick-settings-compact-bottom");
            click(find("quickSettingsBack"));require(!nav->property("moduleOpen").toBool(),"Round back button did not navigate");
            fprintf(stderr,"PASS: Quick Settings native staging, Apply/Reset, theme switching and pickers, keyboard slider, unsaved navigation, six styles and compact layout\n");
            QCoreApplication::exit(0); return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_SETTINGS_STARTUP")) {
            require(nav->property("moduleOpen").toBool(), "Startup deep link did not open a module");
            capture(QStringLiteral("startup"));
            require(back(nav), "Cannot leave startup module");
            require(call(nav, "browse", QStringLiteral("home")), "Cannot reach home after startup deep link");
        }
        require(nav->property("home").toBool(), "Application must start on the category overview");
        capture(QStringLiteral("home"));
        if (qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB")) {
            require(!window->windowFlags().testFlag(Qt::FramelessWindowHint), "Settings must retain the native glass titlebar above its islands");
            auto chrome = window->findChild<QObject *>(QStringLiteral("appearancePreview"));
            require(chrome, "Preview controller missing");
            require(QMetaObject::invokeMethod(chrome, "windowAction", Q_ARG(QString, QStringLiteral("maximize"))), "Maximize action missing");
            QTest::qWait(200);
            require(window->isMaximized(), "Maximize did not reach the native window");
            QMetaObject::invokeMethod(chrome, "windowAction", Q_ARG(QString, QStringLiteral("maximize")));
            QTest::qWait(200);
            require(!window->isMaximized(), "Restore failed");
            window->resize(1100,790);
        }
        QQuickItem *searchField = nullptr;
        for (auto quick : window->findChildren<QQuickWidget *>()) {
            if (quick->rootObject()) {
                if (auto field = quick->rootObject()->findChild<QQuickItem *>(QStringLiteral("settingsSearch"))) searchField = field;
            }
        }
        require(searchField, "Search field missing");
        searchField->setProperty("text", QStringLiteral("keyboard"));
        QTest::qWait(100);
        require(nav->property("query").toString() == QStringLiteral("keyboard"), "Search field not connected");
        searchField->setProperty("text", QString());
        QTest::qWait(100);
        require(nav->property("query").toString().isEmpty(), "Clearing search did not reset results");
        const auto catalog = nav->property("catalog").toList();
        require(catalog.size() > 40, "Too few discovered settings modules");
        QFile inventory(qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY") + QStringLiteral("/inventory.json"));
        require(inventory.open(QIODevice::WriteOnly), "Cannot write inventory");
        inventory.write(QJsonDocument(QJsonArray::fromVariantList(catalog)).toJson());
        inventory.close();
        QSet<QString> all, grouped;
        for (const auto &entry : catalog) all.insert(entry.toMap().value(QStringLiteral("id")).toString());
        require(all.size() == catalog.size(), "Duplicate modules in catalog");
        require(call(nav, "browse", QStringLiteral("all")), "All Settings navigation failed");
        require(nav->property("entries").toList().size() == catalog.size(), "All Settings omits modules");
        capture(QStringLiteral("all"));
        if (all.contains(QStringLiteral("sequoia-test-extra"))) {
            open(nav, QStringLiteral("sequoia-test-extra"));
            require(nav->property("moduleOpen").toBool(), "Uncategorized third-party module did not open");
            require(back(nav), "Cannot return from third-party module");
        }
        for (const auto &group : nav->property("groups").toList()) {
            require(call(nav, "browse", group.toMap().value(QStringLiteral("id")).toString()), "Category navigation failed");
            if (qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB") && !qEnvironmentVariableIsSet("SEQUOIA_POPUP_ONLY")) capture(QStringLiteral("overview-") + group.toMap().value(QStringLiteral("id")).toString());
            for (const auto &entry : nav->property("entries").toList()) grouped.insert(entry.toMap().value(QStringLiteral("id")).toString());
        }
        require(grouped == all, "Category union does not cover All Settings");
        require(!call(nav, "browse", QStringLiteral("unknown-category")), "Invalid category was accepted");
        require(call(nav, "browse", QStringLiteral("appearance")), "Appearance navigation failed");
        capture(QStringLiteral("appearance"));
        if (qEnvironmentVariableIsSet("SEQUOIA_PANELS_ONLY")) { verifyAppearancePanels(window,capture); qApp->quit(); return; }
        if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_LAYOUT")) {
            QQuickItem *image=nullptr; QObject *scroll=nullptr;
            for(auto quick:window->findChildren<QQuickWidget *>()) for(auto item:items(quick->rootObject())) {
                if(item->objectName()=="actualDesktopImage") image=item;
                if(item->objectName()=="appearanceMainContent") scroll=item;
            }
            require(image && scroll,"Missing appearance layout objects");
            window->resize(1100,650);QTest::qWait(200);
            auto viewport=scroll->property("contentItem").value<QQuickItem*>();
            auto edgeFade=scroll->property("edgeFade").value<QObject*>();
            require(viewport && edgeFade,"Appearance scroll fade missing");
            viewport->setProperty("contentY",0);QTest::qWait(200);
            require(!edgeFade->property("topHidden").toBool() && edgeFade->property("bottomHidden").toBool(),"Scroll-start fade must only affect the bottom edge");
            const auto wheelPoint=viewport->mapToScene(QPointF(viewport->width()/2,viewport->height()/2));
            QWheelEvent wheel(wheelPoint,viewport->window()->mapToGlobal(wheelPoint),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
            QCoreApplication::sendEvent(viewport->window(),&wheel);QTest::qWait(300);
            require(viewport->property("contentY").toReal()>0,"Edge fade intercepted wheel scrolling");
            const qreal lastY=viewport->property("contentHeight").toReal()-viewport->height();
            viewport->setProperty("contentY",lastY/2);QTest::qWait(250);
            require(edgeFade->property("topHidden").toBool() && edgeFade->property("bottomHidden").toBool(),"Middle scroll position must fade both edges");
            capture(QStringLiteral("scroll-fade-middle"));
            const auto faded=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
            edgeFade->setProperty("shaderEnabled",false);QTest::qWait(200);
            const auto unfaded=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
            edgeFade->setProperty("shaderEnabled",true);QTest::qWait(200);
            double fadeDifference=0;
            for(int y=0;y<faded.height();++y) for(int x=0;x<faded.width();++x) {
                const auto a=faded.pixel(x,y),b=unfaded.pixel(x,y);
                fadeDifference+=qAbs(qRed(a)-qRed(b))+qAbs(qGreen(a)-qGreen(b))+qAbs(qBlue(a)-qBlue(b));
            }
            require(fadeDifference>1000,"Scroll fade did not visibly soften clipped content");
            viewport->setProperty("contentY",lastY);QTest::qWait(200);
            require(edgeFade->property("topHidden").toBool() && !edgeFade->property("bottomHidden").toBool(),"End-of-scroll content must not be faded out");
            capture(QStringLiteral("scroll-fade-end"));
            QQuickItem *headerIcon=nullptr,*sidebarScroll=nullptr;
            for(auto quick:window->findChildren<QQuickWidget *>()) for(auto item:items(quick->rootObject())) {
                if(item->objectName()=="settingsHeaderIcon")headerIcon=item;
                if(item->objectName()=="settingsCategoryScroll")sidebarScroll=item;
            }
            require(headerIcon && headerIcon->property("source").toString()=="preferences-system" && !headerIcon->property("isMask").toBool(),"Sidebar header must use the full-color System Settings icon");
            require(sidebarScroll,"Sidebar scroll region missing");
            auto sidebarFade=sidebarScroll->property("edgeFade").value<QObject*>();
            auto sidebarViewport=sidebarScroll->property("contentItem").value<QQuickItem*>();
            require(sidebarFade && sidebarViewport,"Sidebar fade missing");
            sidebarViewport->setProperty("contentY",sidebarViewport->property("contentHeight").toReal()-sidebarViewport->height());QTest::qWait(200);
            require(sidebarFade->property("topHidden").toBool() && !sidebarFade->property("bottomHidden").toBool(),"Sidebar end fade hides the last navigation item");
            capture(QStringLiteral("sidebar-scroll-fade"));
            fprintf(stderr,"PASS: themed header icon, conditional scroll fades, wheel scrolling and rendered fade (pixel difference %.0f)\n",fadeDifference);
            viewport->setProperty("contentY",0);
            bool stable=true; int changes=0;
            for(int h=880;h<=1080;h+=4) {
                window->resize(1630,h); QTest::qWait(120);
                require(image->width() >= image->height() * 16. / 9. - 1,
                        "Desktop preview became taller than its landscape frame");
                for(auto quick:window->findChildren<QQuickWidget *>())
                    for(auto item:items(quick->rootObject())) if(item->objectName()=="materialDesktopImage") {
                        require(item->width() >= item->height() * 16. / 9. - 1,
                                "Material preview became taller than its landscape frame");
                    }
                QSignalSpy spy(image,&QQuickItem::widthChanged);
                const qreal before=image->width(); QTest::qWait(180);
                if(spy.count()>0) { stable=false; changes+=spy.count(); fprintf(stderr,"UNSTABLE height=%d width=%.1f -> %.1f changes=%lld availableHeight=%.1f contentHeight=%.1f\n",h,before,image->width(),(long long)spy.count(),scroll->property("availableHeight").toDouble(),scroll->property("contentHeight").toDouble()); }
            }
            fprintf(stderr,"Layout width changes after settling: %d\n",changes);
            require(stable,"Appearance width oscillates at fixed window size");
            capture(QStringLiteral("stable-desktop-layout"));
            fprintf(stderr,"PASS: stable appearance geometry across 51 window heights\n");
            QCoreApplication::exit(0);return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_DESKTOP")) {
            auto renderer=window->findChild<QObject *>(QStringLiteral("desktopRenderer"));
            require(renderer,"Desktop renderer missing");
            for(int i=0;i<1200 && renderer->property("images").toMap().size()<3 && renderer->property("errors").toMap().isEmpty();++i) QTest::qWait(100);
            require(renderer->property("errors").toMap().isEmpty(),"Desktop rendering reported an error");
            require(renderer->property("images").toMap().size()==3,"Three real material previews did not load");
            window->resize(1630,980); QTest::qWait(200);
            QQuickPaintedItem *image=nullptr;
            for(auto quick:window->findChildren<QQuickWidget *>())
                for(auto item:quick->rootObject()->findChildren<QQuickPaintedItem *>())
                    if(item->objectName()==QStringLiteral("actualDesktopImage")) image=item;
            for(auto quick:window->findChildren<QQuickWidget *>())
                for(auto card:items(quick->rootObject())) if(card->objectName().startsWith("material-")) {
                    for(auto child:items(card)) if(child->objectName()=="materialDesktopImage") {
                        const auto bottom=child->mapToItem(card,QPointF(0,child->height())).y();
                        require(bottom+20<=card->height(),"Material image overflows its card or crowds its label");
                    }
                }
            require(image && image->isVisible() && !image->property("source").toUrl().isEmpty(),"Main desktop image is missing");
            QImage rounded(QSize(qCeil(image->width()),qCeil(image->height())),QImage::Format_ARGB32_Premultiplied); rounded.fill(Qt::transparent);
            { QPainter painter(&rounded); image->paint(&painter); }
            require(qAlpha(rounded.pixel(0,0))==0 && qAlpha(rounded.pixel(rounded.width()-1,0))==0 && qAlpha(rounded.pixel(0,rounded.height()-1))==0 && qAlpha(rounded.pixel(rounded.width()-1,rounded.height()-1))==0,"Desktop corners were not clipped");
            require(qAlpha(rounded.pixel(rounded.width()/2,rounded.height()/2))==255,"Desktop center is missing");
            int smooth=0; for(int y=0;y<16;++y) for(int x=0;x<16;++x) { const auto a=qAlpha(rounded.pixel(x,y)); if(a>0 && a<255) ++smooth; }
            require(smooth>5,"Rounded edges are not antialiased");
            rounded.save(qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY")+"/desktop-rounded.png");
            QTest::qWait(500); capture(QStringLiteral("real-desktop-preview"));
            fprintf(stderr,"PASS: real desktop images and antialiased rounded corners\n");
            QCoreApplication::exit(0); return;
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB")) {
            QObject *button = nullptr;
            QObject *dialog = nullptr;
            QObject *hub = nullptr;
            QQuickItem *apply = nullptr;
            QQuickItem *reset = nullptr;
            QQuickItem *light = nullptr;
            QQuickItem *dark = nullptr;
            QQuickItem *selection = nullptr;
            QQuickItem *presets = nullptr;
            QQuickItem *schedule = nullptr;
            QQuickItem *more = nullptr;
            QQuickItem *getThemes = nullptr;
            QQuickItem *related = nullptr;
            for (auto quick : window->findChildren<QQuickWidget *>()) {
                if (!quick->rootObject()) continue;
                for (auto item : items(quick->rootObject())) {
                    if (item->objectName() == QStringLiteral("desktopTrialButton")) button = item;
                    if (item->objectName() == QStringLiteral("appearanceApplyButton")) apply = item;
                    if (item->objectName() == QStringLiteral("appearanceResetButton")) reset = item;
                    if (item->objectName() == QStringLiteral("appearanceMode-light")) light = item;
                    if (item->objectName() == QStringLiteral("appearanceMode-dark")) dark = item;
                    if (item->objectName() == QStringLiteral("appearanceModeSelection")) selection = item;
                    if (item->objectName() == QStringLiteral("appearancePresets")) presets = item;
                    if (item->objectName() == QStringLiteral("appearanceScheduleButton")) schedule = item;
                    if (item->objectName() == QStringLiteral("appearanceMoreButton")) more = item;
                    if (item->objectName() == QStringLiteral("appearanceGetThemes")) getThemes = item;
                    if (item->objectName() == QStringLiteral("appearanceRelatedSettings")) related = item;
                    if (item->objectName() == QStringLiteral("appearanceHub")) {
                        hub = item;
                        dialog = item->property("trialDialog").value<QObject *>();
                        fprintf(stderr, "Hub dialog property type=%s pointer=%p\n", item->property("trialDialog").typeName(), static_cast<void *>(dialog));
                    }
                }
            }
            fprintf(stderr, "Trial button=%p dialog=%p\n", static_cast<void *>(button), static_cast<void *>(dialog));
            require(button && dialog, "Material trial controls missing");
            require(hub && apply && reset && apply->isVisible(), "Appearance Apply/Reset actions must be visible");
            require(!apply->isEnabled(), "Apply must start disabled with no pending changes");
            require(light && dark && selection, "Unified appearance pill missing");
            for (auto modeButton : {dark, light}) {
                QMetaObject::invokeMethod(modeButton,"clicked");
                QTest::qWait(220);
                require(hub->property("modeEdited").toBool(),"Mode pill did not stage its selection");
                const auto buttonCenter=modeButton->mapToScene(QPointF(modeButton->width()/2,modeButton->height()/2));
                const auto selectionCenter=selection->mapToScene(QPointF(selection->width()/2,selection->height()/2));
                require(qAbs(buttonCenter.x()-selectionCenter.x())<2,"Glass selection did not align with its selected mode");
            }
            QMetaObject::invokeMethod(reset,"clicked");
            QTest::qWait(220);
            fprintf(stderr,"PASS: unified mode pill stages changes and glass selection aligns after animation\n");
            require(presets && schedule && presets->height()>=40 && schedule->height()>=40,"Theme and Schedule pills missing or undersized");
            require(more && getThemes && qAbs(more->width()-more->height())<1
                    && qAbs(more->height()-getThemes->height())<1,"More button must be square and match adjacent pills");
            auto moreMenu=more->property("menu").value<QObject*>();
            require(moreMenu,"More menu missing");
            auto clickMore=[&] {
                QTest::mouseClick(more->window(),Qt::LeftButton,Qt::NoModifier,
                                  more->mapToScene(QPointF(more->width()/2,more->height()/2)).toPoint());
                QTest::qWait(180);
            };
            QSignalSpy menuOpened(moreMenu,SIGNAL(opened()));
            for(int i=0;i<3;++i) {
                clickMore();
                require(moreMenu->property("visible").toBool(),"First click did not open More menu");
                auto background=qobject_cast<QQuickItem*>(moreMenu->property("background").value<QObject*>());
                require(background,"More menu background missing");
                const auto right=more->mapToGlobal(QPointF(more->width(),more->height()));
                const auto menuRight=background->mapToGlobal(QPointF(background->width(),0));
                require(qAbs(right.x()-menuRight.x())<2 && menuRight.y()>=right.y()+4,
                        "More menu must extend left from the button's right edge");
                if(i==0) {
                    auto content=qobject_cast<QQuickItem*>(moreMenu->property("contentItem").value<QObject*>());
                    require(content,"More menu content missing");
                    int pillCount=0;QQuickItem *firstOption=nullptr;
                    for(auto option:items(content)) {
                        auto highlight=qobject_cast<QQuickItem*>(option->property("background").value<QObject*>());
                        if(!highlight || highlight->objectName()!="sequoiaPillHighlight")continue;
                        ++pillCount;
                        require(qAbs(highlight->property("radius").toReal()-highlight->height()/2)<1,"More menu highlight must be pill-shaped");
                        if(!firstOption)firstOption=option;
                    }
                    require(pillCount==3 && firstOption,"More menu must use the shared pill highlight on every option");
                    QTest::mouseMove(more->window(),firstOption->mapToScene(QPointF(firstOption->width()/2,firstOption->height()/2)).toPoint());
                    QTest::qWait(200);
                    require(firstOption->property("hovered").toBool(),"More menu highlight did not respond to hover");
                    capture(QStringLiteral("appearance-more-menu"));
                }
                clickMore();
                require(!moreMenu->property("visible").toBool(),"Second click reopened More menu");
                require(menuOpened.count()==i+1,"More menu flashed open twice");
            }
            clickMore();
            QTest::keyClick(more->window(),Qt::Key_Escape);QTest::qWait(180);
            require(!moreMenu->property("visible").toBool(),"Escape did not close More menu");
            clickMore();
            QTest::mouseClick(more->window(),Qt::LeftButton,Qt::NoModifier,QPoint(5,5));QTest::qWait(180);
            require(!moreMenu->property("visible").toBool(),"Outside click did not close More menu");
            fprintf(stderr,"PASS: circular More button, right-aligned popup, repeated click toggle, Escape and outside dismissal\n");
            QQuickItem *showMenuButton=nullptr;
            for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->rootObject()))
                if(item->objectName()=="settingsShowMenu")showMenuButton=item;
            require(showMenuButton,"Sidebar Show Menu trigger missing");
            require(qAbs(showMenuButton->width()-showMenuButton->height())<1 && qAbs(showMenuButton->height()-searchField->height())<1,"Show Menu must be square and match the search field height");
            const auto searchCenter=searchField->mapToGlobal(QPointF(searchField->width()/2,searchField->height()/2));
            const auto menuCenter=showMenuButton->mapToGlobal(QPointF(showMenuButton->width()/2,showMenuButton->height()/2));
            require(qAbs(searchCenter.y()-menuCenter.y())<1,"Show Menu must share the search row");
            require(qApp->layoutDirection()==Qt::RightToLeft ? menuCenter.x()<searchCenter.x() : menuCenter.x()>searchCenter.x(),"Show Menu must follow the search field");
            auto sidebarMenu=showMenuButton->property("menu").value<QObject*>();require(sidebarMenu,"Sidebar popup missing");
            auto clickSidebarMenu=[&] {
                QTest::mouseClick(showMenuButton->window(),Qt::LeftButton,Qt::NoModifier,
                    showMenuButton->mapToScene(QPointF(showMenuButton->width()/2,showMenuButton->height()/2)).toPoint());QTest::qWait(180);
            };
            QSignalSpy sidebarOpened(sidebarMenu,SIGNAL(opened()));
            for(int i=0;i<3;++i){
                clickSidebarMenu();require(sidebarMenu->property("visible").toBool(),"Show Menu did not open");
                require(sidebarMenu->property("width").toReal()>150 && sidebarMenu->property("height").toReal()>150,"Sidebar popup collapsed");
                if(i==0) {
                    auto content=qobject_cast<QQuickItem*>(sidebarMenu->property("contentItem").value<QObject*>());
                    require(content,"Sidebar popup content missing");
                    int options=0;
                    for(auto option:items(content)) {
                        if(!option->objectName().startsWith("settingsAction-") || !option->isVisible())continue;
                        auto highlight=qobject_cast<QQuickItem*>(option->property("background").value<QObject*>());
                        require(highlight && highlight->objectName()=="sequoiaPillHighlight" && qAbs(highlight->property("radius").toReal()-highlight->height()/2)<1,"Show Menu must use the same pill highlight as More");
                        ++options;
                    }
                    require(options>0,"Sidebar menu options missing");
                    capture(QStringLiteral("sidebar-show-menu"));
                }
                clickSidebarMenu();require(!sidebarMenu->property("visible").toBool(),"Show Menu reopened after second click");
                require(sidebarOpened.count()==i+1,"Show Menu flashed open");
            }
            clickSidebarMenu();QTest::keyClick(showMenuButton->window(),Qt::Key_Escape);QTest::qWait(180);
            require(!sidebarMenu->property("visible").toBool(),"Escape did not dismiss sidebar popup");
            clickSidebarMenu();QTest::mouseClick(showMenuButton->window(),Qt::LeftButton,Qt::NoModifier,QPoint(5,5));QTest::qWait(180);
            require(!sidebarMenu->property("visible").toBool(),"Outside click did not dismiss sidebar popup");
            clickSidebarMenu();
            QQuickItem *highlightAction=nullptr;
            for(auto item:items(showMenuButton->window()->contentItem()))if(item->objectName()=="settingsAction-highlight_changes")highlightAction=item;
            require(highlightAction && highlightAction->isVisible(),"Native checkable action missing");
            auto nativeAction=highlightAction->property("nativeAction").value<QObject*>();require(nativeAction,"Native QAction missing");
            const bool originalChecked=nativeAction->property("checked").toBool();
            QSignalSpy nativeTriggered(nativeAction,SIGNAL(triggered(bool)));
            for(int i=0;i<2;++i){
                QTest::mouseClick(highlightAction->window(),Qt::LeftButton,Qt::NoModifier,highlightAction->mapToScene(QPointF(highlightAction->width()/2,highlightAction->height()/2)).toPoint());QTest::qWait(180);
                require(nativeAction->property("checked").toBool()==(i==0 ? !originalChecked : originalChecked),"Native menu check state did not toggle");
                require(nativeTriggered.count()==i+1,"Native QAction was triggered more than once");
                if(i==0){clickSidebarMenu();require(highlightAction->property("checked").toBool()==!originalChecked,"Checkmark did not follow QAction");}
            }
            fprintf(stderr,"PASS: shared rounded sidebar popup, repeated toggle without flashing, Escape/outside dismissal and native checkable actions\n");
            require(related && qAbs(related->height()-getThemes->height())<1,"Related settings must match the action pills");
            require(qAbs(related->width()-related->height())<1,"Related settings must be circular");
            QQuickItem *mainScroll=nullptr,*componentPill=nullptr,*accentBar=nullptr,*panelIsland=nullptr,*optionsRow=nullptr;
            for(auto item:items(related->window()->contentItem())) {
                if(item->objectName()=="appearanceMainContent")mainScroll=item;
                if(item->objectName()=="appearanceComponentsPanel")componentPill=item;
                if(item->objectName()=="appearanceAccentBar")accentBar=item;
                if(item->objectName()=="appearancePanelSettingsPanel")panelIsland=item;
                if(item->objectName()=="appearanceOptionsRow")optionsRow=item;
            }
            require(mainScroll && componentPill && accentBar && panelIsland && optionsRow,"Appearance pill stack missing");
            require(componentPill->mapToScene(QPointF()).y()>optionsRow->mapToScene(QPointF(0,optionsRow->height())).y() && accentBar->mapToScene(QPointF()).y()>=componentPill->mapToScene(QPointF(0,componentPill->height())).y() && panelIsland->mapToScene(QPointF()).y()>=accentBar->mapToScene(QPointF(0,accentBar->height())).y(),"Appearance and accent pills must sit between preview options and panel settings");
            require(qAbs(componentPill->property("radius").toReal()-componentPill->height()/2)<1,"Appearance component bar must be pill-shaped");
            auto mainFlick=mainScroll->property("contentItem").value<QObject*>();require(mainFlick,"Main scroll content missing");
            auto revealRelated=[&](bool low) {
                const qreal top=mainScroll->mapToScene(QPointF()).y();
                const qreal target=low ? top+mainScroll->height()-related->height()-12 : top+100;
                const qreal y=mainFlick->property("contentY").toReal()+related->mapToScene(QPointF()).y()-target;
                mainFlick->setProperty("contentY",qBound(0.0,y,qMax(0.0,mainFlick->property("contentHeight").toReal()-mainScroll->height())));QTest::qWait(150);
            };
            auto relatedMenu=related->property("menu").value<QObject*>();
            require(relatedMenu,"Related settings menu missing");
            auto clickRelated=[&] {
                QTest::mouseClick(related->window(),Qt::LeftButton,Qt::NoModifier,
                                  related->mapToScene(QPointF(related->width()/2,related->height()/2)).toPoint());
                QTest::qWait(180);
            };
            QSignalSpy relatedOpened(relatedMenu,SIGNAL(opened()));
            for(int h : {980,650}) {
                window->resize(1100,h);QTest::qWait(250);revealRelated(h==650);
                for(int i=0;i<2;++i) {
                    const int opens=relatedOpened.count();
                    clickRelated();
                    require(relatedMenu->property("visible").toBool(),"Related settings did not open");
                    auto bg=qobject_cast<QQuickItem*>(relatedMenu->property("background").value<QObject*>());
                    require(bg,"Related settings glass background missing");
                    const auto buttonTop=related->mapToGlobal(QPointF());
                    const auto buttonBottom=related->mapToGlobal(QPointF(related->width(),related->height()));
                    const auto menuTop=bg->mapToGlobal(QPointF());
                    const auto menuBottom=bg->mapToGlobal(QPointF(bg->width(),bg->height()));
                    require(qAbs(menuBottom.x()-buttonBottom.x())<2,"Related menu lost right-edge alignment");
                    if(h==980) require(menuTop.y()>=buttonBottom.y()+4,"Related menu overlaps its button despite room below");
                    else require(menuBottom.y()<=buttonTop.y()-4,"Related menu must open above when space below is insufficient");
                    if(i==0) {
                        auto content=qobject_cast<QQuickItem*>(relatedMenu->property("contentItem").value<QObject*>());
                        int count=0;for(auto option:items(content)) {
                            auto highlight=qobject_cast<QQuickItem*>(option->property("background").value<QObject*>());
                            if(highlight && highlight->objectName()=="sequoiaPillHighlight")++count;
                        }
                        require(count==4,"Related settings must use pill glass highlights");
                        capture(h==980 ? QStringLiteral("related-menu-below") : QStringLiteral("related-menu-above"));
                    }
                    clickRelated();
                    require(!relatedMenu->property("visible").toBool() && relatedOpened.count()==opens+1,
                            "Related settings toggle flashed or reopened");
                }
            }
            window->resize(1100,790);QTest::qWait(200);
            clickRelated();QTest::keyClick(related->window(),Qt::Key_Escape);QTest::qWait(180);
            require(!relatedMenu->property("visible").toBool(),"Escape did not dismiss Related settings");
            clickRelated();QTest::mouseClick(related->window(),Qt::LeftButton,Qt::NoModifier,QPoint(5,5));QTest::qWait(180);
            require(!relatedMenu->property("visible").toBool(),"Outside click did not dismiss Related settings");
            fprintf(stderr,"PASS: Related settings pill, below/above placement, repeated toggle, Escape and outside dismissal\n");
            mainFlick->setProperty("contentY",0);QTest::qWait(200);
            auto popup=presets->property("popup").value<QObject*>();
            require(popup,"Theme dropdown popup missing");
            auto openThemePopup=[&] { QMetaObject::invokeMethod(popup,"open"); QTest::qWait(250); };
            openThemePopup();
            auto list=qobject_cast<QQuickItem*>(popup->property("contentItem").value<QObject*>());
            auto surface=qobject_cast<QQuickItem*>(popup->property("background").value<QObject*>());
            require(list && surface && popup->property("visible").toBool(),"Theme dropdown did not open");
            const auto buttonBottom=presets->mapToGlobal(QPointF(0,presets->height()));
            const auto popupTop=surface->mapToGlobal(QPointF());
            require(qAbs(popupTop.x()-buttonBottom.x())<2 && popupTop.y()>=buttonBottom.y()+4,"Theme dropdown is not directly below its pill");
            require(qAbs(surface->width()-presets->width())<2 && surface->height()<=362,"Theme popup size does not fit its pill or viewport");
            require(list->property("contentHeight").toReal()>list->height(),"Long theme list did not become scrollable");
            if(qEnvironmentVariableIsSet("SEQUOIA_HUB_COMPOSITED")) {
                composedCapture(QStringLiteral("theme-dropdown"));
                auto glass=surface->findChild<QQuickItem*>(QStringLiteral("appearanceThemePopupGlass"));
                require(glass && glass->isVisible(),"Popup glass shader is missing");
                const auto withGlass=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
                glass->setVisible(false);QTest::qWait(200);
                const auto withoutGlass=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
                glass->setVisible(true);QTest::qWait(200);
                double difference=0;
                for(int y=0;y<withGlass.height();++y) for(int x=0;x<withGlass.width();++x) {
                    const auto a=withGlass.pixel(x,y),b=withoutGlass.pixel(x,y);
                    difference+=qAbs(qRed(a)-qRed(b))+qAbs(qGreen(a)-qGreen(b))+qAbs(qBlue(a)-qBlue(b));
                }
                require(difference>1000,"Popup shader did not visibly sample the underlying page");
                fprintf(stderr,"PASS: popup samples and refracts actual page content (pixel difference %.0f)\n",difference);
            } else capture(QStringLiteral("theme-dropdown"));
            QTest::keyClick(list->window(),Qt::Key_Escape);
            QTest::qWait(150);
            require(!popup->property("visible").toBool() && presets->property("currentIndex").toInt()==0,"Escape changed the theme or did not close the dropdown");
            openThemePopup();
            QTest::keyClick(list->window(),Qt::Key_Down);
            QTest::keyClick(list->window(),Qt::Key_Return);
            QTest::qWait(200);
            require(!popup->property("visible").toBool() && presets->property("currentIndex").toInt()==1 && hub->property("modeEdited").toBool(),"Keyboard selection did not stage a saved look");
            QMetaObject::invokeMethod(reset,"clicked");
            window->resize(800,600);QTest::qWait(300);
            openThemePopup();
            const auto compactBottom=presets->mapToGlobal(QPointF(0,presets->height()));
            const auto compactTop=surface->mapToGlobal(QPointF());
            require(qAbs(compactTop.x()-compactBottom.x())<2 && compactTop.y()>=compactBottom.y()+4,"Compact theme popup lost its below-button alignment");
            QTest::mouseClick(list->window(),Qt::LeftButton,Qt::NoModifier,QPoint(5,5));
            QTest::qWait(150);
            require(!popup->property("visible").toBool(),"Clicking outside did not dismiss the theme popup");
            window->resize(1100,790);QTest::qWait(200);
            fprintf(stderr,"PASS: below-button glass popup, scrolling, Escape and keyboard preset selection\n");
            // Accent changes are tested only in this harness's temporary XDG home.
            auto accentController=window->findChild<QObject*>("appearancePreview");require(accentController,"Accent controller missing");
            KConfig accentConfig(QStringLiteral("kdeglobals"),KConfig::SimpleConfig);
            auto general=accentConfig.group(QStringLiteral("General"));
            const auto savedScheme=general.readEntry("ColorScheme",QString());
            general.deleteEntry("ColorScheme");
            general.writeEntry("AccentColor",QColor("#3daee9"));
            general.writeEntry("accentColorFromWallpaper",true);
            accentConfig.group(QStringLiteral("KDE")).writeEntry("AutomaticLookAndFeel",true);
            accentConfig.sync();QTest::qWait(200);
            KConfig effectiveAccentConfig(QStringLiteral("kdeglobals"),KConfig::FullConfig);
            require(effectiveAccentConfig.group(QStringLiteral("General")).readEntry("ColorScheme",QString())==QStringLiteral("BreezeDark") && !general.hasKey("ColorScheme"),"Inherited color scheme fixture is not active");
            const auto paletteBefore=accentConfig.group(QStringLiteral("Colors:Selection")).readEntry("BackgroundNormal",QColor());
            const auto materialBefore=accentController->property("activeMaterial");
            const auto alphaBefore=accentConfig.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal",QColor()).alpha();
            QQuickItem *purple=nullptr;for(auto item:items(related->window()->contentItem()))if(item->objectName()=="appearanceAccent-9b59b6")purple=item;
            require(purple && purple->isEnabled(),"Accent swatch missing or unexpectedly disabled");
            mainFlick->setProperty("contentY",qMax(0.0,mainFlick->property("contentHeight").toReal()-mainScroll->height()));QTest::qWait(100);
            QTest::mouseClick(purple->window(),Qt::LeftButton,Qt::NoModifier,purple->mapToScene(QPointF(purple->width()/2,purple->height()/2)).toPoint());
            accentConfig.reparseConfiguration();
            require(apply->isEnabled() && reset->isEnabled() && accentController->property("accentPending").toBool() && !accentController->property("accentBusy").toBool() && purple->property("selected").toBool(),"Selecting a swatch did not stage it and enable Apply");
            require(general.readEntry("AccentColor",QColor())==QColor("#3daee9") && general.readEntry("accentColorFromWallpaper",false),"Selecting an accent applied before the footer Apply button");
            QMetaObject::invokeMethod(reset,"clicked");QTest::qWait(100);
            require(!accentController->property("accentPending").toBool() && !apply->isEnabled(),"Reset did not discard the pending accent");
            QTest::mouseClick(purple->window(),Qt::LeftButton,Qt::NoModifier,purple->mapToScene(QPointF(purple->width()/2,purple->height()/2)).toPoint());
            general.writeEntry("ColorScheme","missing-accent-test-scheme");accentConfig.sync();QTest::qWait(150);
            QMetaObject::invokeMethod(apply,"clicked");QTest::qWait(100);
            require(accentController->property("accentPending").toBool() && apply->isEnabled(),"Failed accent application discarded the pending choice");
            general.deleteEntry("ColorScheme");accentConfig.sync();QTest::qWait(150);
            QTest::mouseClick(apply->window(),Qt::LeftButton,Qt::NoModifier,apply->mapToScene(QPointF(apply->width()/2,apply->height()/2)).toPoint());
            for(int i=0;i<400 && accentController->property("accentBusy").toBool();++i)QTest::qWait(25);
            require(!accentController->property("accentBusy").toBool(),"Accent application timed out");QTest::qWait(250);
            accentConfig.reparseConfiguration();
            require(accentConfig.group(QStringLiteral("General")).readEntry("AccentColor",QColor())==QColor("#9b59b6") && purple->property("selected").toBool(),"Accent swatch did not persist or update its selection");
            require(accentController->property("activeMaterial")==materialBefore && accentConfig.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal",QColor()).alpha()==alphaBefore,"Accent application changed the surface material");
            require(!accentController->property("accentPending").toBool() && !apply->isEnabled() && !general.readEntry("accentColorFromWallpaper",true) && general.readEntry("LastUsedCustomAccentColor",QColor())==QColor("#9b59b6"),"Apply did not commit the custom accent source and clear pending state");
            require(accentConfig.group(QStringLiteral("KDE")).readEntry("AutomaticLookAndFeel",false),"Manual accent selection disabled unrelated automatic light/dark mode");
            const auto appliedSelection=accentConfig.group(QStringLiteral("Colors:Selection")).readEntry("BackgroundNormal",QColor());
            require(appliedSelection!=paletteBefore && qApp->palette().highlight().color()==appliedSelection,"Accent did not update the system palette and running application");
            // Selecting the same color must still let users leave automatic accent mode.
            general.writeEntry("accentColorFromWallpaper",true);accentConfig.sync();QTest::qWait(150);
            QTest::mouseClick(purple->window(),Qt::LeftButton,Qt::NoModifier,purple->mapToScene(QPointF(purple->width()/2,purple->height()/2)).toPoint());
            require(apply->isEnabled(),"Choosing the current accent did not allow switching from automatic to custom");
            QMetaObject::invokeMethod(apply,"clicked");
            for(int i=0;i<400 && accentController->property("accentBusy").toBool();++i)QTest::qWait(25);
            QTest::qWait(200);accentConfig.reparseConfiguration();
            require(!accentController->property("accentPending").toBool() && !general.readEntry("accentColorFromWallpaper",true),"Same-color Apply left automatic accent mode active");
            fprintf(stderr,"PASS: staged accent Apply/Reset, inherited scheme, failure retry, system palette and automatic-to-custom source switch\n");
            QMetaObject::invokeMethod(accentController,"applyAccent",Q_ARG(QString,QString("invalid-color")));
            require(!accentController->property("accentBusy").toBool(),"Invalid accent color started a process");
            QQuickItem *customAccent=nullptr,*accentTitle=nullptr,*panelTitle=nullptr;
            for(auto item:items(related->window()->contentItem())) {
                if(item->objectName()=="appearanceCustomAccent")customAccent=item;
                if(item->objectName()=="appearanceAccentTitle")accentTitle=item;
                if(item->objectName()=="appearancePanelSettingsTitle")panelTitle=item;
            }
            require(customAccent && accentTitle && panelTitle,"Custom accent chooser or section titles missing");
            require(accentTitle->mapToScene(QPointF(0,accentTitle->height())).y()<accentBar->mapToScene(QPointF()).y() && panelTitle->mapToScene(QPointF(0,panelTitle->height())).y()<panelIsland->mapToScene(QPointF()).y(),"Section titles must sit outside and above their surfaces");
            auto colorDialog=customAccent->property("dialog").value<QObject*>();require(colorDialog,"Custom accent dialog missing");
            auto openCustomAccent=[&] {
                QTest::mouseClick(customAccent->window(),Qt::LeftButton,Qt::NoModifier,customAccent->mapToScene(QPointF(customAccent->width()/2,customAccent->height()/2)).toPoint());
                QTest::qWait(250);
                require(colorDialog->property("visible").toBool(),"Custom accent chooser did not open");
                require(colorDialog->property("selectedColor").value<QColor>()==accentController->property("accentColor").value<QColor>(),"Custom chooser did not start with the active accent");
            };
            openCustomAccent();
            QQuickItem *colorSurface=nullptr,*colorSpectrum=nullptr,*colorHex=nullptr,*colorCancel=nullptr,*colorApply=nullptr,*colorOption=nullptr;
            int colorOptionCount=0;
            for(auto item:items(customAccent->window()->contentItem())) {
                if(item->objectName()=="appearanceCustomAccentSurface")colorSurface=item;
                if(item->objectName()=="appearanceCustomAccentSpectrum")colorSpectrum=item;
                if(item->objectName()=="appearanceCustomAccentHex")colorHex=item;
                if(item->objectName()=="appearanceCustomAccentCancel")colorCancel=item;
                if(item->objectName()=="appearanceCustomAccentApply")colorApply=item;
                if(item->objectName().startsWith("appearanceCustomAccentOption-")) {
                    ++colorOptionCount;
                    auto background=item->property("background").value<QQuickItem*>();
                    require(qAbs(item->width()-item->height())<1 && background && qAbs(background->property("radius").toReal()*2-background->height())<1,"Color options must be circular");
                    if(item->objectName()=="appearanceCustomAccentOption-27ae60")colorOption=item;
                }
            }
            require(colorSurface && colorSpectrum && colorHex && colorCancel && colorApply && colorOption && colorOptionCount==12,"Themed color chooser controls missing");
            auto clickColorControl=[&](QQuickItem *item) { QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(100); };
            clickColorControl(colorOption);
            require(colorDialog->property("selectedColor").value<QColor>()==QColor("#27ae60"),"Circular color option did not select a color");
            clickColorControl(colorSpectrum);
            const auto spectrumColor=colorDialog->property("selectedColor").value<QColor>();
            require(qAbs(spectrumColor.hsvSaturationF()-.5)<.02 && qAbs(spectrumColor.valueF()-.5)<.02,"Spectrum pointer input did not select saturation/brightness");
            QTest::keyClick(colorSpectrum->window(),Qt::Key_Up);QTest::qWait(50);
            require(colorDialog->property("selectedColor").value<QColor>().valueF()>spectrumColor.valueF(),"Spectrum keyboard input did not adjust brightness");
            auto enterHex=[&](const char *text) { clickColorControl(colorHex);QTest::keyClick(colorHex->window(),Qt::Key_A,Qt::ControlModifier);for(const char *ch=text;*ch;++ch)QTest::keyClick(colorHex->window(),*ch);QTest::qWait(100); };
            enterHex("#46");require(!colorApply->isEnabled(),"Incomplete hex color allowed applying");
            enterHex("#468f82");require(colorApply->isEnabled() && colorDialog->property("selectedColor").value<QColor>()==QColor("#468f82"),"Hex entry did not update the selected color");
            auto chooserTheme=colorDialog->property("theme").value<QObject*>();require(chooserTheme,"Color chooser theme missing");
            for(const auto &material: {QString("solid"),QString("transparent"),QString("glass")}) {
                chooserTheme->setProperty("material",material);
                for(const auto &mode: {QString("light"),QString("dark")}) {
                    chooserTheme->setProperty("mode",mode);QTest::qWait(150);
                    require(colorSurface->property("radius").toReal()==24,"Color chooser rounded surface lost its radius");
                    capture(QString("custom-accent-%1-%2").arg(material,mode));
                }
            }
            auto chooserBlur=colorSurface->findChild<QQuickItem*>(QStringLiteral("appearanceCustomAccentBlur"));
            require(chooserBlur && chooserBlur->isVisible(),"Glass chooser backdrop blur missing");
            const auto blurredChooser=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
            chooserBlur->setProperty("blurEnabled",false);QTest::qWait(200);
            const auto clearChooser=window->grab().toImage().convertToFormat(QImage::Format_RGB32);
            chooserBlur->setProperty("blurEnabled",true);QTest::qWait(200);
            double chooserBlurDifference=0;
            for(int y=0;y<blurredChooser.height();++y) for(int x=0;x<blurredChooser.width();++x) {
                const auto a=blurredChooser.pixel(x,y),b=clearChooser.pixel(x,y);
                chooserBlurDifference+=qAbs(qRed(a)-qRed(b))+qAbs(qGreen(a)-qGreen(b))+qAbs(qBlue(a)-qBlue(b));
            }
            require(chooserBlurDifference>1000,"Color chooser did not visibly blur the underlying page");
            fprintf(stderr,"PASS: color chooser blur changes underlying page detail (pixel difference %.0f), six theme variants and circular options\n",chooserBlurDifference);
            chooserTheme->setProperty("material",materialBefore);chooserTheme->setProperty("mode","system");
            window->resize(800,600);QTest::qWait(200);
            require(colorSurface->mapToScene(QPointF()).y()>=0 && colorSurface->mapToScene(QPointF(0,colorSurface->height())).y()<=colorSurface->window()->height(),"Compact color chooser overflows the window");
            capture(QStringLiteral("custom-accent-compact"));
            clickColorControl(colorCancel);QTest::qWait(150);
            window->resize(1100,790);QTest::qWait(200);
            mainFlick->setProperty("contentY",qMax(0.0,mainFlick->property("contentHeight").toReal()-mainScroll->height()));QTest::qWait(100);
            accentConfig.reparseConfiguration();
            require(!colorDialog->property("visible").toBool() && !accentController->property("accentBusy").toBool() && accentConfig.group(QStringLiteral("General")).readEntry("AccentColor",QColor())==QColor("#9b59b6"),"Cancel applied the custom accent");
            openCustomAccent();
            enterHex("#468f82");
            clickColorControl(colorApply);
            accentConfig.reparseConfiguration();
            require(accentController->property("accentPending").toBool() && apply->isEnabled() && !accentController->property("accentBusy").toBool() && general.readEntry("AccentColor",QColor())==QColor("#9b59b6"),"Custom chooser must stage its color for footer Apply");
            QTest::qWait(250); // Let the chooser's modal exit transition finish.
            clickColorControl(apply);
            for(int i=0;i<400 && accentController->property("accentBusy").toBool();++i)QTest::qWait(25);
            QTest::qWait(250);accentConfig.reparseConfiguration();
            require(!colorDialog->property("visible").toBool() && !accentController->property("accentBusy").toBool() && accentConfig.group(QStringLiteral("General")).readEntry("AccentColor",QColor())==QColor("#468f82") && accentController->property("accentColor").value<QColor>()==QColor("#468f82"),"Custom accent did not apply and persist");
            require(accentController->property("activeMaterial")==materialBefore && accentConfig.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal",QColor()).alpha()==alphaBefore,"Custom accent changed the surface material");
            fprintf(stderr,"PASS: external accent/panel titles and custom color chooser open, cancel, accept and persistence\n");
            QTest::mouseMove(purple->window(),QPoint(5,5));QTest::qWait(700);
            capture(QStringLiteral("appearance-components-accent"));
            QQuickItem *iconCard=nullptr,*iconSelector=nullptr;
            for(auto item:items(related->window()->contentItem())) {
                if(item->objectName()=="appearanceIconPackCard")iconCard=item;
                if(item->objectName()=="appearanceIconPackSelector")iconSelector=item;
            }
            require(iconCard && iconSelector,"Quick icon pack card missing");
            require(iconCard->mapToScene(QPointF()).y()>accentBar->mapToScene(QPointF(0,accentBar->height())).y() && iconCard->mapToScene(QPointF(0,iconCard->height())).y()<panelIsland->mapToScene(QPointF()).y(),"Icon pack card must sit below Accent color and above Panel settings");
            require(qAbs(iconCard->mapToScene(QPointF()).x()-accentBar->mapToScene(QPointF()).x())<1,"Icon pack card must align with the accent bar");
            const auto packs=accentController->property("iconPacks").toList();
            const auto originalPack=accentController->property("iconPack").toString();
            require(packs.size()>=2,"Installed icon pack discovery returned too few packs");
            for(const auto &pack:packs) {
                const auto id=pack.toMap().value("id").toString();
                require(id!="hicolor" && id!="Adwaita" && id!="HighContrast","Internal or unsupported icon theme was offered");
                require(pack.toMap().value("samples").toList().size()==5,"Icon pack samples missing");
            }
            const int targetPack=packs.first().toMap().value("id").toString()==originalPack ? packs.size()-1 : 0;
            const auto targetId=packs.at(targetPack).toMap().value("id").toString();
            mainFlick->setProperty("contentY",qMax(0.0,mainFlick->property("contentHeight").toReal()-mainScroll->height()));QTest::qWait(100);
            clickColorControl(iconSelector);
            auto iconsPopup=iconSelector->property("popup").value<QObject*>();require(iconsPopup && iconsPopup->property("visible").toBool(),"Icon pack dropdown did not open");
            capture(QStringLiteral("appearance-icon-pack-menu"));
            QTest::keyClick(iconSelector->window(),targetPack==0 ? Qt::Key_Home : Qt::Key_End);
            QTest::keyClick(iconSelector->window(),Qt::Key_Return);QTest::qWait(650);
            accentConfig.reparseConfiguration();
            require(!iconsPopup->property("visible").toBool() && accentController->property("iconPack").toString()==targetId && accentConfig.group(QStringLiteral("Icons")).readEntry("Theme",QString())==targetId,"Quick icon pack selection did not apply and persist");
            require(iconSelector->property("currentIndex").toInt()==targetPack,"Icon pack selector did not reflect the applied pack");
            require(accentController->property("activeMaterial")==materialBefore && accentConfig.group(QStringLiteral("General")).readEntry("AccentColor",QColor())==QColor("#468f82"),"Icon pack switching changed the material or accent");
            capture(QStringLiteral("appearance-icon-pack-card"));
            QMetaObject::invokeMethod(accentController,"applyIconPack",Q_ARG(QString,QString("../missing-icon-pack")));QTest::qWait(100);
            accentConfig.reparseConfiguration();
            require(accentConfig.group(QStringLiteral("Icons")).readEntry("Theme",QString())==targetId,"Unknown icon pack changed the current theme");
            accentConfig.group(QStringLiteral("Icons")).writeEntry("Theme",originalPack);accentConfig.sync();QTest::qWait(400);
            require(accentController->property("iconPack").toString()==originalPack,"External icon theme changes did not refresh the card");
            fprintf(stderr,"PASS: quick icon pack card placement, installed packs, keyboard switching, persistence, material/accent preservation and external refresh\n");
            if(savedScheme.isEmpty())general.deleteEntry("ColorScheme");else general.writeEntry("ColorScheme",savedScheme);accentConfig.sync();
            mainFlick->setProperty("contentY",0);QTest::qWait(100);
            fprintf(stderr,"PASS: appearance icon pill, circular Related settings, pill menu highlights, live accent selection and preserved material alpha\n");
            if(qEnvironmentVariableIsSet("SEQUOIA_POPUP_ONLY")) { qApp->quit(); return; }
            hub->setProperty("selectedMaterial", hub->property("selectedMaterial").toString()==QStringLiteral("glass") ? QStringLiteral("solid") : QStringLiteral("glass"));
            QTest::qWait(50);
            require(apply->isEnabled() && reset->isEnabled(), "Material selection did not enable Apply and Reset");
            QMetaObject::invokeMethod(reset,"clicked");
            QTest::qWait(50);
            require(!apply->isEnabled(), "Reset did not discard staged appearance choices");
            QMetaObject::invokeMethod(button, "clicked");
            QTest::qWait(200);
            require(dialog->property("visible").toBool(), "Material confirmation did not open");
            capture(QStringLiteral("material-confirmation"));
            QMetaObject::invokeMethod(dialog, "reject");
            QTest::qWait(100);
            require(!window->findChild<QObject *>(QStringLiteral("appearancePreview"))->property("busy").toBool(), "Cancel started a desktop trial");
        }
        if (qEnvironmentVariableIsSet("SEQUOIA_HUB_COMPOSITED")) {
            auto controller=window->findChild<QObject *>(QStringLiteral("appearancePreview"));
            Plasma::Theme livePlasmaTheme;
            QTest::qWait(1400);
            fprintf(stderr,"HUB_WINDOW_CLIENT visible=%d exposed=%d opacity=%.2f title=%s\n",window->isVisible(),window->windowHandle()->isExposed(),window->windowOpacity(),qPrintable(window->windowTitle()));
            window->grab().save(qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY")+"/client-buffer.png");
            require(window->property("appearanceBlurRequested").toBool(), "Preview did not request compositor blur");
            const auto withGlass=composedCapture(QStringLiteral("backdrop-glass"));
            auto disable=QDBusMessage::createMethodCall("org.kde.KWin","/Effects","org.kde.kwin.Effects","unloadEffect");
            disable << QStringLiteral("sequoia_liquid_glass_v6");
            QDBusConnection::sessionBus().call(disable);
            QTest::qWait(500);
            const auto withoutGlass=composedCapture(QStringLiteral("backdrop-disabled"));
            double total=0; int pixels=0;
            for(int y=0;y<withGlass.height();y+=2) for(int x=0;x<withGlass.width();x+=2) {
                const auto a=withGlass.pixelColor(x,y),b=withoutGlass.pixelColor(x,y);
                total+=qAbs(a.red()-b.red())+qAbs(a.green()-b.green())+qAbs(a.blue()-b.blue());pixels+=3;
            }
            require(total/pixels > .08, "Backdrop effect has no visible contribution");
            fprintf(stderr,"PASS: real compositor backdrop contribution %.3f RGB levels per channel\n",total/pixels);
            auto enable=QDBusMessage::createMethodCall("org.kde.KWin","/Effects","org.kde.kwin.Effects","loadEffect");
            enable << QStringLiteral("sequoia_liquid_glass_v6");
            QDBusConnection::sessionBus().call(enable);
            window->resize(1101,790);window->resize(1100,790);QTest::qWait(300);
            for(const QString &material : {QStringLiteral("transparent"),QStringLiteral("solid"),QStringLiteral("glass"),QStringLiteral("transparent"),QStringLiteral("glass")}) {
                QMetaObject::invokeMethod(controller,"trial",Q_ARG(QString,material));
                for(int i=0;i<150 && controller->property("seconds").toInt()==0;++i) QTest::qWait(100);
                require(controller->property("seconds").toInt()>0,"Material trial did not become ready");
                QMetaObject::invokeMethod(controller,"keep");
                for(int i=0;i<100 && controller->property("busy").toBool();++i) QTest::qWait(100);
                require(!controller->property("busy").toBool(),"Material keep did not finish");
                QTest::qWait(1400);
                const auto loaded=loadedProviders();
                require(loaded.contains(QStringLiteral("sequoia_liquid_glass_v6"))==(material==QStringLiteral("glass")),"Wrong glass provider state after transition");
                require(loaded.contains(QStringLiteral("blur"))==(material==QStringLiteral("transparent")),"Wrong standard blur state after transition");
                require(window->property("appearanceBlurRequested").toBool()==(material!=QStringLiteral("solid")),"Preview backdrop request did not follow material");
                capture(QStringLiteral("material-")+material);
            }
            fprintf(stderr,"PASS: live Glass/Transparent/Solid transitions, including return to Glass twice\n");
            QMetaObject::invokeMethod(controller,"stageAccent",Q_ARG(QString,QStringLiteral("#f47750")));
            QMetaObject::invokeMethod(controller,"applyLook",Q_ARG(QString,QStringLiteral("glass")),Q_ARG(QString,QStringLiteral("light")));
            for(int i=0;i<350 && (controller->property("busy").toBool() || controller->property("accentBusy").toBool());++i) QTest::qWait(100);
            require(!controller->property("busy").toBool() && controller->property("colorMode").toString()==QStringLiteral("light"),"Apply did not keep the chosen light palette");
            require(loadedProviders().contains(QStringLiteral("sequoia_liquid_glass_v6")),"Applying color mode lost Glass");
            require(!controller->property("accentPending").toBool() && controller->property("accentColor").value<QColor>()==QColor("#f47750"),"Combined mode/material Apply lost its pending accent");
            QTest::qWait(1000);
            require(livePlasmaTheme.themeName()==QStringLiteral("sequoia-eleven-glass-light"),"Plasma retained its old glass theme after Light was applied");
            QObject *hub = nullptr;
            for (auto quick : window->findChildren<QQuickWidget *>()) if (quick->rootObject())
                for (auto item : items(quick->rootObject())) if(item->objectName()==QStringLiteral("appearanceHub")) hub=item;
            require(hub,"Appearance hub missing during live-update verification");
            hub->setProperty("selectedMode",QStringLiteral("light"));
            hub->setProperty("modeEdited",true);
            require(QProcess::execute(QStringLiteral("plasma-apply-colorscheme"),{QStringLiteral("SequoiaElevenDarkTransparent")})==0,"External color scheme change failed");
            QTest::qWait(1400);
            require(controller->property("colorMode").toString()==QStringLiteral("dark") && hub->property("selectedMode").toString()==QStringLiteral("dark") && !hub->property("modeEdited").toBool(),"External theme change left stale appearance selections");
            require(qApp->palette().window().color().lightness()<128,"Settings did not repaint to the external dark palette");
            fprintf(stderr,"PASS: live Plasma glass theme reload and external palette/selection synchronization\n");
            QMetaObject::invokeMethod(controller,"saveLook",Q_ARG(QString,QStringLiteral("Verification look")),Q_ARG(QString,QStringLiteral("glass")),Q_ARG(QString,QStringLiteral("light")));
            require(!controller->property("savedLooks").toList().isEmpty(),"Save look did not persist a selectable preset");
            fprintf(stderr,"PASS: staged Apply/Reset, kept Light + Glass, and saved appearance preset\n");
        }
        // Every module is searchable by stable ID, including category owners.
        for (const auto &id : all) {
            require(call(nav, "search", id), "Search failed");
            bool found = false;
            for (const auto &entry : nav->property("entries").toList()) found |= entry.toMap().value(QStringLiteral("id")).toString() == id;
            require(found, "Search lost an installed module");
        }
        require(call(nav, "search", QStringLiteral("[.* impossible-query")), "Literal search failed");
        require(nav->property("entries").toList().isEmpty(), "Search unexpectedly treats text as a regular expression");
        capture(QStringLiteral("empty"));
        require(call(nav, "search", QStringLiteral("color")), "Keyword search failed");
        capture(QStringLiteral("search"));
        open(nav, QStringLiteral("kcm_colors"));
        require(nav->property("moduleOpen").toBool(), "Native Colors module did not open");
        require(!window->findChildren<QQuickWidget *>().isEmpty(), "Missing native module host");
        capture(QStringLiteral("colors"));
        QObject *kcm = nullptr;
        for (auto candidate : window->findChildren<QObject *>()) {
            if (candidate->inherits("KCModule")) { kcm = candidate; break; }
        }
        require(kcm, "KDE KCModule was not loaded");
        require(kcm->setProperty("needsSave", true), "Cannot exercise unsaved state");
        auto cancel = [] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            require(dialog, "KDE unsaved-change dialog did not appear");
            for (auto button : dialog->findChildren<QAbstractButton *>()) {
                if (button->text().remove(QLatin1Char('&')) == QStringLiteral("Cancel")) { button->click(); return; }
            }
            require(false, "Cancel action missing");
        };
        QTimer::singleShot(100, qApp, cancel);
        require(!back(nav), "Back discarded unsaved changes after Cancel");
        require(nav->property("moduleOpen").toBool(), "Cancel lost the current page");
        QTimer::singleShot(100, qApp, cancel);
        require(!call(nav, "browse", QStringLiteral("devices")), "Category switch ignored Cancel");
        QTimer::singleShot(100, qApp, cancel);
        require(!call(nav, "search", QStringLiteral("keyboard")), "Search ignored Cancel");
        require(nav->property("query").toString() == QStringLiteral("color"), "Canceled search mutated navigation");
        if (qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB")) {
            QTimer::singleShot(100, qApp, cancel);
            auto chrome = window->findChild<QObject *>(QStringLiteral("appearancePreview"));
            QMetaObject::invokeMethod(chrome, "windowAction", Q_ARG(QString, QStringLiteral("close")));
            require(window->isVisible(), "Custom close discarded unsaved changes after Cancel");
        }
        // Reset only the test flag; no settings are saved.
        kcm->setProperty("needsSave", false);
        require(back(nav), "Return to search failed");
        require(nav->property("query").toString() == QStringLiteral("color"), "Back lost search context");
        require(call(nav, "browse", QStringLiteral("devices")), "Devices navigation failed");
        open(nav, QStringLiteral("kcm_keyboard"));
        require(nav->property("moduleOpen").toBool(), "Native QWidget keyboard module did not open");
        capture(QStringLiteral("keyboard"));
        require(back(nav), "Back from keyboard failed");
        const auto captureGroup = qEnvironmentVariable("SEQUOIA_SETTINGS_CAPTURE_GROUP");
        if (!captureGroup.isEmpty()) {
            require(call(nav, "browse", captureGroup), "Capture category missing");
            const auto entries = nav->property("entries").toList();
            require(!entries.isEmpty(), "Capture category is empty");
            for (const auto &entry : entries) {
                const auto id = entry.toMap().value(QStringLiteral("id")).toString();
                fprintf(stderr, "Opening %s\n", qPrintable(id));
                fflush(stderr);
                open(nav, id);
                // KDE's external-app host has no KCModule metadata; the
                // navigation controller reports it as open with an empty ID.
                require(nav->property("moduleOpen").toBool(), "Category page did not open");
                if (id != QStringLiteral("sequoia-test-extra")) {
                    require(nav->property("currentModule").toString() == id, "Wrong category page opened");
                }
                for (auto quick : window->findChildren<QQuickWidget *>()) {
                    if (quick->isVisible()) require(quick->status() != QQuickWidget::Error && quick->rootObject(), "Category QML page failed to load");
                }
                capture(id);
                if (id == QStringLiteral("kcm_kwindecoration")) {
                    bool clicked = false;
                    for (auto quick : window->findChildren<QQuickWidget *>()) {
                        if (!quick->isVisible() || !quick->rootObject()) continue;
                        for (auto item : items(quick->rootObject())) {
                            if (item->isVisible() && item->inherits("QQuickAbstractButton")
                                && item->property("text").toString().contains(QStringLiteral("Configure Titlebar Buttons"))) {
                                QTest::mouseClick(quick->quickWindow(), Qt::LeftButton, Qt::NoModifier,
                                    item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
                                clicked = true;
                                break;
                            }
                        }
                        if (clicked) break;
                    }
                    require(clicked, "Cannot open titlebar button subpage");
                    QTest::qWait(600);
                    capture(QStringLiteral("appearance-titlebar-buttons"));
                }
                if (id == QStringLiteral("kcm_fonts")) {
                    window->setMinimumSize(0, 0);
                    window->resize(800, 600);
                    QTest::qWait(200);
                    capture(QStringLiteral("appearance-fonts-compact"));
                    window->resize(1100, 790);
                    QTest::qWait(200);
                }
                // Some native pages mark defaults dirty during initialization
                // in an empty profile (for example Spell Check). Discard those
                // changes through the real dialog; never apply them.
                QTimer::singleShot(100, qApp, [] {
                    auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                    if (!dialog) return;
                    for (auto button : dialog->findChildren<QAbstractButton *>()) {
                        if (button->text().remove(QLatin1Char('&')) == QStringLiteral("Discard")) {
                            button->click();
                            return;
                        }
                    }
                    require(false, "Unexpected dialog while leaving captured page");
                });
                require(back(nav), "Cannot leave category page");
            }
            fprintf(stderr, "PASS: opened and captured %lld pages in %s\n", static_cast<long long>(entries.size()), qPrintable(captureGroup));
        }
        require(call(nav, "browse", QStringLiteral("home")), "Home navigation failed");
        // Exercise a real card click; QML delegates must be wired to the backend.
        QQuickWidget *overview = nullptr;
        QQuickItem *card = nullptr;
        for (auto quick : window->findChildren<QQuickWidget *>()) {
            if (!quick->isVisible() || !quick->rootObject()) continue;
            for (auto item : items(quick->rootObject())) {
                if (item->property("modelData").toMap().value(QStringLiteral("id")) == QStringLiteral("appearance")
                    && item->inherits("QQuickAbstractButton") && (qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB") ? item->objectName() == QStringLiteral("overviewCategory-appearance") : item->width() > 250)) { overview = quick; card = item; }
            }
        }
        require(overview && card, "Appearance card missing");
        QTest::mouseClick(overview->quickWindow(), Qt::LeftButton, Qt::NoModifier,
                         card->mapToScene(QPointF(card->width()/2, card->height()/2)).toPoint());
        QTest::qWait(200);
        require(nav->property("selectedGroup").toString() == QStringLiteral("appearance"), "Card click did not navigate");
        require(back(nav), "Return home failed");
        // The command-line and D-Bus entry path remains the upstream path.
        QObject *sidebar = nav->parent();
        require(QMetaObject::invokeMethod(sidebar, "loadModule", Qt::DirectConnection, Q_ARG(QModelIndex, QModelIndex()), Q_ARG(QStringList, QStringList())), "Native loadModule entry point missing");
        require(nav->property("home").toBool(), "Invalid module changed the overview");
        window->setMinimumSize(0, 0);
        window->resize(800, 600);
        QTest::qWait(200);
        capture(QStringLiteral("compact"));
        fprintf(stderr, "PASS: %lld unique modules; complete category and search coverage; native QML and QWidget pages; Cancel preserves pending changes; real card navigation\n", static_cast<long long>(catalog.size()));
        QCoreApplication::exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
