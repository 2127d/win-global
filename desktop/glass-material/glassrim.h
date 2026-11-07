// SPDX-License-Identifier: MIT
#pragma once
#include <QFileSystemWatcher>
#include <QQuickPaintedItem>
#include <QTimer>

namespace SequoiaGlass {
// One setting for every glass edge. Watching the directory also handles KConfig's
// atomic file replacements; no polling and no changes to surface opacity.
class Settings : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal intensity READ intensity NOTIFY intensityChanged)
public:
    explicit Settings(QObject *parent = nullptr);
    qreal intensity() const { return m_intensity; }
    static Settings *instance();
Q_SIGNALS:
    void intensityChanged();
private:
    void reload();
    QString m_path;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    qreal m_intensity = 1;
};
void paintRim(QPainter *painter, const QRectF &rect, qreal radius, qreal intensity, bool active = true);
class Rim : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(qreal radius MEMBER m_radius NOTIFY radiusChanged)
    Q_PROPERTY(bool active MEMBER m_active NOTIFY activeChanged)
public:
    explicit Rim(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
Q_SIGNALS:
    void radiusChanged();
    void activeChanged();
private:
    qreal m_radius = 14;
    bool m_active = true;
};
void registerTypes();
}
