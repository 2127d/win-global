// SPDX-License-Identifier: MIT
#include "sequoiaelevenquickstyle.h"

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickStyle>
#endif

namespace SequoiaEleven {
void initializeQuickStyle()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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
