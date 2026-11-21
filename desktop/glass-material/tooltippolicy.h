// SPDX-License-Identifier: MIT
#pragma once
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
namespace SequoiaGlass {
inline QString tooltipConfigPath() { return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/sequoiatooltipsrc"); }
inline QString tooltipDefault(const QString &section) { return section == QLatin1String("RunningApps") ? QStringLiteral("rounded") : QStringLiteral("pill"); }
inline QString tooltipShape(const QString &section) {
    QSettings config(tooltipConfigPath(), QSettings::IniFormat);
    const auto value = config.value(QStringLiteral("Tooltips/") + section, tooltipDefault(section)).toString();
    return value == QLatin1String("pill") || value == QLatin1String("rounded") || value == QLatin1String("square") ? value : tooltipDefault(section);
}
inline QString tooltipApplicationSection() {
    const auto name = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    return name.contains(QLatin1String("systemsettings")) || name.startsWith(QLatin1String("kcmshell")) || name.startsWith(QLatin1String("sequoiaeleven-settings"))
        ? QStringLiteral("Settings") : QStringLiteral("Applications");
}
inline qreal tooltipRadius(const QString &shape, qreal width, qreal height) {
    return shape == QLatin1String("pill") ? qMin(width, height) / 2 : shape == QLatin1String("rounded") ? qMin(20., qMin(width, height) / 2) : 0.;
}
}
