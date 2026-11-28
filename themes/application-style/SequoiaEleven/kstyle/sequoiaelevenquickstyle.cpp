// SPDX-License-Identifier: MIT
#include "sequoiaelevenquickstyle.h"
#include "../../../../desktop/glass-material/glassrim.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>
#include <QDebug>
#include <QtQml>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickStyle>
#endif

namespace SequoiaEleven {
void initializeQuickStyle()
{
    // Both native style plugins call this during a live material switch.
    // QML registrations and import/style selection belong to application
    // startup, not to the lifetime of each replaceable QStyle instance.
    auto app = QCoreApplication::instance();
    if (app->property("sequoiaQuickStyleInitialized").toBool()) {
        return;
    }
    app->setProperty("sequoiaQuickStyleInitialized", true);
    SequoiaGlass::registerTypes();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // NewStuff is also hosted by QWidget apps (Dolphin, Konsole, games)
    // and Plasma. Expose only its module and the component library there;
    // leave their unrelated Quick/Plasma controls untouched.
    const QString shared = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/sequoia-eleven/newstuff/qml");
    QFile sharedManifest(shared + QStringLiteral("/manifest.json"));
    if (sharedManifest.open(QIODevice::ReadOnly)) {
        const auto sources = QJsonDocument::fromJson(sharedManifest.readAll()).object()
            .value(QStringLiteral("sources")).toObject();
        bool valid = !sources.isEmpty();
        for (auto it = sources.begin(); valid && it != sources.end(); ++it) {
            QFile source(it.key());
            QCryptographicHash hash(QCryptographicHash::Sha256);
            valid = source.open(QIODevice::ReadOnly) && hash.addData(&source)
                && hash.result().toHex() == it.value().toString().toLatin1();
        }
        if (valid) {
            const auto tooltipBackground = shared + QStringLiteral("/org/kde/plasma/core/DialogBackground.qml");
            if (QFileInfo::exists(tooltipBackground) && !QCoreApplication::instance()->property("sequoiaTooltipBackgroundRegistered").toBool()) {
                QCoreApplication::instance()->setProperty("sequoiaTooltipBackgroundRegistered", true);
                qmlRegisterType(QUrl::fromLocalFile(tooltipBackground), "org.kde.plasma.core", 2, 99, "DialogBackground");
            }
            // Sequoia is a QWidget style, not a Qt Quick style module. Avoid
            // Qt treating its native key as a missing QML controls import in
            // mixed-widget applications opening their first NewStuff window.
            qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");
            QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
            for (const char *key : {"QML_IMPORT_PATH", "QML2_IMPORT_PATH"}) {
                auto paths = qEnvironmentVariable(key).split(QDir::listSeparator(), Qt::SkipEmptyParts);
                paths.removeAll(shared);
                paths.prepend(shared);
                qputenv(key, paths.join(QDir::listSeparator()).toUtf8());
            }
        }
    }
    // QApplication creates its native style before applications construct their
    // QML engines. Cover direct commands and KCMLauncher as well as desktop
    // entries, without putting application QML overrides into Plasma itself.
    const QString executable = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    const QStringList applications = {
        QStringLiteral("systemsettings"), QStringLiteral("kinfocenter"),
        QStringLiteral("plasma-systemmonitor"), QStringLiteral("plasma-discover"),
        QStringLiteral("kcmshell6"), QStringLiteral("merkuro-calendar")
    };
    if (!applications.contains(executable)) {
        return;
    }
    const QString overlay = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/sequoia-eleven/quick-islands/qml");
    QFile manifest(overlay + QStringLiteral("/manifest.json"));
    if (!manifest.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto sources = QJsonDocument::fromJson(manifest.readAll()).object()
        .value(QStringLiteral("sources")).toObject();
    bool valid = !sources.isEmpty();
    for (auto it = sources.begin(); valid && it != sources.end(); ++it) {
        QFile source(it.key());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        valid = source.open(QIODevice::ReadOnly) && hash.addData(&source)
            && hash.result().toHex() == it.value().toString().toLatin1();
    }
    if (!valid) {
        qWarning("Sequoia Eleven: KDE components changed; refresh the Quick styling installation.");
        return;
    }
    for (const char *key : {"QML_IMPORT_PATH", "QML2_IMPORT_PATH"}) {
        auto paths = qEnvironmentVariable(key).split(QDir::listSeparator(), Qt::SkipEmptyParts);
        paths.removeAll(overlay);
        paths.prepend(overlay);
        qputenv(key, paths.join(QDir::listSeparator()).toUtf8());
    }
    qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
#endif
}
}
