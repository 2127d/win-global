// SPDX-License-Identifier: MIT
// Test-only hook for an isolated plasmawindowed process, never installed.
#include <QApplication>
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
    const auto output = qEnvironmentVariable("SEQUOIA_CONTROL_CAPTURE");
    if (output.isEmpty()) return;
    if (QFileInfo(QCoreApplication::applicationFilePath()).fileName() != QStringLiteral("plasmawindowed")) return;
    qApp->setQuitOnLastWindowClosed(false);
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
    if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_CONTACTS_STARTUP")) {
        QTimer::singleShot(12000, qApp, [] {
            bool found = false;
            for (auto window : QGuiApplication::allWindows()) {
                if (auto quick = qobject_cast<QQuickWindow *>(window)) {
                    for (auto item : items(quick->contentItem())) {
                        if (item->objectName() == QStringLiteral("sequoiaContactsApplet")) {
                            found = true;
                            if (item->property("preloadFullRepresentation").toBool()) std::_Exit(3);
                        }
                    }
                }
            }
            const auto names = QDBusConnection::sessionBus().interface()->registeredServiceNames();
            if (!names.isValid()) std::_Exit(3);
            for (const auto &name : names.value()) {
                if (name.contains(QStringLiteral("akonadi"), Qt::CaseInsensitive)) {
                    fprintf(stderr, "FAIL: Contacts started Akonadi during preload\n");
                    std::_Exit(3);
                }
            }
            fprintf(stderr, found ? "PASS: Contacts startup remains responsive without Akonadi\n" : "FAIL: Contacts applet missing\n");
            std::_Exit(found ? 0 : 3);
        });
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
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_CONTACTS_STARTUP")) return;
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
                            if (page && name == "contacts") {
                                const auto settle = [] {
                                    QEventLoop loop;
                                    QTimer::singleShot(350, &loop, &QEventLoop::quit);
                                    loop.exec();
                                };
                                QQmlExpression profile(qmlContext(page), page, "stack.push(Qt.resolvedUrl('ContactDetails.qml'), {contact: {formattedName: 'Preview Contact', photoUrl: '', nickName: '', preferredEmail: '', phoneModel: [], emailModel: [], addressesModel: [], birthday: new Date(NaN)}}); stack.depth === 2");
                                found = profile.evaluate().toBool() && !profile.hasError() && found;
                                if (profile.hasError()) fprintf(stderr, "FAIL: contacts profile: %s\n", qPrintable(profile.error().toString()));
                                settle();
                                QQmlExpression code(qmlContext(page), page, "stack.push(Qt.resolvedUrl('QrCodePage.qml'), {qrCodeData: 'BEGIN:VCARD\\nVERSION:3.0\\nFN:Preview Contact\\nEND:VCARD'}); stack.depth === 3");
                                found = code.evaluate().toBool() && !code.hasError() && found;
                                if (code.hasError()) fprintf(stderr, "FAIL: contacts QR: %s\n", qPrintable(code.error().toString()));
                                settle();
                                QQmlExpression back(qmlContext(item), item, "systemTrayState.activeApplet.backAction.trigger()");
                                back.evaluate();
                                settle();
                                found = found && page->property("navigationDepth").toInt() == 2;
                                back.evaluate();
                                settle();
                                found = found && page->property("navigationDepth").toInt() == 1;
                            }
                            if (page && name == "clipboard") {
                                QQmlExpression barcode(qmlContext(page), page, "clipboardMenu.barcode('Control Center preview'); stack.depth === 2");
                                const bool opened = barcode.evaluate().toBool() && !barcode.hasError();
                                if (!opened) fprintf(stderr, "FAIL: barcode open: %s\n", qPrintable(barcode.error().toString()));
                                found = found && opened;
                                QQmlExpression back(qmlContext(item), item, "systemTrayState.activeApplet.backAction.trigger()");
                                back.evaluate();
                                QQmlExpression restored(qmlContext(page), page, "stack.depth === 1");
                                const bool returned = restored.evaluate().toBool() && !restored.hasError();
                                if (!returned) fprintf(stderr, "FAIL: barcode Back: %s\n", qPrintable(restored.error().toString()));
                                found = found && returned;
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
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_MEDIA")) {
                    QMetaObject::invokeMethod(item, "openApplet", Q_ARG(QVariant, QStringLiteral("org.kde.plasma.mediacontroller")));
                    QTimer::singleShot(500, qApp, [item, output] {
                        for (auto window : QGuiApplication::allWindows()) {
                            auto quick = qobject_cast<QQuickWindow *>(window);
                            if (!quick) continue;
                            for (auto child : items(quick->contentItem())) {
                                if (child->objectName() != "sequoiaMediaPlayer" || !child->isVisible()) continue;
                                bool tabs = false, art = false;
                                for (auto control : items(child)) {
                                    if (control->objectName() == "mediaPlayerTabs") tabs = control->property("count").toInt() == 3;
                                    if (control->objectName() == "mediaArtworkImage") art = control->property("status").toInt() == 1;
                                }
                                const bool capture = quick->grabWindow().save(output);
                                QMetaObject::invokeMethod(item, "goHome");
                                const bool home = item->property("compactDashboard").toBool();
                                fprintf(stderr, "%s: native media tabs, artwork and Back navigation\n", tabs && art && capture && home ? "PASS" : "FAIL");
                                std::_Exit(tabs && art && capture && home ? 0 : 8);
                            }
                        }
                        fprintf(stderr, "FAIL: redesigned native media popup was not found\n");
                        std::_Exit(9);
                    });
                    return;
                }
                auto policy = item->findChild<QObject *>("controlCenterItems");
                if (!policy) { fprintf(stderr, "FAIL: no tray visibility controller\n"); std::_Exit(4); }
                auto config = policy->property("configuration").value<QObject *>();
                const auto originalExtra = config->property("extraItems");
                const QString network = "org.kde.plasma.networkmanagement";
                const QString clipboard = "org.kde.plasma.clipboard";
                auto check = [policy](const QString &expression) {
                    QQmlExpression test(qmlContext(policy), policy, expression);
                    const bool passed = test.evaluate().toBool();
                    fprintf(stderr, "%s: %s\n", passed ? "PASS" : "FAIL", qPrintable(expression));
                    if (!passed || test.hasError()) std::_Exit(5);
                };
                check("customItems.includes('org.kde.plasma.networkmanagement') && otherItems.some(e => e.id === 'org.kde.plasma.clipboard')");
                auto steps = std::make_shared<QList<std::function<void()>>>();
                steps->append([=] { config->setProperty("shownItems", QStringList{network, clipboard}); });
                steps->append([=] {
                    check("!entries.some(e => e.id === 'org.kde.plasma.networkmanagement' || e.id === 'org.kde.plasma.clipboard')");
                    config->setProperty("shownItems", QStringList{});
                    config->setProperty("hiddenItems", QStringList{network, clipboard});
                });
                steps->append([=] {
                    check("customItems.includes('org.kde.plasma.networkmanagement') && otherItems.some(e => e.id === 'org.kde.plasma.clipboard')");
                    config->setProperty("hiddenItems", QStringList{});
                    auto extra = originalExtra.toStringList(); extra.removeAll(network); extra.removeAll(clipboard);
                    config->setProperty("extraItems", extra);
                });
                steps->append([=] {
                    check("!entries.some(e => e.id === 'org.kde.plasma.networkmanagement' || e.id === 'org.kde.plasma.clipboard')");
                    config->setProperty("extraItems", originalExtra);
                });
                steps->append([=] {
                    check("customItems.includes('org.kde.plasma.networkmanagement') && otherItems.some(e => e.id === 'org.kde.plasma.clipboard')");
                    config->setProperty("showAllItems", true);
                });
                steps->append([=] {
                    check("entries.length === 0");
                    config->setProperty("showAllItems", false);
                });
                steps->append([=] {
                    check("otherItems.every(e => !customItems.includes(e.id))");
                    QQuickItem *card = nullptr;
                    for (auto candidate : items(item)) {
                        if (candidate->objectName() == "trayCard:" + clipboard) card = candidate;
                    }
                    if (!card) { fprintf(stderr, "FAIL: generic clipboard card missing\n"); std::_Exit(6); }
                    QMetaObject::invokeMethod(card, "activate");
                });
                auto next = std::make_shared<std::function<void()>>();
                *next = [=] {
                    if (!steps->isEmpty()) {
                        steps->takeFirst()();
                        QTimer::singleShot(250, qApp, *next);
                        return;
                    }
                    QQmlExpression clipboardActive(qmlContext(item), item, "systemTrayState.activeApplet !== null");
                    if (!clipboardActive.evaluate().toBool()) { fprintf(stderr, "FAIL: clipboard card did not open details\n"); std::_Exit(7); }
                    fprintf(stderr, "PASS: live visibility settings and custom tray cards\n");
                    QMetaObject::invokeMethod(item, "goHome");
                    QMetaObject::invokeMethod(item, "openApplet", Q_ARG(QVariant, QStringLiteral("org.kde.plasma.volume")));
                QTimer::singleShot(500, qApp, [item] {
                    bool active = false;
                    for (auto window : QGuiApplication::allWindows()) {
                        auto quick = qobject_cast<QQuickWindow *>(window);
                        if (!quick) continue;
                        for (auto root : items(quick->contentItem())) {
                            auto state = root->property("systemTrayState").value<QObject *>();
                            if (!state) continue;
                            active = state->property("activeApplet").value<QObject *>() != nullptr;
                            fprintf(stderr, "Details active=%d; expanded=%d; fallback=%d\n", active, state->property("expanded").toBool(), item->property("showAllItems").toBool());
                            QMetaObject::invokeMethod(item, "goHome");
                            fprintf(stderr, "After Back: active=%d; expanded=%d\n", state->property("activeApplet").value<QObject *>() != nullptr, state->property("expanded").toBool());
                            active = active && state->property("activeApplet").value<QObject *>() == nullptr
                                && state->property("expanded").toBool();
                        }
                    }
                    fprintf(stderr, "Native audio details and return to dashboard: %s\n", active ? "PASS" : "FAIL");
                    // plasmawindowed tears down the font database before tray items.
                    // Avoid that upstream shutdown crash in this test-only process.
                    std::_Exit(active ? 0 : 3);
                });
                };
                (*next)();
                return;
            }
        }
        qWarning("Dashboard was not found in native tray");
        qApp->exit(1);
    });
}
Q_COREAPP_STARTUP_FUNCTION(testStartup)
