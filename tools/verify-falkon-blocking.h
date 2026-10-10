// SPDX-License-Identifier: GPL-3.0-or-later
// Included by the test-only preload hook after its require/until/js helpers.
#include "adblockmanager.h"
#include "adblocksubscription.h"
#include "adblockrule.h"
#include "adblockdialog.h"
#include "navigationbartoolbutton.h"
#include "mainapplication.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QTreeView>
#ifdef Q_OS_LINUX
#include <signal.h>
#endif

static void verifyBlocking(BrowserWindow *window, const QString &output) {
    auto *manager = AdBlockManager::instance();
    require(manager->isEnabled() && manager->usesUBlock(), "stock uBlock core enabled at startup");
    QTcpServer server;
    require(server.listen(QHostAddress::LocalHost), "local fixture server");
    QStringList requests;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                QByteArray input = socket->property("input").toByteArray() + socket->readAll();
                socket->setProperty("input", input);
                if (!input.contains("\r\n\r\n") || socket->property("sent").toBool()) return;
                socket->setProperty("sent", true);
                const QString path = QString::fromLatin1(input.split(' ').value(1));
                requests.append(path);
                QByteArray body;
                QByteArray type = "application/javascript";
                if (path == "/") {
                    type = "text/html";
                    body = "<!doctype html><title>Native protection test</title>"
                           "<h1>Native protection test</h1><div class='sequoia-test-ad'>Advertisement</div>"
                           "<p>Allowed content stays visible.</p><script>window.fixture={};</script>"
                           "<script src='/allowed.js'></script><script src='/ads/cbr.js'></script>"
                           "<script src='/priority.js'></script>";
                } else if (path == "/allowed.js") body = "window.fixture.allowed=true;";
                else if (path == "/ads/cbr.js") body = "window.fixture.ad=true;";
                else if (path == "/priority.js") body = "window.fixture.priority=true;";
                else if (path.startsWith("/collect")) { type = "text/plain"; body = "ok"; }
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Type: "
                    + type + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    const QString origin = QString("http://127.0.0.1:%1").arg(server.serverPort());
    auto *view = window->weView();
    auto load = [&] {
        requests.clear();
        bool finished = false;
        const auto connection = QObject::connect(view->page(), &QWebEnginePage::loadFinished,
                                                 &server, [&](bool ok) { finished = ok; });
        view->load(QUrl(origin + "/"));
        require(until([&] { return finished; }), "HTTP fixture loaded");
        QObject::disconnect(connection);
        return QJsonDocument::fromJson(js(view->page(), "JSON.stringify(window.fixture)").toString().toUtf8()).object();
    };
    manager->setEnabled(false);
    auto result = load();
    require(result.value("ad").toBool() && result.value("allowed").toBool(), "unfiltered control scripts execute");
    manager->setEnabled(true);
    result = load();
    require(!result.contains("ad") && result.value("allowed").toBool(), "bundled EasyList blocks ad and allows content");
    require(!requests.contains("/ads/cbr.js"), "blocked request never reaches server");
    require(!manager->blockedRequestsForUrl(QUrl(origin + "/")).isEmpty(), "native blocked-request counter");

    auto *custom = manager->customList();
    auto add = [&](const QString &rule) { custom->addRule(new AdBlockRule(rule, custom)); };
    auto remove = [&](const QString &filter) {
        const auto rules = custom->allRules();
        for (int i = 0; i < rules.size(); ++i)
            if (rules[i]->filter() == filter) return custom->removeRule(i);
        return false;
    };
    const QString allow = "@@|" + origin + "/ads/cbr.js|$script";
    add(allow);
    result = load();
    require(result.value("ad").toBool(), "uBlock exception overrides stock block");
    require(remove(allow), "remove custom exception");

    // Important precedence distinguishes the real uBlock engine from Falkon's
    // old matcher. Standard CSS hiding still uses Falkon's native implementation.
    add("|" + origin + "/priority.js|$script,important");
    add("@@|" + origin + "/priority.js|$script");
    add("127.0.0.1##.sequoia-test-ad");
    add("|" + origin + "/collect$removeparam=track");
    result = load();
    require(!result.contains("priority"), "uBlock important rule overrides ordinary exception");
    require(until([&] { return js(view->page(), "getComputedStyle(document.querySelector('.sequoia-test-ad')).display").toString() == "none"; }), "native cosmetic CSS hiding");
    js(view->page(), "fetch('/collect?track=secret&keep=1').then(()=>window.collected=true)");
    require(until([&] { return js(view->page(), "window.collected===true").toBool(); }), "rewritten request completes");
    require(requests.contains("/collect?keep=1") && !requests.contains("/collect?track=secret&keep=1"), "tracking parameter removed before HTTP request");

    NavigationBarToolButton *button = nullptr;
    for (auto *candidate : window->findChildren<NavigationBarToolButton *>())
        if (candidate->toolTip().contains("uBlock")) button = candidate;
    require(button && button->isVisible(), "native protection button visible");
    QTest::mouseClick(button, Qt::LeftButton);
    QMenu *menu = nullptr;
    require(until([&] {
        for (auto *widget : QApplication::topLevelWidgets()) {
            auto *candidate = qobject_cast<QMenu *>(widget);
            if (candidate && candidate->isVisible()) { menu = candidate; return true; }
        }
        return false;
    }), "protection toolbar opens native menu");
    require(window->grab().save(output + "/protection.png"), "protection screenshot");
    QAction *pause = nullptr;
    for (auto *action : menu->actions()) if (action->text() == "Disable on 127.0.0.1") pause = action;
    require(pause, "per-site protection control");
    pause->trigger(); menu->close();
    result = load();
    require(result.value("ad").toBool() && result.value("priority").toBool(), "per-site pause bypasses filtering");
    require(js(view->page(), "getComputedStyle(document.querySelector('.sequoia-test-ad')).display").toString() != "none", "per-site pause disables cosmetic hiding");
    require(remove("@@||127.0.0.1^$document"), "remove per-site pause");
    mApp->reloadUserStyleSheet();
    result = load();
    require(!result.contains("ad"), "protection resumes");
    manager->save();
    custom->loadSubscription(manager->disabledRules());
    manager->updateMatcher();
    result = load();
    require(!result.contains("priority"), "saved custom rules survive disk reload");
#ifdef Q_OS_LINUX
    // Kill only this test browser's own filtering child, never another browser.
    bool killed = false;
    const auto tids = QDir("/proc/self/task").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &tid : tids) {
        QFile children("/proc/self/task/" + tid + "/children");
        if (!children.open(QFile::ReadOnly)) continue;
        const auto pids = children.readAll().trimmed().split(' ');
        for (const auto &pid : pids) {
            QFile command("/proc/" + QString::fromLatin1(pid) + "/cmdline");
            if (!command.open(QFile::ReadOnly)) continue;
            if (command.readAll().contains("/ublock/worker.mjs"))
                killed = (::kill(pid.toInt(), SIGKILL) == 0) || killed;
        }
    }
    require(killed, "located this browser's filtering worker");
    require(until([&] { return !manager->usesUBlock(); }), "worker failure detected");
    result = load();
    require(!result.contains("ad") && result.value("allowed").toBool(), "Falkon fallback keeps blocking after worker failure");
    require(manager->engineStatus().contains("fallback"), "fallback is disclosed in UI status");
    manager->updateMatcher();
    require(manager->usesUBlock(), "worker recovers on rule reload");
    result = load();
    require(!result.contains("priority"), "uBlock semantics restored after recovery");
#endif
    auto *dialog = manager->showDialog(window);
    require(dialog->findChild<QLabel *>("ublock-engine-status")->text().contains("uBlock Origin core"), "settings report actual engine");
    QTest::qWait(250);
    bool importantChecked = false;
    for (auto *tree : dialog->findChildren<QTreeView *>()) {
        for (int row = 0; row < tree->model()->rowCount(); ++row) {
            const auto index = tree->model()->index(row, 0);
            if (index.data().toString().contains("/priority.js|$script,important"))
                importantChecked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        }
    }
    require(importantChecked, "native settings show active uBlock-specific rule as enabled");
    require(dialog->grab().save(output + "/protection-settings.png"), "protection settings screenshot");
    dialog->close();
    fprintf(stderr, "PASS: uBlock default blocking, real HTTP suppression, exceptions, important precedence, CSS hiding, removeparam, per-site toolbar control, persistence and native settings\n");
}
