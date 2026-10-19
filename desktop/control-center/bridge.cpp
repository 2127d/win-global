// SPDX-License-Identifier: MIT
// Redirect managed applet entry points to version-checked local QML.
#include <QCoreApplication>
#include <QUrl>
#include <QFileInfo>
#include <QResource>
#include <dlfcn.h>
#include <cstdlib>

static QString trayDirectory;
static bool dockResources = false;
static void initializeControlCenter()
{
    trayDirectory = qEnvironmentVariable("SEQUOIA_CONTROL_CENTER_DIR");
    if (!trayDirectory.isEmpty())
        dockResources = QResource::registerResource(trayDirectory + QStringLiteral("/taskmanager.rcc"));
    qunsetenv("SEQUOIA_CONTROL_CENTER_DIR");
    // Children launched by Plasma must not inherit this shell-only helper.
    const auto previous = qgetenv("SEQUOIA_CONTROL_CENTER_PRELOAD");
    if (previous.isEmpty()) qunsetenv("LD_PRELOAD");
    else qputenv("LD_PRELOAD", previous);
    qunsetenv("SEQUOIA_CONTROL_CENTER_PRELOAD");
}
Q_COREAPP_STARTUP_FUNCTION(initializeControlCenter)

// Only the method ABI is needed; we never access Applet's object layout.
namespace Plasma {
class Applet { public: QUrl mainScript() const; QString qrcPath() const; QString pluginName() const; };
QString Applet::qrcPath() const
{
    using Path = QString (*)(const Applet *);
    static auto original = reinterpret_cast<Path>(dlsym(RTLD_NEXT, "_ZNK6Plasma6Applet7qrcPathEv"));
    static auto name = reinterpret_cast<Path>(dlsym(RTLD_NEXT, "_ZNK6Plasma6Applet10pluginNameEv"));
    if (!original || !name) std::abort();
    if (dockResources && name(this) == QStringLiteral("org.kde.plasma.icontasks"))
        return QStringLiteral(":/sequoia/taskmanager/");
    return original(this);
}
QUrl Applet::mainScript() const
{
    using MainScript = QUrl (*)(const Applet *);
    static auto original = reinterpret_cast<MainScript>(dlsym(RTLD_NEXT, "_ZNK6Plasma6Applet10mainScriptEv"));
    if (!original) std::abort();
    const QUrl source = original(this);
    if (!trayDirectory.isEmpty() && source == QUrl(QStringLiteral("qrc:/qt/qml/plasma/applet/org/kde/merkuro/contact/applet/main.qml"))) {
        const QString local = trayDirectory + QStringLiteral("/contacts/main.qml");
        if (QFileInfo::exists(local)) return QUrl::fromLocalFile(local);
    }
    // Only exact entry points under this prefix can resolve into the overlay.
    const QString prefix = QStringLiteral("qrc:/qt/qml/plasma/applet/org/kde/plasma/");
    const QString url = source.toString();
    if (!trayDirectory.isEmpty() && url.startsWith(prefix) && url.endsWith(QStringLiteral("/main.qml"))) {
        QString applet = url.mid(prefix.size());
        applet.chop(QStringLiteral("/main.qml").size());
        if (!applet.contains('/') && !applet.contains('.') && !applet.isEmpty()) {
            if (applet == QStringLiteral("systemtray")) applet = QStringLiteral("tray");
            const QString local = trayDirectory + '/' + applet + QStringLiteral("/main.qml");
            if (QFileInfo::exists(local)) return QUrl::fromLocalFile(local);
        }
    }
    return source;
}
}
