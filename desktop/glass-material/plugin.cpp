// SPDX-License-Identifier: MIT
#include "glassrim.h"
#include <QQmlExtensionPlugin>
class SequoiaGlassPlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *) override { SequoiaGlass::registerTypes(); }
};
#include "plugin.moc"
