// SPDX-License-Identifier: MIT
// Redirect only the installed greeter's main view; authentication stays native.
#include <QFile>
#include <QCryptographicHash>
#include <QUrl>
#include <QDebug>
#include <dlfcn.h>
#include <cstdlib>

#ifndef SEQUOIA_LOGIN_MAIN
#error Build with the absolute installed Main.qml path and greeter SHA-256.
#endif

using SetSource = void (*)(void *, const QUrl &);
using RootObject = void *(*)(const void *);

static void loadTheme(void *view, const QUrl &source, SetSource original, RootObject rootObject)
{
    if (!original) std::abort();
    if (source.toString() != QStringLiteral("qrc:/qt/qml/org/kde/plasma/login/Main.qml")) {
        original(view, source);
        return;
    }
    // A system update must not strand the user at a broken login screen.
    QFile executable(QStringLiteral("/proc/self/exe"));
    QCryptographicHash digest(QCryptographicHash::Sha256);
    const bool compatible = executable.open(QIODevice::ReadOnly) && digest.addData(&executable)
        && digest.result().toHex() == QByteArray(SEQUOIA_LOGIN_SHA256);
    if (!compatible || !rootObject || !QFile::exists(QStringLiteral(SEQUOIA_LOGIN_MAIN))) {
        qWarning("Sequoia login: using stock greeter; reinstall the theme after greeter updates.");
        original(view, source);
        return;
    }
    original(view, QUrl::fromLocalFile(QStringLiteral(SEQUOIA_LOGIN_MAIN)));
    if (!rootObject(view)) {
        qWarning("Sequoia login: theme failed to load; restoring stock greeter.");
        original(view, source);
    }
}

// Plasma Login 6.8 uses QQuickView directly. Keep the older view hook for
// supported installations which still use PlasmaQuick's shared-engine view.
extern "C" void sequoiaSetSource(void *view, const QUrl &source)
    asm("_ZN10QQuickView9setSourceERK4QUrl");
extern "C" void sequoiaSetSource(void *view, const QUrl &source)
{
    static const auto original = reinterpret_cast<SetSource>(dlsym(RTLD_NEXT,
        "_ZN10QQuickView9setSourceERK4QUrl"));
    static const auto rootObject = reinterpret_cast<RootObject>(dlsym(RTLD_NEXT,
        "_ZNK10QQuickView10rootObjectEv"));
    loadTheme(view, source, original, rootObject);
}

extern "C" void sequoiaSharedSetSource(void *view, const QUrl &source)
    asm("_ZN11PlasmaQuick21QuickViewSharedEngine9setSourceERK4QUrl");
extern "C" void sequoiaSharedSetSource(void *view, const QUrl &source)
{
    static const auto original = reinterpret_cast<SetSource>(dlsym(RTLD_NEXT,
        "_ZN11PlasmaQuick21QuickViewSharedEngine9setSourceERK4QUrl"));
    static const auto rootObject = reinterpret_cast<RootObject>(dlsym(RTLD_NEXT,
        "_ZNK11PlasmaQuick21QuickViewSharedEngine10rootObjectEv"));
    loadTheme(view, source, original, rootObject);
}

// The preload belongs only to this process, never to launched sessions/helpers.
__attribute__((constructor)) static void clearPreload() { unsetenv("LD_PRELOAD"); }
