// SPDX-License-Identifier: MIT
// Extract the installed application's resources before it starts its backends.
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <cstdlib>

static void exportDiscoverQml()
{
    const auto target = qEnvironmentVariable("SEQUOIA_EXPORT_DISCOVER");
    if (target.isEmpty()) {
        return;
    }
    const QString source = QStringLiteral(":/qt/qml/org/kde/discover/qml");
    QDirIterator files(source, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (files.hasNext()) {
        const auto input = files.next();
        const auto output = target + input.mid(source.size());
        if (!QDir().mkpath(QFileInfo(output).absolutePath()) || !QFile::copy(input, output)) {
            std::_Exit(2);
        }
        ++count;
    }
    std::_Exit(count > 0 ? 0 : 3);
}
Q_COREAPP_STARTUP_FUNCTION(exportDiscoverQml)
