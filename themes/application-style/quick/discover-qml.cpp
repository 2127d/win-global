// SPDX-License-Identifier: MIT
// Install the URL interceptor before Discover creates its embedded main module.
#include <QQmlAbstractUrlInterceptor>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QFileInfo>
#include <QImageReader>
#include <cstdlib>
#include <dlfcn.h>

class DiscoverUrls final : public QQmlAbstractUrlInterceptor {
public:
    explicit DiscoverUrls(QString directory) : directory(std::move(directory)) {}

    QUrl intercept(const QUrl &url, DataType) override
    {
        const QString prefix = QStringLiteral("/qt/qml/org/kde/discover/qml/");
        if (url.scheme() == QStringLiteral("qrc") && url.path().startsWith(prefix)) {
            const auto file = directory + QLatin1Char('/') + url.path().mid(prefix.size());
            if (QFileInfo::exists(file)) {
                return QUrl::fromLocalFile(file);
            }
        }
        return url;
    }

private:
    const QString directory;
};

// Attach before Discover imports its Navigation singleton. Installing after
// that import would create separate resource and local singleton instances.
extern "C" void discoverEngine(QQmlApplicationEngine *, QObject *)
    asm("_ZN21QQmlApplicationEngineC1EP7QObject");

extern "C" void discoverEngine(QQmlApplicationEngine *engine, QObject *parent)
{
    using Constructor = void (*)(QQmlApplicationEngine *, QObject *);
    static const auto original = reinterpret_cast<Constructor>(
        dlsym(RTLD_NEXT, "_ZN21QQmlApplicationEngineC1EP7QObject"));
    if (!original) {
        std::abort();
    }
    original(engine, parent);
    const auto directory = qEnvironmentVariable("SEQUOIA_DISCOVER_QML");
    if (!directory.isEmpty()
        && QFileInfo(QCoreApplication::applicationFilePath()).fileName() == QStringLiteral("plasma-discover")) {
        // Child processes may close inherited descriptors. Stop forwarding our
        // descriptor-based preload after the library has loaded in Discover.
        const auto preload = qgetenv("SEQUOIA_DISCOVER_PRELOAD");
        if (!preload.isEmpty()) {
            auto libraries = qgetenv("LD_PRELOAD").split(':');
            libraries.removeAll(preload);
            qputenv("LD_PRELOAD", libraries.join(':'));
            qunsetenv("SEQUOIA_DISCOVER_PRELOAD");
        }
        // Initialize formats on the GUI thread before screenshot workers start.
        QImageReader::supportedImageFormats();
        auto interceptor = new DiscoverUrls(directory);
        engine->addUrlInterceptor(interceptor);
        QObject::connect(engine, &QObject::destroyed, [interceptor] { delete interceptor; });
    }
}
