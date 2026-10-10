// SPDX-License-Identifier: MIT
// Export the RPM's own embedded QML at QCoreApplication startup, before Merkuro
// opens services or calendar data. No KDE executable or library is modified.
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <cstdlib>

static void exportQml()
{
    const auto target = qEnvironmentVariable("SEQUOIA_EXPORT_MERKURO");
    if (target.isEmpty()) return;
    const QString source = QStringLiteral(":/qt/qml/org/kde/merkuro/calendar");
    QDirIterator files(source, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (files.hasNext()) {
        const auto input = files.next();
        const auto output = target + input.mid(source.size());
        if (!QDir().mkpath(QFileInfo(output).absolutePath()) || !QFile::copy(input, output)) std::_Exit(2);
        ++count;
    }
    std::_Exit(count > 0 ? 0 : 3);
}
Q_COREAPP_STARTUP_FUNCTION(exportQml)
