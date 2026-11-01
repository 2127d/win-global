// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QHash>

class MenuItem;
class SidebarMode;

// Presentation only: all module activation still goes through KDE's SidebarMode.
class SettingsNavigation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList groups READ groups CONSTANT)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QVariantList catalog READ catalog CONSTANT)
    Q_PROPERTY(QString selectedGroup READ selectedGroup NOTIFY changed)
    Q_PROPERTY(QString query READ query NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(bool moduleOpen READ moduleOpen NOTIFY changed)
    Q_PROPERTY(QString currentModule READ currentModule NOTIFY changed)
    Q_PROPERTY(bool home READ home NOTIFY changed)
public:
    SettingsNavigation(MenuItem *root, SidebarMode *sidebar);
    QVariantList groups() const { return m_groups; }
    QVariantList catalog() const { return m_catalog; }
    QVariantList entries() const;
    QString selectedGroup() const { return m_group; }
    QString query() const { return m_query; }
    QString title() const;
    QString description() const;
    bool moduleOpen() const { return m_moduleOpen; }
    QString currentModule() const { return m_currentModule; }
    bool home() const { return m_group == QStringLiteral("home") && m_query.isEmpty(); }
    Q_INVOKABLE bool browse(const QString &group);
    Q_INVOKABLE bool search(const QString &query);
    Q_INVOKABLE bool back();
    Q_INVOKABLE void open(const QString &id);
    void moduleLoaded(const QString &id);
    void refresh() { Q_EMIT entriesChanged(); Q_EMIT changed(); }
    void setShowIrrelevant(bool show) { m_showIrrelevant = show; refresh(); }
Q_SIGNALS:
    void changed();
    void entriesChanged();
private:
    void collect(MenuItem *item);
    QString groupFor(MenuItem *item) const;
    SidebarMode *m_sidebar;
    QVariantList m_groups;
    QVariantList m_catalog;
    QHash<QString, MenuItem *> m_items;
    QString m_group = QStringLiteral("home");
    QString m_query;
    QString m_currentModule;
    bool m_moduleOpen = false;
    bool m_showIrrelevant = true;
};
