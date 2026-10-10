// SPDX-License-Identifier: GPL-2.0-or-later
#include "SettingsNavigation.h"
#include "SidebarMode.h"
#include "MenuItem.h"
#include <KCModuleData>
#include <KLocalizedString>
#include <KPluginMetaData>
#include <QRegularExpression>
#include <algorithm>

SettingsNavigation::SettingsNavigation(MenuItem *root, SidebarMode *sidebar)
    : QObject(sidebar), m_sidebar(sidebar)
{
    setObjectName(QStringLiteral("settingsNavigation"));
    auto group = [this](const char *id, const QString &name, const QString &description, const char *icon) {
        m_groups.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(id)},
                                  {QStringLiteral("name"), name}, {QStringLiteral("description"), description},
                                  {QStringLiteral("icon"), QString::fromLatin1(icon)}});
    };
    group("appearance", i18n("Appearance"), i18n("Wallpaper, colors, themes, and fonts"), "preferences-desktop-theme-global");
    group("display", i18n("Displays & Sound"), i18n("Screens, scaling, speakers, and microphones"), "preferences-desktop-display");
    group("devices", i18n("Connected Devices"), i18n("Keyboard, mouse, Bluetooth, and printers"), "preferences-devices");
    group("network", i18n("Internet & Network"), i18n("Connections, online accounts, and sharing"), "preferences-system-network");
    group("desktop", i18n("Desktop & Apps"), i18n("Windows, shortcuts, notifications, and applications"), "preferences-desktop");
    group("power", i18n("Power & Battery"), i18n("Energy use, sleep, and power saving"), "preferences-system-power-management");
    group("accessibility", i18n("Accessibility"), i18n("Tools for vision, hearing, and interaction"), "preferences-desktop-accessibility");
    group("system", i18n("System & Accounts"), i18n("Users, privacy, language, and system information"), "preferences-system");
    collect(root);
    std::sort(m_catalog.begin(), m_catalog.end(), [](const QVariant &a, const QVariant &b) {
        return QString::localeAwareCompare(a.toMap().value(QStringLiteral("name")).toString(),
                                          b.toMap().value(QStringLiteral("name")).toString()) < 0;
    });
    for (auto &entry : m_groups) {
        auto map = entry.toMap();
        int count = 0;
        for (const auto &module : m_catalog) {
            count += module.toMap().value(QStringLiteral("group")) == map.value(QStringLiteral("id"));
        }
        map.insert(QStringLiteral("count"), count);
        entry = map;
    }
}

QString SettingsNavigation::groupFor(MenuItem *item) const
{
    const QString id = item->metaData().pluginId();
    // Only a few cross-category destinations need explicit placement. Everything
    // else follows KDE's metadata, including modules added by distributions.
    if (id == QLatin1String("kcm_pulseaudio") || id == QLatin1String("kcm_nighttime")) return QStringLiteral("display");
    if (id == QLatin1String("kcm_keys") || id == QLatin1String("kcm_kwinscreenedges")
        || id == QLatin1String("kcm_kwintouchscreen") || id == QLatin1String("kcm_autostart")
        || id == QLatin1String("kcm_smserver")) return QStringLiteral("desktop");
    if (id == QLatin1String("kcm_krdpserver") || id == QLatin1String("kcm_webshortcuts")) return QStringLiteral("network");
    if (id == QLatin1String("kcm_access")) return QStringLiteral("accessibility");
    if (id.contains(QLatin1String("power")) || id == QLatin1String("kcm_energyinfo")) return QStringLiteral("power");
    if (id == QLatin1String("kcm_bluetooth") || id == QLatin1String("kcm_printer_manager")) return QStringLiteral("devices");
    if (id == QLatin1String("kcm_wallpaper") || id == QLatin1String("kcm_splashscreen")) return QStringLiteral("appearance");
    QStringList categories;
    for (auto current = item; current; current = current->parent()) categories << current->category();
    categories << item->metaData().value(QStringLiteral("X-KDE-System-Settings-Parent-Category"))
               << item->metaData().value(QStringLiteral("X-KDE-System-Settings-Parent-Category-V2"));
    const QString path = categories.join(QLatin1Char('/'));
    if (path.contains(QLatin1String("appearance"))) return QStringLiteral("appearance");
    if (path.contains(QLatin1String("display")) || path.contains(QLatin1String("multimedia"))) return QStringLiteral("display");
    if (path.contains(QLatin1String("network"))) return QStringLiteral("network");
    if (path.contains(QLatin1String("input")) || path.contains(QLatin1String("keyboard"))
        || path.contains(QLatin1String("hardware"))) return QStringLiteral("devices");
    if (path.contains(QLatin1String("workspace")) || path.contains(QLatin1String("applications"))) return QStringLiteral("desktop");
    return QStringLiteral("system");
}

