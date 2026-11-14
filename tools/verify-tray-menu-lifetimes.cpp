// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QMenu>
#include <QPluginLoader>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest/quicktest.h>

class MenuProbe : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE int visibleMenus() const {
        int count = 0;
        for (auto *widget : QApplication::topLevelWidgets())
            if (qobject_cast<QMenu *>(widget) && widget->isVisible()) ++count;
        return count;
    }
public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        engine->rootContext()->setContextProperty("nativeMenuProbe", this);
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QPluginLoader plugin(QStringLiteral("/usr/lib64/qt6/plugins/plasma/applets/org.kde.plasma.notifications.so"));
    if (!plugin.load()) qFatal("%s", qPrintable(plugin.errorString()));
    MenuProbe probe;
    return quick_test_main_with_setup(argc, argv, "tray-menu-lifetimes", nullptr, &probe);
}
#include "verify-tray-menu-lifetimes.moc"
