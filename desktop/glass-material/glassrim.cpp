// SPDX-License-Identifier: MIT
#include "glassrim.h"
#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QWidget>
#include <QtQml>

namespace SequoiaGlass {
Settings::Settings(QObject *parent) : QObject(parent)
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    m_path = directory + QStringLiteral("/sequoiaelevenrc");
    m_watcher.addPath(directory);
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(60);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Settings::reload);
    reload();
}
Settings *Settings::instance()
{
    static Settings *settings = new Settings(QCoreApplication::instance());
    return settings;
}
void Settings::reload()
{
    if (!m_watcher.files().contains(m_path) && QFile::exists(m_path)) m_watcher.addPath(m_path);
    QSettings config(m_path, QSettings::IniFormat);
    const qreal value = qBound(0, config.value(QStringLiteral("Windeco/GlassRimIntensity"), 100).toInt(), 200) / 100.0;
    if (qFuzzyCompare(value, m_intensity)) return;
    m_intensity = value;
    Q_EMIT intensityChanged();
    if (qobject_cast<QApplication *>(QCoreApplication::instance()))
        for (QWidget *widget : QApplication::allWidgets()) widget->update();
}
void paintRim(QPainter *painter, const QRectF &rect, qreal radius, qreal intensity, bool active)
{
    if (rect.isEmpty() || intensity <= 0) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->setBrush(Qt::NoBrush);
    radius = qMin(radius, qMin(rect.width(), rect.height()) / 2);
    auto alpha = [intensity](int value) { return qBound(0, qRound(value * intensity), 255); };
    QLinearGradient rim(rect.topLeft(), rect.topLeft() + QPointF(rect.width() * .7, rect.height() * .45));
    rim.setColorAt(0, QColor(255, 255, 255, alpha(active ? 48 : 28)));
    rim.setColorAt(.45, QColor(255, 255, 255, alpha(active ? 24 : 14)));
    rim.setColorAt(1, QColor(255, 255, 255, alpha(6)));
    painter->setPen(QPen(rim, 1));
    painter->drawRoundedRect(rect.adjusted(.5, .5, -.5, -.5), qMax(0., radius-.5), qMax(0., radius-.5));
    painter->setPen(QPen(QColor(0, 0, 0, alpha(active ? 20 : 12)), 1));
    painter->drawRoundedRect(rect.adjusted(1.5, 1.5, -1.5, -1.5), qMax(0., radius-1.5), qMax(0., radius-1.5));
    painter->restore();
}
Rim::Rim(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    connect(Settings::instance(), &Settings::intensityChanged, this, [this] { update(); });
    connect(this, &Rim::radiusChanged, this, [this] { update(); });
    connect(this, &Rim::activeChanged, this, [this] { update(); });
}
void Rim::paint(QPainter *painter)
{
    paintRim(painter, boundingRect(), m_radius, Settings::instance()->intensity(), m_active);
}
void registerTypes()
{
    // A process-wide guard also covers the native style and standalone QML
    // plugin being loaded together. qmlTypeId here recursively imports the
    // module when called from QQmlExtensionPlugin::registerTypes.
    auto app = QCoreApplication::instance();
    if (app->property("sequoiaGlassTypesRegistered").toBool()) return;
    app->setProperty("sequoiaGlassTypesRegistered", true);
    qmlRegisterType<Rim>("org.sequoia.glass", 1, 0, "Rim");
    qmlRegisterSingletonType<Settings>("org.sequoia.glass", 1, 0, "Settings", [](QQmlEngine *, QJSEngine *) -> QObject * {
        auto settings = Settings::instance();
        QQmlEngine::setObjectOwnership(settings, QQmlEngine::CppOwnership);
        return settings;
    });
}
}