void SettingsNavigation::collect(MenuItem *item)
{
    // Category owners also exist as real leaf modules; collect the leaves so
    // their child pages remain individually reachable and are not duplicated.
    const auto metadata = item->metaData();
    if (!item->menu() && metadata.isValid() && !metadata.isHidden() && !m_items.contains(metadata.pluginId())) {
        const QString group = groupFor(item);
        QString groupName;
        for (const auto &entry : m_groups) {
            if (entry.toMap().value(QStringLiteral("id")).toString() == group)
                groupName = entry.toMap().value(QStringLiteral("name")).toString();
        }
        m_items.insert(metadata.pluginId(), item);
        if (item->moduleData()) {
            connect(item->moduleData(), &KCModuleData::relevantChanged, this, &SettingsNavigation::refresh);
        }
        m_catalog.append(QVariantMap{{QStringLiteral("id"), metadata.pluginId()},
                                    {QStringLiteral("name"), item->name()},
                                    {QStringLiteral("description"), item->comment()},
                                    {QStringLiteral("icon"), item->iconName()},
                                    {QStringLiteral("group"), group},
                                    {QStringLiteral("groupName"), groupName},
                                    {QStringLiteral("keywords"), item->keywords().join(QLatin1Char(' '))}});
    }
    for (auto child : item->children()) collect(child);
}

QVariantList SettingsNavigation::entries() const
{
    QVariantList result;
    const auto terms = m_query.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const auto &entry : m_catalog) {
        auto map = entry.toMap();
        auto item = m_items.value(map.value(QStringLiteral("id")).toString());
        map.insert(QStringLiteral("changedFromDefault"), item->showDefaultIndicator());
        if (terms.isEmpty() && m_group != QLatin1String("all") && !m_showIrrelevant && item->moduleData() && !item->moduleData()->isRelevant()) continue;
        if (terms.isEmpty() && m_group != QLatin1String("all") && map.value(QStringLiteral("group")).toString() != m_group) continue;
        const QString haystack = map.value(QStringLiteral("name")).toString() + QLatin1Char(' ')
            + map.value(QStringLiteral("description")).toString() + QLatin1Char(' ')
            + map.value(QStringLiteral("keywords")).toString() + QLatin1Char(' ')
            + map.value(QStringLiteral("id")).toString();
        if (std::all_of(terms.begin(), terms.end(), [&haystack](const QString &term) { return haystack.contains(term, Qt::CaseInsensitive); }))
            result.append(map);
    }
    return result;
}

QString SettingsNavigation::title() const
{
    if (!m_query.isEmpty()) return i18n("Search Results");
    if (m_group == QLatin1String("all")) return i18n("All Settings");
    for (const auto &entry : m_groups) {
        const auto map = entry.toMap();
        if (map.value(QStringLiteral("id")).toString() == m_group) return map.value(QStringLiteral("name")).toString();
    }
    return i18n("Make yourself at home");
}

QString SettingsNavigation::description() const
{
    if (!m_query.isEmpty()) return i18np("%1 matching settings page", "%1 matching settings pages", entries().size());
    if (m_group == QLatin1String("all")) return i18n("Every available settings page, in one place.");
    for (const auto &entry : m_groups) {
        const auto map = entry.toMap();
        if (map.value(QStringLiteral("id")).toString() == m_group) return map.value(QStringLiteral("description")).toString();
    }
    return i18n("Choose an area to personalize your computer.");
}

bool SettingsNavigation::browse(const QString &group)
{
    bool valid = group == QLatin1String("home") || group == QLatin1String("all");
    for (const auto &entry : m_groups) valid |= entry.toMap().value(QStringLiteral("id")).toString() == group;
    if (!valid || !m_sidebar->showOverview()) return false;
    m_group = group;
    m_query.clear();
    m_moduleOpen = false;
    Q_EMIT entriesChanged();
    Q_EMIT changed();
    return true;
}

bool SettingsNavigation::search(const QString &query)
{
    if (query == m_query && !m_moduleOpen) return true;
    if (!m_sidebar->showOverview()) return false;
    m_query = query;
    m_moduleOpen = false;
    Q_EMIT entriesChanged();
    Q_EMIT changed();
    return true;
}

bool SettingsNavigation::back()
{
    if (m_moduleOpen) {
        if (!m_sidebar->showOverview()) return false;
        m_moduleOpen = false;
        Q_EMIT changed();
        return true;
    }
    return browse(QStringLiteral("home"));
}

void SettingsNavigation::open(const QString &id)
{
    if (m_items.contains(id)) m_sidebar->openNavigationItem(m_items.value(id));
}

void SettingsNavigation::moduleLoaded(const QString &id)
{
    m_currentModule = id;
    m_moduleOpen = true;
    if (home() && m_items.contains(id)) {
        m_group = groupFor(m_items.value(id));
        Q_EMIT entriesChanged();
    }
    Q_EMIT changed();
}
