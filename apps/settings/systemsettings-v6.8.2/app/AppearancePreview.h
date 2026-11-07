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
class QWidget;
class AppearancePreview : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(QUrl wallpaper READ wallpaper CONSTANT)
    Q_PROPERTY(bool shaderPreviews READ shaderPreviews CONSTANT)
    Q_PROPERTY(QString activeMaterial READ activeMaterial NOTIFY surfaceChanged)
    Q_PROPERTY(QColor canvasColor READ canvasColor NOTIFY surfaceChanged)
    Q_PROPERTY(QString colorMode READ colorMode NOTIFY surfaceChanged)
    Q_PROPERTY(bool automaticMode READ automaticMode NOTIFY surfaceChanged)
    Q_PROPERTY(QVariantList savedLooks READ savedLooks NOTIFY looksChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(int seconds READ seconds NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit AppearancePreview(QWidget *anchor, QObject *parent);
    ~AppearancePreview() override;
    bool enabled() const;
    QUrl wallpaper() const { return m_wallpaper; }
    bool shaderPreviews() const;
    QString activeMaterial() const { return m_activeMaterial; }
    QColor canvasColor() const { return m_canvasColor; }
    QString colorMode() const { return m_colorMode; }
    bool automaticMode() const { return m_automaticMode; }
    QVariantList savedLooks() const;
    Q_INVOKABLE void saveLook(const QString &name, const QString &material, const QString &mode);
    Q_INVOKABLE void previewLook(const QString &material, const QString &mode);
    Q_INVOKABLE void applyLook(const QString &material, const QString &mode);
    bool busy() const { return m_process.state() != QProcess::NotRunning; }
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
    void looksChanged();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void updateMaterial();
    void requestBackdrop();
    void startLook(const QString &material, const QString &mode, bool keep);
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
    QByteArray m_buffer;
    int m_seconds = 0;
    QString m_status;
};
