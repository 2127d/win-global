// SPDX-License-Identifier: MIT
// Loaded only by verify-tray-sizing.py in a private Plasma process.
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QDBusConnection>
#include <QFileInfo>
#include <QTest>
#include <QSet>
#include <QFile>
#include <QTextStream>
#include <functional>
#include <memory>
#include <cstdlib>

static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (qsizetype i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}
// This runs only against the disposable virtual compositor, never the desktop.
static void checkClosing(QQuickItem *popup) {
    QQmlExpression unpin(qmlContext(popup), popup, "Plasmoid.configuration.pin = false");
    unpin.evaluate();
    auto other = new QQuickWindow;
    other->setTitle("Private focus target"); other->resize(200, 100);
    other->show(); other->requestActivate();
    QTimer::singleShot(650, qApp, [popup, other] {
        QQmlExpression closed(qmlContext(popup), popup, "!dialog.visible && !systemTrayState.expanded");
        bool ok = closed.evaluate().toBool() && !closed.hasError();
        fprintf(stderr, "%s: unpinned popup closes when another window activates\n", ok ? "PASS" : "FAIL");
        if (!ok) std::_Exit(5);
        other->close(); other->deleteLater();
        QQmlExpression reopen(qmlContext(popup), popup, "systemTrayState.expanded = true");
        reopen.evaluate();
        QTimer::singleShot(300, qApp, [popup] {
            popup->forceActiveFocus();
            QTest::keyClick(popup->window(), Qt::Key_Escape);
            QTimer::singleShot(250, qApp, [popup] {
                QQmlExpression closed(qmlContext(popup), popup, "!dialog.visible && !systemTrayState.expanded");
                const bool ok = closed.evaluate().toBool() && !closed.hasError();
                fprintf(stderr, "%s: Escape closes the reopened popup\n", ok ? "PASS" : "FAIL");
                std::_Exit(ok ? 0 : 6);
            });
        });
    });
}
static void checkDeviceAnimation(QQuickItem *popup, const std::function<void()> &done) {
    QObject *controller = nullptr;
    for (auto child : items(popup)) if (child->objectName() == "disksDevicesView") controller = child->property("controller").value<QObject *>();
    if (!controller) { fprintf(stderr, "FAIL: device page not found\n"); std::_Exit(7); }
    QMetaObject::invokeMethod(controller, "setFilter", Q_ARG(QVariant, QString("removable")));
    QTimer::singleShot(500, qApp, [popup, controller, done] {
        const auto sample = [popup, controller](const QString &filter, const std::function<void()> &next) {
            const int initial = qRound(popup->height());
            QMetaObject::invokeMethod(controller, "setFilter", Q_ARG(QVariant, filter));
            auto timer = new QTimer(qApp); timer->setInterval(16);
            auto values = std::make_shared<QList<int>>();
            auto windowValues = std::make_shared<QList<int>>();
            QObject::connect(timer, &QTimer::timeout, qApp, [popup, values, windowValues] {
                values->append(qRound(popup->height())); windowValues->append(popup->window()->height());
            });
            timer->start();
            QTimer::singleShot(550, qApp, [popup, timer, values, windowValues, initial, filter, next] {
                timer->stop(); timer->deleteLater();
                const int final = qRound(popup->property("targetHeight").toDouble());
                const QSet<int> distinct(values->begin(), values->end());
                const QSet<int> windowDistinct(windowValues->begin(), windowValues->end());
                bool ok = qAbs(popup->height()-final)<2;
                if (qAbs(initial-final)>30) ok = ok && distinct.size()>3 && windowDistinct.size()>3;
                QFile csv(qEnvironmentVariable("SEQUOIA_DETAIL_CAPTURE")+"-filter-"+filter+".csv");
                if (csv.open(QIODevice::WriteOnly)) {
                    QTextStream stream(&csv); stream << "sample,content_height,window_height\n";
                    for (qsizetype i=0;i<values->size();++i) stream << i << ',' << values->at(i) << ',' << windowValues->at(i) << '\n';
                }
                fprintf(stderr, "%s: %s filter resize %d -> %d, %lld content / %lld native window sizes\n", ok ? "PASS" : "FAIL", qPrintable(filter), initial, final, static_cast<long long>(distinct.size()), static_cast<long long>(windowDistinct.size()));
                if (!ok) std::_Exit(8);
                next();
            });
        };
        sample("internal", [sample, done] { sample("removable", done); });
    });
}
static void checkAudioNavigation(QQuickItem *popup, const std::function<void()> &done) {
    QQuickItem *navigation = nullptr;
    QObject *applet = nullptr;
    for (auto item : items(popup)) {
        if (item->objectName() == "audioNavigation") navigation = item;
        if (item->objectName() == "sequoiaDetailPage" && item->property("applet").value<QObject *>()) applet = item->property("applet").value<QObject *>();
    }
    if (!navigation || !applet) std::_Exit(11);
    // Navigation only: never press Record or access the microphone.
    applet->setProperty("micTestActive", true);
    QTimer::singleShot(300, qApp, [navigation, applet, done] {
        if (navigation->property("currentIndex").toInt() != 1 || navigation->property("busy").toBool()) std::_Exit(11);
        applet->setProperty("micTestActive", false);
        QTimer::singleShot(300, qApp, [navigation, done] {
            const bool ok = navigation->property("currentIndex").toInt() == 0 && !navigation->property("busy").toBool();
            fprintf(stderr, "%s: microphone page opens and returns without recording\n", ok ? "PASS" : "FAIL");
            if (!ok) std::_Exit(11);
            done();
        });
    });
}
static void testStartup() {
    if (QFileInfo(QCoreApplication::applicationFilePath()).fileName() != "plasmawindowed") return;
    qApp->setQuitOnLastWindowClosed(false);
    QDBusConnection::sessionBus().registerService("org.kde.Solid.PowerManagement");
    QTimer::singleShot(1800, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (!quick) continue;
            for (auto item : items(quick->contentItem())) {
                auto state = item->property("systemTrayState").value<QObject *>();
                if (state) { state->setProperty("expanded", true); return; }
            }
        }
    });
    QTimer::singleShot(4500, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (!quick) continue;
            for (auto popup : items(quick->contentItem())) {
                if (popup->objectName() != "sequoiaControlCenter") continue;
                auto names = std::make_shared<QStringList>(QStringList{"weather", "vault", "printmanager", "dwellclicker", "devicenotifier", "kdeconnect", "kscreen", "brightness", "battery", "notifications", "clipboard", "volume", "bluetooth", "networkmanagement", "weather"});
                auto weatherSize = std::make_shared<QSizeF>();
                auto next = std::make_shared<std::function<void()>>();
                *next = [popup, names, weatherSize, next] {
                    if (names->isEmpty()) {
                        QMetaObject::invokeMethod(popup, "goHome");
                        QTimer::singleShot(60, qApp, [popup] {
                            QQuickItem *navigation = nullptr;
                            for (auto item : items(popup)) if (item->objectName() == "controlNavigation") navigation = item;
                            if (!navigation) std::_Exit(9);
                            QQmlExpression slide(qmlContext(navigation), navigation,
                                "busy && pages[0].x < 0 && pages[2].x > 0");
                            const bool ok = slide.evaluate().toBool() && !slide.hasError();
                            fprintf(stderr, "%s: Back slides the dashboard in from the left\n", ok ? "PASS" : "FAIL");
                            if (!ok) std::_Exit(9);
                        });
                        QTimer::singleShot(300, qApp, [popup] {
                            bool ok = popup->property("compactDashboard").toBool()
                                && qAbs(popup->width()-popup->property("dashboardWidth").toDouble()) < 2
                                && qAbs(popup->height()-popup->property("dashboardHeight").toDouble()) < 2;
                            fprintf(stderr, "%s: Back restores configured Control Center size\n", ok ? "PASS" : "FAIL");
                            if (!ok) std::_Exit(3);
                            // Sample a warm page: loading a native plugin may block
                            // the event loop beyond the animation on its first open.
                            QMetaObject::invokeMethod(popup, "openApplet", Q_ARG(QVariant, QString("org.kde.plasma.weather")));
                            QTimer::singleShot(60, qApp, [popup] {
                                QQuickItem *navigation = nullptr;
                                for (auto item : items(popup)) if (item->objectName() == "controlNavigation") navigation = item;
                                if (!navigation) std::_Exit(10);
                                QQmlExpression slide(qmlContext(navigation), navigation,
                                    "busy && pages[2].x > 0 && pages[0].x < 0");
                                const bool ok = slide.evaluate().toBool() && !slide.hasError();
                                fprintf(stderr, "%s: opening a widget slides content to the left\n", ok ? "PASS" : "FAIL");
                                if (!ok) std::_Exit(10);
                                QTimer::singleShot(250, qApp, [popup] {
                                    QMetaObject::invokeMethod(popup, "goHome");
                                    QTimer::singleShot(250, qApp, [popup] { checkClosing(popup); });
                                });
                            });
                        });
                        return;
                    }
                    const auto name = names->takeFirst();
                    QMetaObject::invokeMethod(popup, "openApplet", Q_ARG(QVariant, name == "kscreen" || name == "kdeconnect" ? "org.kde." + name : "org.kde.plasma." + name));
                    if (name == "devicenotifier") {
                        QQmlExpression showAll(qmlContext(popup), popup, "systemTrayState.activeApplet.Plasmoid.configuration.allDevices = true");
                        showAll.evaluate();
                    }
                    QTimer::singleShot(850, qApp, [popup, name, weatherSize, next] {
                        QQmlExpression check(qmlContext(popup), popup,
                            "!!plasmoidContainer.currentItem && Math.abs(width-targetWidth)<2 && Math.abs(height-targetHeight)<2 && height<=720 && width<=680");
                        bool ok = check.evaluate().toBool() && !check.hasError();
                        if (name == "weather") {
                            const QSizeF current(popup->width(), popup->height());
                            if (!weatherSize->isEmpty()) ok = ok && *weatherSize == current;
                            *weatherSize = current;
                        }
                        const auto path = qEnvironmentVariable("SEQUOIA_DETAIL_CAPTURE") + '-' + name + ".png";
                        ok = popup->window()->grabWindow().save(path) && ok;
                        fprintf(stderr, "%s: %s %.0fx%.0f target %.0fx%.0f\n", ok ? "PASS" : "FAIL", qPrintable(name), popup->width(), popup->height(), popup->property("targetWidth").toDouble(), popup->property("targetHeight").toDouble());
                        if (!ok) std::_Exit(2);
                        if (name == "devicenotifier") checkDeviceAnimation(popup, [next] { (*next)(); });
                        else if (name == "volume") checkAudioNavigation(popup, [next] { (*next)(); });
                        else (*next)();
                    });
                };
                (*next)();
                return;
            }
        }
        fprintf(stderr, "FAIL: native tray not found\n"); std::_Exit(4);
    });
}
Q_COREAPP_STARTUP_FUNCTION(testStartup)
