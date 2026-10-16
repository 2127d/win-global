// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QVariantMap>
#include <KSharedConfig>
#include <KConfigWatcher>

class TaskbarSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
public:
    explicit TaskbarSettings(QObject *parent = nullptr);
    QVariantMap values() const { return m_values; }
    bool dirty() const { return m_values != m_saved; }
    static QVariantMap defaultValues();
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void load();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void defaults();
    Q_INVOKABLE bool openSettings();
Q_SIGNALS:
    void valuesChanged();
    void dirtyChanged();
    void saveError(const QString &message);
private:
    QVariant normalized(const QString &key, const QVariant &value) const;
    KSharedConfig::Ptr m_config;
    KConfigWatcher::Ptr m_watcher;
    QVariantMap m_values, m_saved;
};
