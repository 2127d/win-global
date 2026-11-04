// SPDX-License-Identifier: MIT
// Test-only hook for an isolated plasmawindowed process, never installed.
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QBuffer>
#include <QAbstractButton>
#include <QDialog>
#include <QMimeData>
#include <QImage>
#include <QAbstractItemModel>
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
    if (qEnvironmentVariable("SEQUOIA_CLIPBOARD_TEST_PRIVATE") != "1") return;
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
                            if (page && name == "clipboard") {

                                const auto settle = [] { QEventLoop loop; QTimer::singleShot(450, &loop, &QEventLoop::quit); loop.exec(); };
                                auto check = [&](const QString &expression) {
                                    QQmlExpression test(qmlContext(page), page, expression);
                                    const bool ok = test.evaluate().toBool() && !test.hasError();
                                    fprintf(stderr, "%s: %s %s\n", ok ? "PASS" : "FAIL", qPrintable(expression), test.hasError() ? qPrintable(test.error().toString()) : "");
                                    found = found && ok;
                                    return ok;
                                };
                                const auto named = [&](const QString &name) -> QQuickItem* {
                                    for (auto child: items(page)) if (child->objectName() == name) return child;
                                    return nullptr;
                                };
                                const auto capture = [&](const QString &name) { settle(); return item->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_DETAIL_CAPTURE") + '-' + name + ".png"); };

                                auto run = [&](const QString &program, const QStringList &args, const QByteArray &input = QByteArray()) {
                                    QProcess process;
                                    QEventLoop loop;
                                    QObject::connect(&process, &QProcess::finished, &loop, &QEventLoop::quit);
                                    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
                                    process.start(program,args); process.waitForStarted();
                                    process.write(input); process.closeWriteChannel(); loop.exec();
                                    QByteArray result=process.readAllStandardOutput();
                                    fprintf(stderr,"COMMAND %s exit=%d err=%s\n",qPrintable(program),process.exitCode(),process.readAllStandardError().constData());
                                    return result;
                                };
                                const auto pos=item->window()->position();
                                run(qEnvironmentVariable("SEQUOIA_TEST_CLICK"),{QString::number(pos.x()+190),QString::number(pos.y()+30)});settle();
                                // This executable runs only in an isolated Wayland compositor, D-Bus session and XDG homes.
                                check("clipboardMenu.model.sourceCount === 0");
                                run("wl-copy",{"--type","text/plain","A quiet place for everything you copy."}); settle();
                                run("wl-copy",{"--type","text/plain","Design notes\nKeep the useful controls close. Make the content easy to scan."}); settle();
                                run("wl-copy",{"--type","text/plain","https://kde.org/plasma-desktop/"}); settle();
                                check("clipboardMenu.model.sourceCount === 3 && clipboardMenu.view.count === 3");
                                capture("history");
                                check("clipboardMenu.view.itemAtIndex(0) !== null");
                                for (auto child:items(page)) if(child->objectName().startsWith("clipboardMore:") && child->isVisible()) { QMetaObject::invokeMethod(child,"clicked");break; }
                                capture("actions");
                                for (auto window:QGuiApplication::allWindows()) if(auto quick=qobject_cast<QQuickWindow*>(window)) for(auto child:items(quick->contentItem())) if(child->objectName().startsWith("clipboardActions:")) QMetaObject::invokeMethod(child,"close");
                                settle();
                                check("clipboardMenu.view.itemAtIndex(1).toggleStar(); true"); settle();
                                check("clipboardMenu.model.starredCount === 1");
                                check("clipboardMenu.model.starredOnly = true; true"); settle();
                                check("clipboardMenu.view.count === 1"); capture("starred");
                                check("clipboardMenu.model.starredOnly = false; true"); settle();
                                auto search = named("clipboardSearch");
                                if (search) search->setProperty("text", "quiet place"); else found=false;
                                settle(); check("clipboardMenu.view.count === 1"); capture("search");
                                if (search) search->setProperty("text", "no matching item");
                                settle(); check("clipboardMenu.view.count === 0"); capture("no-results");
                                check("clipboardMenu.clearFilter(); true"); settle();
                                check("clipboardMenu.view.itemAtIndex(1).edit(); stack.depth === 2"); settle();
                                auto editor = named("clipboardEditor");
                                if (editor) { QMetaObject::invokeMethod(editor, "insert", Q_ARG(int, 0), Q_ARG(QString, QStringLiteral("Updated: "))); }
                                else found=false;
                                settle(); check("stack.currentItem.text.startsWith('Updated: ') && stack.currentItem.children.length > 0");
                                capture("editor");
                                auto save = named("clipboardEditSave");
                                if (save && save->property("enabled").toBool()) QMetaObject::invokeMethod(save,"clicked"); else { fprintf(stderr,"FAIL: edit save is disabled\n"); found=false; }
                                settle(); check("stack.depth === 1 && clipboardMenu.model.currentText.startsWith('Updated: ')");
                                check("clipboardMenu.view.itemAtIndex(1).barcode(); stack.depth === 2"); settle();
                                check("stack.currentItem.copyAction.enabled"); capture("qr");
                                auto qrCopy=named("clipboardQrCopy");
                                if (qrCopy) { auto point=qrCopy->mapToGlobal(QPointF(qrCopy->width()/2,qrCopy->height()/2));run(qEnvironmentVariable("SEQUOIA_TEST_CLICK"),{QString::number(point.x()),QString::number(point.y())}); } else found=false;
                                settle();
                                capture("qr-copied");
                                const auto qrData=run("wl-paste",{"--no-newline","--type","image/png"});
                                const bool copiedQr=!QImage::fromData(qrData).isNull();
                                fprintf(stderr,"%s: QR copy produces image\n",copiedQr?"PASS":"FAIL");found=found&&copiedQr;
                                QQmlExpression back(qmlContext(item), item, "systemTrayState.activeApplet.backAction.trigger()"); back.evaluate(); settle();
                                check("stack.depth === 1");
                                QImage sample(480,240,QImage::Format_RGB32);sample.fill(QColor("#8fa9c4"));
                                item->window()->requestActivate();settle();
                                QByteArray png; QBuffer buf(&png);buf.open(QIODevice::WriteOnly);sample.save(&buf,"PNG");
                                run("wl-copy",{"--type","image/png"},png);settle();settle();capture("image");
                                check("clipboardMenu.view.itemAtIndex(0).type === 4");
                                run("wl-copy",{"--type","text/uri-list"},QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_TEST_FILE")).toEncoded()+"\r\n");settle();settle();capture("files");
                                check("clipboardMenu.view.itemAtIndex(0).type === 8");
                                {
                                    QQmlExpression uuid(qmlContext(page), page, "clipboardMenu.view.itemAtIndex(0).uuid");
                                    auto removeButton = named("clipboardRemove:" + uuid.evaluate().toString());
                                    const bool invoked = removeButton && QMetaObject::invokeMethod(removeButton, "clicked");
                                    found = found && invoked;
                                    fprintf(stderr, "%s: direct delete button\n", invoked ? "PASS" : "FAIL");
                                }
                                settle();
                                check("clipboardMenu.view.itemAtIndex(0).type === 4");
                                check("clipboardMenu.view.itemAtIndex(2).itemSelected(); true");settle();
                                const bool copiedText=!run("wl-paste",{"--no-newline","--type","text/plain"}).isEmpty();found=found&&copiedText;fprintf(stderr,"%s: selecting history copies text\n",copiedText?"PASS":"FAIL");
                                check("clipboardMenu.clearHistory(); true");settle();
                                bool confirmed=false;
                                for (auto w: QApplication::topLevelWidgets()) {
                                    if (!w->isVisible() || !w->windowTitle().contains("Clear Clipboard")) continue;
                                    for (auto button:w->findChildren<QAbstractButton*>()) if (button->text().contains("Delete")) { confirmed=true;button->click();break; }
                                }
                                found=found&&confirmed;fprintf(stderr,"%s: native clear-history confirmation\n",confirmed?"PASS":"FAIL");
                                settle();settle();

                                capture("cleared");
                                check("clipboardMenu.model.sourceCount === 1 && clipboardMenu.model.starredCount === 1");
                                {
                                    QQmlExpression uuid(qmlContext(page), page, "clipboardMenu.view.itemAtIndex(0).uuid");
                                    auto removeButton = named("clipboardRemove:" + uuid.evaluate().toString());
                                    const bool invoked = removeButton && QMetaObject::invokeMethod(removeButton, "clicked");
                                    found = found && invoked;
                                    fprintf(stderr, "%s: direct delete button\n", invoked ? "PASS" : "FAIL");
                                }
                                settle();
                                bool removeConfirmed=false;
                                for (auto w: QApplication::topLevelWidgets()) {
                                    if (!w->isVisible() || !w->windowTitle().contains("Remove Starred")) continue;
                                    for (auto button:w->findChildren<QAbstractButton*>()) if (button->text().contains("Delete")) { removeConfirmed=true;button->click();break; }
                                }
                                found=found&&removeConfirmed;fprintf(stderr,"%s: native starred-removal confirmation\n",removeConfirmed?"PASS":"FAIL");
                                settle();check("clipboardMenu.model.sourceCount === 0 && clipboardMenu.view.count === 0");

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
