// SPDX-License-Identifier: MIT
// Check that plasmawindowed actually creates the installed applet's QML item.
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>
#include <cstdio>
#include <cstdlib>
#include <QAbstractItemModel>
#include <QSet>

static bool containsApplet(QQuickItem *item, const QString &entry)
{
    if (auto context = QQmlEngine::contextForObject(item)) {
        if (context->baseUrl().toLocalFile() == entry && item->isVisible()) {
            if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_TRAY_DEFAULTS")) {
                auto applet = item->property("_plasma_applet").value<QObject *>();
                auto model = applet ? applet->property("systemTrayModel").value<QAbstractItemModel *>() : nullptr;
                if (!model) return false;
                const auto roles = model->roleNames();
                QSet<QString> loaded;
                for (int row = 0; row < model->rowCount(); ++row) {
                    auto index = model->index(row, 0);
                    if (model->data(index, roles.key("applet")).value<QObject *>())
                        loaded.insert(model->data(index, roles.key("itemId")).toString());
                }
                for (const auto name : {"org.sequoia.notifications", "org.sequoia.volume", "org.sequoia.clipboard"})
                    if (!loaded.contains(QString::fromLatin1(name))) return false;
                std::fprintf(stdout, "PASS: fresh tray loads custom notifications, volume and clipboard without seeded settings\n");
            }
            return true;
        }
    }
    for (auto child : item->childItems())
        if (containsApplet(child, entry)) return true;
    return false;
}

static void verify()
{
    auto timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, [] {
        const auto entry = qEnvironmentVariable("SEQUOIA_VERIFY_WIDGET_ENTRY");
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (quick && containsApplet(quick->contentItem(), entry)) {
                std::fprintf(stdout, "PASS: loaded installed widget %s\n", qPrintable(entry));
                // Default tray loads real backends; their teardown can wait for
                // services absent from this private bus. The test owns this process.
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_TRAY_DEFAULTS")) {
                    std::fflush(stdout);
                    std::_Exit(0);
                }
                QCoreApplication::exit(0);
                return;
            }
        }
    });
    timer->start(100);
    QTimer::singleShot(8000, [] {
        std::fprintf(stderr, "FAIL: installed widget QML did not become visible\n");
        QCoreApplication::exit(1);
    });
}
Q_COREAPP_STARTUP_FUNCTION(verify)
