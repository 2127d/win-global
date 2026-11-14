// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QMenu>
#include <QPointer>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>
#include <qqml.h>

// KDE's file/audio menu wrappers do not expose close(). Close their native
// transient menus without replacing the wrappers or changing their actions.
class MenuCloser : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    Q_INVOKABLE void closeFor(QQuickWindow *owner) {
        if (!owner) return;
        QList<QPointer<QMenu>> menus;
        for (auto *widget : QApplication::topLevelWidgets()) {
            auto *menu = qobject_cast<QMenu *>(widget);
            if (!menu || !menu->isVisible() || !menu->windowHandle()) continue;
            for (auto *window = menu->windowHandle()->transientParent(); window; window = window->transientParent()) {
                if (window == owner) {
                    menus.append(menu);
                    break;
                }
            }
        }
        // Closing a parent can delete its submenus; guarded pointers avoid
        // retaining stale QWidget pointers while processing the snapshot.
        for (const auto &menu : menus)
            if (menu) menu->close();
    }
};
class NativeMenusPlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override {
        qmlRegisterType<MenuCloser>(uri, 1, 0, "MenuCloser");
    }
};
#include "plugin.moc"
