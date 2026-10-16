// SPDX-License-Identifier: MIT
#include "settings.h"
#include <KConfigGroup>
#include <QProcess>
#include <QUrl>

QVariantMap TaskbarSettings::defaultValues()
{
    return {{"sizeMode", "fixed"}, {"iconSize", 32}, {"spacing", 6}, {"padding", 6},
            {"alignment", "left"}, {"showIndicators", true}, {"showLauncher", true},
            {"currentScreenOnly", false}, {"currentDesktopOnly", false},
            {"launchers", QStringList{"preferred://browser", "applications:org.kde.dolphin.desktop", "applications:org.kde.konsole.desktop"}}};
}

TaskbarSettings::TaskbarSettings(QObject *parent) : QObject(parent),
    m_config(KSharedConfig::openConfig(QStringLiteral("sequoiataskbarrc"))),
    m_watcher(KConfigWatcher::create(m_config))
{
    load();
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this,
            [this](const KConfigGroup &group, const QByteArrayList &) {
        if (group.name() == QLatin1String("Taskbar") && !dirty()) load();
    });
}

QVariant TaskbarSettings::normalized(const QString &key, const QVariant &value) const
{
    if (key == QLatin1String("iconSize")) return qBound(16, value.toInt(), 128);
    if (key == QLatin1String("spacing")) return qBound(0, value.toInt(), 64);
    if (key == QLatin1String("padding")) return qBound(0, value.toInt(), 32);
    if (key == QLatin1String("sizeMode"))
        return QStringList{"fixed", "fill", "panel"}.contains(value.toString()) ? value.toString() : QString("fixed");
    if (key == QLatin1String("alignment"))
        return QStringList{"left", "center", "right"}.contains(value.toString()) ? value.toString() : QString("left");
    if (key == QLatin1String("launchers")) {
        QStringList result;
        for (const auto &entry : value.toStringList()) {
            const auto clean = entry.trimmed();
            const QUrl url(clean);
            // Only desktop applications and KDE preferred application URLs.
            if (!clean.isEmpty() && !result.contains(clean)
                && ((url.scheme() == QLatin1String("applications") && clean.endsWith(QLatin1String(".desktop")))
                    || (url.scheme() == QLatin1String("preferred") && !url.host().isEmpty())
                    || (url.isLocalFile() && url.toLocalFile().endsWith(QLatin1String(".desktop")))))
                result.append(clean);
        }
        return result;
    }
    return value.toBool();
}

void TaskbarSettings::load()
{
    m_config->reparseConfiguration();
    const KConfigGroup group(m_config, "Taskbar");
    m_values = defaultValues();
    for (auto it = m_values.begin(); it != m_values.end(); ++it)
        it.value() = normalized(it.key(), group.readEntry(it.key(), it.value()));
    m_saved = m_values;
    Q_EMIT valuesChanged();
    Q_EMIT dirtyChanged();
}

void TaskbarSettings::setValue(const QString &key, const QVariant &value)
{
    if (!m_values.contains(key)) return;
    const auto next = normalized(key, value);
    if (next == m_values.value(key)) return;
    m_values[key] = next;
    Q_EMIT valuesChanged();
    Q_EMIT dirtyChanged();
}

bool TaskbarSettings::save()
{
    // Write only edited keys; another panel may have pinned an app meanwhile.
    m_config->reparseConfiguration();
    KConfigGroup group(m_config, "Taskbar");
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it)
        if (it.value() != m_saved.value(it.key()))
            group.writeEntry(it.key(), it.value(), KConfig::Notify);
    if (!m_config->sync()) {
        Q_EMIT saveError(tr("Could not save taskbar settings."));
        return false;
    }
    load();
    return true;
}

void TaskbarSettings::defaults()
{
    m_values = defaultValues();
    Q_EMIT valuesChanged();
    Q_EMIT dirtyChanged();
}

bool TaskbarSettings::openSettings()
{
    return QProcess::startDetached(QStringLiteral("systemsettings"), {QStringLiteral("kcm_sequoia_taskbar")});
}
