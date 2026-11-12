// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <QQueue>
#include <Plasma/Theme>

class PanelSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList panels READ panels NOTIFY changed)
    Q_PROPERTY(QVariantList screens READ screens NOTIFY changed)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
    Q_PROPERTY(int selectedId READ selectedId WRITE setSelectedId NOTIFY changed)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool opacitySupported READ opacitySupported NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit PanelSettings(QObject *parent = nullptr);
    QVariantList panels() const { return m_panels; }
    QVariantList screens() const { return m_screens; }
    QVariantMap selected() const;
    int selectedId() const { return m_selectedId; }
    void setSelectedId(int id);
    bool active() const { return m_active; }
    void setActive(bool active);
    bool busy() const { return m_busy; }
    bool opacitySupported() const { return m_theme.adaptiveTransparencyEnabled(); }
    QString error() const { return m_error; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setSetting(const QString &key, const QVariant &value);
    Q_INVOKABLE void editPanel();
Q_SIGNALS:
    void changed();
    void activeChanged();
    void busyChanged();
private:
    void dispatch(const QVariantMap &command = {});
    void enqueue(const QVariantMap &command);
    QTimer m_poll;
    Plasma::Theme m_theme;
    QVariantList m_panels, m_screens;
    QQueue<QVariantMap> m_pending;
    QString m_error, m_script;
    int m_selectedId = -1;
    bool m_active = false, m_busy = false;
};
