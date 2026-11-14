// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QColor>
#include <QProcess>
#include <QUrl>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QVariantList>
#include <QPointer>
#include <QMap>
class QWidget;
class AppearancePreview : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(QUrl wallpaper READ wallpaper CONSTANT)
    Q_PROPERTY(bool shaderPreviews READ shaderPreviews CONSTANT)
    Q_PROPERTY(QString activeMaterial READ activeMaterial NOTIFY surfaceChanged)
    Q_PROPERTY(QVariantList iconPacks READ iconPacks NOTIFY iconPacksChanged)
    Q_PROPERTY(QString iconPack READ iconPack NOTIFY iconPackChanged)
    Q_PROPERTY(QColor selectedAccentColor READ selectedAccentColor NOTIFY accentSelectionChanged)
    Q_PROPERTY(bool accentPending READ accentPending NOTIFY accentSelectionChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY surfaceChanged)
    Q_PROPERTY(QColor canvasColor READ canvasColor NOTIFY surfaceChanged)
    Q_PROPERTY(QString colorMode READ colorMode NOTIFY surfaceChanged)
    Q_PROPERTY(bool automaticMode READ automaticMode NOTIFY surfaceChanged)
    Q_PROPERTY(QVariantList savedLooks READ savedLooks NOTIFY looksChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool accentBusy READ accentBusy NOTIFY changed)
    Q_PROPERTY(int seconds READ seconds NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit AppearancePreview(QWidget *anchor, QObject *parent);
    ~AppearancePreview() override;
    bool enabled() const;
    QUrl wallpaper() const { return m_wallpaper; }
    bool shaderPreviews() const;
    QString activeMaterial() const { return m_activeMaterial; }
    QVariantList iconPacks() const { return m_iconPacks; }
    QString iconPack() const { return m_iconPack; }
    Q_INVOKABLE void refreshIconPacks();
    Q_INVOKABLE void applyIconPack(const QString &id);
    QColor accentColor() const { return m_accentColor; }
    Q_INVOKABLE void applyAccent(const QString &color);
    QColor selectedAccentColor() const { return m_accentPending ? m_pendingAccent : m_accentColor; }
    bool accentPending() const { return m_accentPending; }
    Q_INVOKABLE void stageAccent(const QString &color);
    Q_INVOKABLE void discardAccent();
    Q_INVOKABLE void applyPendingAccent();
    QColor canvasColor() const { return m_canvasColor; }
    QString colorMode() const { return m_colorMode; }
    bool automaticMode() const { return m_automaticMode; }
    QVariantList savedLooks() const;
    Q_INVOKABLE void saveLook(const QString &name, const QString &material, const QString &mode);
    Q_INVOKABLE void previewLook(const QString &material, const QString &mode);
    Q_INVOKABLE void applyLook(const QString &material, const QString &mode);
    bool busy() const { return m_process.state() != QProcess::NotRunning; }
    bool accentBusy() const { return m_accentProcess.state() != QProcess::NotRunning; }
    int seconds() const { return m_seconds; }
    QString status() const { return m_status; }
    Q_INVOKABLE void trial(const QString &material);
    Q_INVOKABLE void keep();
    Q_INVOKABLE void revert();
    Q_INVOKABLE void moveWindow();
    Q_INVOKABLE void windowAction(const QString &action);
Q_SIGNALS:
    void changed();
    void surfaceChanged();
    void accentSelectionChanged();
    void looksChanged();
    void iconPacksChanged();
    void iconPackChanged();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void updateMaterial();
    void requestBackdrop();
    void startLook(const QString &material, const QString &mode, bool keep);
    QVariantList m_iconPacks;
    QString m_iconPack;
    QString m_activeMaterial;
    QColor m_canvasColor;
    QString m_colorMode;
    bool m_automaticMode = false;
    bool m_keepWhenReady = false;
    QByteArray m_globalsContent;
    QFileSystemWatcher m_configWatcher;
    QTimer m_configDebounce;
    QPointer<QWidget> m_anchor;
    QUrl m_wallpaper;
    QProcess m_process;
    QProcess m_accentProcess;
    QTimer m_accentTimeout;
    QColor m_accentColor;
    QColor m_requestedAccent;
    QColor m_pendingAccent;
    bool m_accentPending = false;
    bool m_applyAccentAfterLook = false;
    bool m_previousWallpaperAccent = false;
    QMap<QString, int> m_accentSurfaceAlpha;
    QByteArray m_buffer;
    int m_seconds = 0;
    QString m_status;
};
