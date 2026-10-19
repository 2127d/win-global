// SPDX-License-Identifier: MIT
#include <QQmlExtensionPlugin>
#include <qqml.h>
#include "settings.h"
class TaskbarPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override { qmlRegisterType<TaskbarSettings>(uri, 1, 0, "Settings"); }
};
#include "plugin.moc"
