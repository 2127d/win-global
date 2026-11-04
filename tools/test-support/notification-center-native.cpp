// SPDX-License-Identifier: MIT
// Test-only hook for an isolated plasmawindowed process, never installed.
#include <QApplication>
#include <QTest>
namespace NotificationManager { class Server { public: static Server *self(); bool init(); }; }
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlExpression>
#include <QEventLoop>
#include <QQmlContext>
#include <functional>
#include <memory>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QColor>
#include <QPalette>
#include <QTextStream>
#include <cstdlib>

static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (qsizetype i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}
static void testStartup() {
    if (qEnvironmentVariable("SEQUOIA_NOTIFICATION_TEST_PRIVATE") != "1") return;
    const auto output = qEnvironmentVariable("SEQUOIA_CONTROL_CAPTURE");
    if (output.isEmpty()) return;
    if (QFileInfo(QCoreApplication::applicationFilePath()).fileName() != QStringLiteral("plasmawindowed")) return;
    qApp->setQuitOnLastWindowClosed(false);
    QTimer::singleShot(0,qApp,[]{ if (!NotificationManager::Server::self()->init()) std::_Exit(4); });
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
    if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_DETAILS")) {
        // Enable the battery/brightness applets on the private test bus without
        // starting PowerDevil or granting a route to real power actions.
        QDBusConnection::sessionBus().registerService("org.kde.Solid.PowerManagement");
    }
    QTimer::singleShot(1500, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (!quick) continue;
            for (auto item : items(quick->contentItem())) {
                auto state = item->property("systemTrayState").value<QObject *>();
                if (state) {
                    QMetaObject::invokeMethod(state, "setActiveApplet", Q_ARG(QVariant, QVariant::fromValue(nullptr)), Q_ARG(QVariant, QVariant()));
                    state->setProperty("expanded", true);
                    return;
                }
            }
        }
        qWarning("No system tray found");
    });
    QTimer::singleShot(4000, qApp, [output] {
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (!quick) continue;
            for (auto item : items(quick->contentItem())) {
                if (item->objectName() != QStringLiteral("sequoiaControlCenter")) continue;
                if (!quick->grabWindow().save(output)) { qApp->exit(2); return; }
                fprintf(stderr, "PASS: real native tray loaded the dashboard and opened its popup\n");
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_DETAILS")) {
                    auto names = std::make_shared<QStringList>(qEnvironmentVariable("SEQUOIA_VERIFY_DETAILS").split(','));
                    auto next = std::make_shared<std::function<void()>>();
                    *next = [item, names, next] {
                        if (names->isEmpty()) {
                            fprintf(stderr, "PASS: all detail menus and Back navigation\n");
                            std::_Exit(0);
                        }
                        const QString name = names->takeFirst();
                        QMetaObject::invokeMethod(item, "openApplet", Q_ARG(QVariant, name == "contacts" ? QStringLiteral("org.kde.merkuro.contact.applet") : "org.kde.plasma." + name));
                        QTimer::singleShot(650, qApp, [item, name, next] {
                            QQmlExpression active(qmlContext(item), item, "systemTrayState.activeApplet?.Plasmoid.pluginName ?? ''");
                            const auto activeName = active.evaluate().toString();
                            bool found = activeName == (name == "contacts" ? QStringLiteral("org.kde.merkuro.contact.applet") : "org.kde.plasma." + name);
                            QQuickItem *page = nullptr;
                            for (auto child : items(item)) {
                                if (child->isVisible() && child->objectName() == (name == "contacts" ? "sequoiaContactsPage" : name == "weather" ? "sequoiaWeatherPage" : "sequoiaDetailPage")) page = child;
                            }
                            found = found && page;
                            if (page) found = found && page->width() > 200 && page->height() > 200;
                            if (page) {
                                for (auto child : items(page)) {
                                    const auto color = child->property("color").value<QColor>();
                                    if (child->isVisible() && QByteArray(child->metaObject()->className()).startsWith("DetailSurface") && color.isValid()) {
                                        const bool correct = (color.lightnessF() < 0.5) == qEnvironmentVariableIsSet("SEQUOIA_VERIFY_DARK");
                                        if (!correct) fprintf(stderr, "FAIL: %s surface palette %s\n", qPrintable(name), qPrintable(color.name()));
                                        found = found && correct;
                                    }
                                }
                            }
                            const bool captured = item->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_DETAIL_CAPTURE") + '-' + name + ".png");
                            if (page && name == "notifications") {
                                const auto settle=[] {QTest::qWait(650);};
                                auto eval=[&](const QString &expression) {
                                    QObject *scope=page;for(auto c:items(page))if(c->objectName()=="notificationHistoryList")scope=c;
                                    QQmlExpression test(qmlContext(scope),page,expression);auto value=test.evaluate();
                                    if(test.hasError()){fprintf(stderr,"FAIL: %s\n",qPrintable(test.error().toString()));found=false;}
                                    return value;
                                };
                                auto check=[&](const QString &expression){const bool ok=eval(expression).toBool();found=found&&ok;fprintf(stderr,"%s: %s\n",ok?"PASS":"FAIL",qPrintable(expression));};
                                auto named=[&](const QString &name)->QQuickItem*{for(auto child:items(page)) if(child->objectName()==name)return child;return nullptr;};
                                auto capture=[&](const QString &name){settle();item->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_DETAIL_CAPTURE")+'-'+name+".png");};
                                auto click=[&](QQuickItem *target){if(!target){found=false;fprintf(stderr,"FAIL: missing control\n");return;}QTest::mouseClick(target->window(),Qt::LeftButton,Qt::NoModifier,target->mapToScene(QPointF(target->width()/2,target->height()/2)).toPoint());settle();};
                                auto send=[&](QString app,QString title,QString body,QStringList actions=QStringList{},QString desktop=QString("org.kde.merkuro.calendar")){
                                    auto msg=QDBusMessage::createMethodCall("org.freedesktop.Notifications","/org/freedesktop/Notifications","org.freedesktop.Notifications","Notify");
                                    msg.setArguments({app,uint(0),QString("calendar"),title,body,actions,QVariantMap{{"urgency",uchar(1)},{"suppress-sound",true},{"desktop-entry",desktop}},0});
                                    auto reply=QDBusConnection::sessionBus().call(msg,QDBus::BlockWithGui);if(reply.type()==QDBusMessage::ErrorMessage){found=false;fprintf(stderr,"FAIL: notification delivery %s\n",qPrintable(reply.errorMessage()));}settle();
                                };
                                check("NotificationManager.Server.valid");
                                eval("notificationSettings.inhibitNotificationsWhenFullscreen = false; NotificationState.Globals.revokeInhibitions(); notificationSettings.save()");settle();
                                capture("empty");
                                click(named("notificationDndToggle"));capture("duration-menu");
                                check("quietCard.children.length > 0");
                                // Menu items live in the popup overlay, outside the full representation.
                                QQuickItem *duration=nullptr;
                                for(auto child:items(item->window()->contentItem())) if(child->objectName()=="notificationDuration:0" && child->isVisible()) duration=child;
                                click(duration);
                                check("NotificationState.Globals.inhibited && notificationSettings.notificationsInhibitedUntil.getTime() > Date.now()+3500000 && notificationSettings.notificationsInhibitedUntil.getTime() < Date.now()+3700000");
                                capture("dnd");
                                send("Calendar","Design review in 10 minutes","Bring the latest desktop updates.",{"open","Open Calendar","later","Remind me"});
                                send("Calendar","Afternoon check-in","Review the revised navigation.");
                                send("Calendar","Tomorrow's planning","Write down the next improvements.");
                                send("Notes","A message from Alex","The spacing looks good. Can you send the updated mockup?",{"inline-reply","Reply","later","Later"},"org.kde.kwrite");
                                check("historyModel.count >= 4");
                                capture("history");
                                auto quiet=named("notificationQuietCard");auto list=named("notificationHistoryList");
                                if(quiet&&list){
                                    auto rail=quiet->mapRectToScene(quiet->boundingRect());
                                    QObject *heading=nullptr;for(auto child:item->childItems())if(child->isVisible() && QByteArray(child->metaObject()->className()).startsWith("PlasmoidHeading"))heading=child;
                                    auto headingBackground=heading?qobject_cast<QQuickItem*>(heading->property("background").value<QObject*>()):nullptr;
                                    if(headingBackground){auto headerRail=headingBackground->mapRectToScene(headingBackground->boundingRect());bool match=qAbs(headerRail.left()-rail.left())<1&&qAbs(headerRail.width()-rail.width())<1;found=found&&match;fprintf(stderr,"%s: content matches the tray header width\n",match?"PASS":"FAIL");}else found=false;
                                    auto listRail=list->mapRectToScene(list->boundingRect());
                                    const bool same=qAbs(rail.left()-listRail.left())<1&&qAbs(rail.width()-listRail.width())<1;found=found&&same;fprintf(stderr,"%s: Do Not Disturb and history share horizontal edges\n",same?"PASS":"FAIL");
                                    for(auto c:items(page))if(c->objectName().startsWith("notificationHistoryCard:")){bool match=qAbs(c->width()-rail.width())<1;found=found&&match;}
                                }else found=false;
                                check("list.setGroupExpanded(1, true); true");settle();
                                check("historyModel.data(historyModel.index(1,0), NotificationManager.Notifications.IsGroupExpandedRole)");capture("expanded");
                                eval("list.positionViewAtEnd()");settle();capture("history-actions");
                                QQuickItem *actionButton=nullptr;for(auto c:items(page))if(c->isVisible()&&c->property("text").toString()=="Open Calendar")actionButton=c;
                                const int beforeAction=eval("historyModel.count").toInt();click(actionButton);
                                const bool actionSent=eval("historyModel.count").toInt()<beforeAction;found=found&&actionSent;fprintf(stderr,"%s: history action dispatch removes non-resident notification\n",actionSent?"PASS":"FAIL");
                                eval("list.positionViewAtBeginning()");settle();
                                check("list.setGroupExpanded(1, false); true");settle();check("!historyModel.data(historyModel.index(1,0), NotificationManager.Notifications.IsGroupExpandedRole)");
                                // Exercise native reply controls and model dispatch with synthetic private notifications.
                                QQuickItem *replyButton=nullptr;
                                for(auto c:items(page))if(c->isVisible()&&c->property("text").toString()=="Reply")replyButton=c;
                                click(replyButton);capture("reply");
                                QQuickItem *replyField=nullptr,*sendButton=nullptr;
                                for(auto c:items(page)){if(c->isVisible()&&c->inherits("QQuickTextField"))replyField=c;if(c->isVisible()&&c->property("text").toString()=="Send")sendButton=c;}
                                if(replyField)replyField->setProperty("text","I will send it shortly.");else found=false;
                                const int beforeReply=eval("historyModel.count").toInt();click(sendButton);
                                check("historyModel.count > 0");
                                const bool replied=eval("historyModel.count").toInt()<beforeReply;found=found&&replied;fprintf(stderr,"%s: inline reply dispatch removes non-resident notification\n",replied?"PASS":"FAIL");
                                click(named("notificationDndToggle"));check("!NotificationState.Globals.inhibited");
                                // Expire private items so Clear history can use KDE's exact ClearExpired policy.
                                check("for (let i = historyModel.count - 1; i >= 0; --i) { const ix=historyModel.index(i,0); if (!historyModel.data(ix, NotificationManager.Notifications.IsGroupRole)) historyModel.expire(ix); } true");settle();
                                auto clear=named("notificationClearHistory");if(!clear||!clear->isEnabled())found=false;else click(clear);
                                check("historyModel.expiredNotificationsCount === 0");
                            }
                            QMetaObject::invokeMethod(item, "goHome");
                            found = found && captured && item->property("compactDashboard").toBool();
                            fprintf(stderr, "%s: %s native detail surface, capture and Back\n", found ? "PASS" : "FAIL", qPrintable(name));
                            if (!found) {
                                fprintf(stderr, "FAIL: active=%s page=%p captured=%d home=%d\n", qPrintable(activeName), static_cast<void *>(page), captured, item->property("compactDashboard").toBool());
                                std::_Exit(10);
                            }
                            QTimer::singleShot(150, qApp, *next);
                        });
                    };
                    (*next)();
                    return;
                }
            }
        }
        fprintf(stderr, "FAIL: native Control Center was not found\n");
        std::_Exit(3);
    });
}
Q_COREAPP_STARTUP_FUNCTION(testStartup)
