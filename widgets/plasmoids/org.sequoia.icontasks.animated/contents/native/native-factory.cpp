// SPDX-License-Identifier: MIT
// Give a QML fork its own applet identity while reusing KDE's installed backend.
// This is a normal applet factory, not a shell-wide preload or resource override.
#include <KPluginFactory>
#include <KPluginMetaData>
#include <QDebug>

class SequoiaAppletFactory : public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "native-metadata.json")
    Q_INTERFACES(KPluginFactory)
protected:
    QObject *create(const char *, QWidget *, QObject *parent, const QVariantList &args) override
    {
        const auto backend = metaData().value(QStringLiteral("X-Sequoia-NativePlugin"));
        const auto result = KPluginFactory::loadFactory(
            KPluginMetaData(QStringLiteral("plasma/applets/") + backend));
        if (!result.plugin) {
            qWarning() << "Cannot load Sequoia applet backend:" << backend << result.errorString;
            return nullptr;
        }
        auto factory = result.plugin;
        const auto originalMetadata = factory->metaData();
        factory->setMetaData(metaData());
        auto applet = factory->create<QObject>(parent, args);
        // The stock applet may be created later in the same shell. Its factory
        // must retain the original metadata after constructing our instance.
        factory->setMetaData(originalMetadata);
        return applet;
    }
};

#include "native-factory.moc"
