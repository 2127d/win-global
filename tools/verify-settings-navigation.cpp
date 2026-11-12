// SPDX-License-Identifier: MIT
#include <QQuickPaintedItem>
#include <QSignalSpy>
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
                if(i==0)capture(QStringLiteral("sidebar-show-menu"));
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
            auto relatedMenu=related->property("menu").value<QObject*>();
            require(relatedMenu,"Related settings menu missing");
            auto clickRelated=[&] {
                QTest::mouseClick(related->window(),Qt::LeftButton,Qt::NoModifier,
                                  related->mapToScene(QPointF(related->width()/2,related->height()/2)).toPoint());
                QTest::qWait(180);
            };
            QSignalSpy relatedOpened(relatedMenu,SIGNAL(opened()));
            for(int h : {980,650}) {
                window->resize(1100,h);QTest::qWait(250);
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
                    if(i==0) capture(h==980 ? QStringLiteral("related-menu-below") : QStringLiteral("related-menu-above"));
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
            QMetaObject::invokeMethod(controller,"applyLook",Q_ARG(QString,QStringLiteral("glass")),Q_ARG(QString,QStringLiteral("light")));
            for(int i=0;i<250 && controller->property("busy").toBool();++i) QTest::qWait(100);
            require(!controller->property("busy").toBool() && controller->property("colorMode").toString()==QStringLiteral("light"),"Apply did not keep the chosen light palette");
            require(loadedProviders().contains(QStringLiteral("sequoia_liquid_glass_v6")),"Applying color mode lost Glass");
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
