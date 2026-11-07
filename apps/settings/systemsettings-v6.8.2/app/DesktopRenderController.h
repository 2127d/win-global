// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariantMap>
#include <QSize>

class DesktopRenderController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap images READ images NOTIFY changed)
    Q_PROPERTY(QVariantMap errors READ errors NOTIFY changed)
    Q_PROPERTY(qreal aspectRatio READ aspectRatio CONSTANT)
public:
    explicit DesktopRenderController(QObject *parent = nullptr);
    ~DesktopRenderController() override;
    QVariantMap images() const { return m_images; }
    QVariantMap errors() const { return m_errors; }
    qreal aspectRatio() const { return qreal(m_size.width()) / m_size.height(); }
    Q_INVOKABLE void request(const QString &material, const QString &mode, bool priority);
    Q_INVOKABLE void refresh();
Q_SIGNALS:
    void changed();
    void invalidated();
private:
    QByteArray fingerprint();
    void startNext();
    void stop();
    QTemporaryDir m_directory;
    QProcess m_worker;
    QFileSystemWatcher m_watcher;
    QTimer m_dispatch;
    QTimer m_debounce;
    QSize m_size;
    QVariantMap m_images, m_errors;
    QStringList m_queue;
    QString m_runningKey, m_output;
    QByteArray m_fingerprint;
    int m_generation = 0, m_runningGeneration = 0;
    bool m_closing = false;
};
