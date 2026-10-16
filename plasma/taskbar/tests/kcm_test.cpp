// SPDX-License-Identifier: MIT
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <KPluginFactory>
#include <KQuickConfigModule>
#include <cstdio>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const auto result = KQuickConfigModuleLoader::loadModule(KPluginMetaData(QStringLiteral(TASKBAR_KCM_FILE)));
    if (!result.plugin) {
        fprintf(stderr, "%s\n", qPrintable(result.errorString));
        return 1;
    }
    auto module = result.plugin;
    if (KPluginMetaData(QStringLiteral(TASKBAR_KCM_FILE)).value(QStringLiteral("X-KDE-System-Settings-Parent-Category")) != QLatin1String("appearance")) return 1;
    module->load();
    auto ui = module->mainUi();
    if (!ui) {
        fprintf(stderr, "%s\n", qPrintable(module->errorString()));
        return 1;
    }
    QQuickWindow window;
    window.resize(780, 900);
    ui->setParentItem(window.contentItem());
    ui->setSize(QSizeF(780, 900));
    window.show();
    QTimer::singleShot(2000, [&] {
        if (!ui->findChild<QObject *>(QStringLiteral("sizeModeControl"))
            || !ui->findChild<QObject *>(QStringLiteral("pinnedApplicationsControl"))) {
            fprintf(stderr, "Settings controls were not created\n");
            app.exit(1);
            return;
        }
        auto settings = module->property("settings").value<QObject *>();
        if (!settings || !QMetaObject::invokeMethod(settings, "setValue", Q_ARG(QString, "spacing"), Q_ARG(QVariant, 17)) || !module->needsSave()) {
            fprintf(stderr, "Changing a setting did not enable Apply\n");
            app.exit(1);
            return;
        }
        module->load();
        if (module->needsSave()) { app.exit(1); return; }
        if (argc > 1) window.grabWindow().save(QString::fromLocal8Bit(argv[1]));
        fprintf(stdout, "PASS: native Taskbar KCM and its controls loaded\n");
        app.quit();
    });
    return app.exec();
}
