// SPDX-License-Identifier: MIT
// Runs only in the private test Plasma shell, never in the user's shell.
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTimer>
#include <QSet>
#include <QDir>
#include <cstdlib>
static void startGlobalMenuProbe()
{
    if (!qEnvironmentVariableIsSet("SEQUOIA_GLOBAL_MENU_PROBE")) return;
    QTimer::singleShot(1500, qApp, [] {
        auto timer = new QTimer(qApp);
        QObject::connect(timer, &QTimer::timeout, qApp, [] {
            static int samples = 0;
            int populated = 0;
            QList<QMenu *> menus;
            for (auto widget : QApplication::topLevelWidgets()) if (auto menu = qobject_cast<QMenu *>(widget)) menus << menu;
            for (auto menu : menus) {
                QSet<int> ids; QSet<QString> commands;
                bool imported = false;
                for (auto action : menu->actions()) {
                    const QVariant id = action->property("_dbusmenu_id");
                    if (!id.isValid()) continue;
                    imported = true;
                    const QString key = action->text() + '\t' + action->shortcut().toString();
                    if (ids.contains(id.toInt()) || (!action->isSeparator() && commands.contains(key))) {
                        fprintf(stderr, "FAIL: duplicated imported global-menu command: %s id=%d\n", qPrintable(action->text()), id.toInt());
                        std::_Exit(2);
                    }
                    ids.insert(id.toInt());
                    if (!action->isSeparator()) commands.insert(key);
                }
                if (!imported) continue;
                ++populated;
                // Exercise the widget's real asynchronous DBus importer, just
                // as opening File/Edit/View requests an up-to-date action list.
                QMetaObject::invokeMethod(menu, "aboutToShow");
            }
            if (populated >= 3) {
                ++samples;
                QFile ready(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-ready");
                if (ready.open(QIODevice::WriteOnly)) ready.write(QByteArray::number(samples));
            }
            if (QFile::exists(qEnvironmentVariable("XDG_RUNTIME_DIR") + "/global-menu-done")) {
                if (samples < 20) { fprintf(stderr, "FAIL: global menu did not receive enough updates\n"); std::_Exit(2); }
                fprintf(stderr, "PASS: %d samples of real Plasma global-menu imports without duplicate IDs or commands\n", samples);
                std::_Exit(0);
            }
        });
        timer->start(80);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startGlobalMenuProbe)
