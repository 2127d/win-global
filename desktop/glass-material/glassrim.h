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
    Q_PROPERTY(int tooltipRevision READ tooltipRevision NOTIFY tooltipsChanged)
    Q_PROPERTY(QString applicationSection READ applicationSection CONSTANT)
public:
    explicit Settings(QObject *parent = nullptr);
    qreal intensity() const { return m_intensity; }
    static Settings *instance();
    int tooltipRevision() const { return m_tooltipRevision; }
    QString applicationSection() const;
    Q_INVOKABLE QString tooltipShape(const QString &section) const;
Q_SIGNALS:
    void intensityChanged();
    void tooltipsChanged();
private:
    void reload();
    QString m_path;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    qreal m_intensity = 1;
    int m_tooltipRevision = 0;
    QStringList m_shapes;
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
class TooltipSurface : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER m_color NOTIFY colorChanged)
    Q_PROPERTY(qreal radius MEMBER m_radius NOTIFY radiusChanged)
    Q_PROPERTY(QRegion mask READ mask NOTIFY maskChanged)
public:
    explicit TooltipSurface(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    QRegion mask() const;
    Q_INVOKABLE int popupMargin() const;
    Q_INVOKABLE void setPopupMargin(int margin);
Q_SIGNALS:
    void colorChanged();
    void radiusChanged();
    void maskChanged();
private:
    QColor m_color;
    qreal m_radius = 20;
};
void registerTypes();
}
