// SPDX-License-Identifier: GPL-2.0-or-later
#include "AppearancePreview.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QUuid>
#include <QStandardPaths>
#include <QWidget>
#include <QWindow>
#include <KWindowEffects>
#include <QPainterPath>
#include <QEvent>
#include <QQuickWindow>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KColorScheme>
#include <KLocalizedString>
AppearancePreview::AppearancePreview(QWidget *anchor, QObject *parent) : QObject(parent), m_anchor(anchor)
{
    setObjectName(QStringLiteral("appearancePreview"));
    KConfig config(QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc"), KConfig::SimpleConfig);
    auto containments = config.group(QStringLiteral("Containments"));
    for (const auto &id : containments.groupList()) {
        const QString image = containments.group(id).group(QStringLiteral("Wallpaper")).group(QStringLiteral("org.kde.image")).group(QStringLiteral("General")).readEntry("Image", QString());
        QUrl url(image);
        if (url.scheme().isEmpty()) url = QUrl::fromLocalFile(image);
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
            QDir images(url.toLocalFile() + QStringLiteral("/contents/images"));
            const auto candidates = images.entryList({QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.webp")}, QDir::Files, QDir::Name | QDir::Reversed);
            if (!candidates.isEmpty()) url = QUrl::fromLocalFile(images.filePath(candidates.first()));
        }
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile()) { m_wallpaper = url; break; }
    }
    m_status = i18n("Preview only. Your desktop is unchanged.");
    m_configWatcher.addPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    m_configDebounce.setSingleShot(true); m_configDebounce.setInterval(100);
    connect(&m_configWatcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_configDebounce.start(); });
    connect(&m_configWatcher, &QFileSystemWatcher::fileChanged, this, [this] { m_configDebounce.start(); });
    connect(&m_configDebounce, &QTimer::timeout, this, &AppearancePreview::updateMaterial);
    updateMaterial();
    if (enabled()) qApp->installEventFilter(this);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_buffer += m_process.readAllStandardOutput();
        while (m_buffer.contains('\n')) {
            const auto end = m_buffer.indexOf('\n');
            auto value = QJsonDocument::fromJson(m_buffer.left(end)).object();
            m_buffer.remove(0, end + 1);
            m_seconds = value.value(QStringLiteral("seconds")).toInt();
            m_status = value.value(QStringLiteral("message")).toString();
            if (m_seconds > 0) requestBackdrop();
            if (m_seconds > 0 && m_keepWhenReady) { m_keepWhenReady = false; keep(); }
            Q_EMIT changed();
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this] {
        m_status = i18n("Could not start the reversible material test: %1", m_process.errorString());
        Q_EMIT changed();
    });
    connect(&m_process, &QProcess::finished, this, [this](int code) {
        m_seconds = 0;
        m_keepWhenReady = false;
        updateMaterial();
        if (code != 0) m_status = i18n("The material test failed. %1", QString::fromUtf8(m_process.readAllStandardError()).left(300));
        Q_EMIT changed();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { revert(); if (busy()) m_process.waitForFinished(10000); });
}
AppearancePreview::~AppearancePreview() { revert(); if (busy()) m_process.waitForFinished(10000); }
bool AppearancePreview::enabled() const { return qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB"); }
bool AppearancePreview::shaderPreviews() const { return QQuickWindow::graphicsApi() != QSGRendererInterface::Software; }
void AppearancePreview::trial(const QString &material) {
    startLook(material, QStringLiteral("current"), false);
}
void AppearancePreview::previewLook(const QString &material, const QString &mode) { startLook(material, mode, false); }
void AppearancePreview::applyLook(const QString &material, const QString &mode) { startLook(material, mode, true); }
void AppearancePreview::startLook(const QString &material, const QString &mode, bool keep) {
    if (!enabled() || busy() || !QStringList{QStringLiteral("solid"), QStringLiteral("transparent"), QStringLiteral("glass"), QStringLiteral("restore")}.contains(material)) return;
    if (!QStringList{QStringLiteral("current"), QStringLiteral("light"), QStringLiteral("dark")}.contains(mode)) return;
    const QString script = QCoreApplication::applicationDirPath() + QStringLiteral("/appearance-material.py");
    if (!QFileInfo::exists(script)) { m_status = i18n("Install the preview helper before testing desktop materials."); Q_EMIT changed(); return; }
    m_status = i18n("Preparing a reversible desktop test…");
    m_keepWhenReady = keep;
    m_process.start(QStringLiteral("python3"), {script, material, QStringLiteral("--mode"), mode});
    Q_EMIT changed();
}
void AppearancePreview::keep() { if (busy()) m_process.write("keep\n"); }
void AppearancePreview::revert() { if (busy()) m_process.write("revert\n"); }
void AppearancePreview::moveWindow() { if (auto handle = m_anchor->window()->windowHandle()) handle->startSystemMove(); }
void AppearancePreview::windowAction(const QString &action) {
    QWidget *window = m_anchor->window();
    if (action == QStringLiteral("close")) window->close();
    else if (action == QStringLiteral("minimize")) window->showMinimized();
    else if (action == QStringLiteral("maximize")) { if (window->isMaximized()) window->showNormal(); else window->showMaximized(); }
}

void AppearancePreview::updateMaterial() {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    for (const auto &file : {QStringLiteral("kwinrc"), QStringLiteral("kdeglobals"), QStringLiteral("plasmarc"), QStringLiteral("sequoiaelevenrc")}) {
        const QString path = directory + QLatin1Char('/') + file;
        if (QFileInfo::exists(path) && !m_configWatcher.files().contains(path)) m_configWatcher.addPath(path);
    }
    KConfig kwin(QStringLiteral("kwinrc"), KConfig::SimpleConfig);
    KConfig colors(QStringLiteral("kdeglobals"), KConfig::SimpleConfig);
    QFile globalsFile(directory + QStringLiteral("/kdeglobals"));
    const QByteArray contents = globalsFile.open(QIODevice::ReadOnly) ? globalsFile.readAll() : QByteArray();
    const bool paletteChanged = contents != m_globalsContent;
    if (paletteChanged) {
        m_globalsContent = contents;
        auto shared = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
        shared->reparseConfiguration();
        const QPalette palette = KColorScheme::createApplicationPalette(shared);
        if (enabled() && qApp->palette() != palette) qApp->setPalette(palette);
    }
    const QStringList bg = colors.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal", QStringLiteral("242,243,245")).split(QLatin1Char(','));
    const bool translucent = bg.size() > 3 && bg.at(3).toInt() < 255;
    const QString material = !translucent ? QStringLiteral("solid") : kwin.group(QStringLiteral("Plugins")).readEntry("sequoia_liquid_glass_v6Enabled", false) ? QStringLiteral("glass") : QStringLiteral("transparent");
    const QString mode = bg.value(0).toInt() + bg.value(1).toInt() + bg.value(2).toInt() < 384 ? QStringLiteral("dark") : QStringLiteral("light");
    const bool automatic = colors.group(QStringLiteral("KDE")).readEntry("AutomaticLookAndFeel", false);
    // Use the same RGBA source as the native glass decoration. Overriding its
    // alpha in QML creates a visible seam between the titlebar and the canvas.
    const QColor canvasColor = colors.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal", qApp->palette().color(QPalette::Window));
    if (paletteChanged || canvasColor != m_canvasColor || material != m_activeMaterial || mode != m_colorMode || automatic != m_automaticMode) {
        m_canvasColor = canvasColor;
        m_activeMaterial = material; m_colorMode = mode; m_automaticMode = automatic; Q_EMIT surfaceChanged();
    }
    if (enabled()) {
        // Effect replacement and Wayland capability advertisement are asynchronous.
        for (int delay : {0, 300, 1200}) QTimer::singleShot(delay, this, &AppearancePreview::requestBackdrop);
    }
}

bool AppearancePreview::eventFilter(QObject *watched, QEvent *event) {
    if (enabled() && m_anchor && watched == m_anchor->window() && (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::PaletteChange || event->type() == QEvent::WindowStateChange)) {
        QTimer::singleShot(0, this, &AppearancePreview::requestBackdrop);
    }
    return QObject::eventFilter(watched, event);
}
void AppearancePreview::requestBackdrop() {
    if (!m_anchor) return;
    QWidget *window = m_anchor->window();
    if (!enabled() || !window->isVisible() || !window->windowHandle()) return;
    QPainterPath shape;
    shape.addRoundedRect(QRectF(window->rect()), window->isMaximized() ? 0 : 20, window->isMaximized() ? 0 : 20);
    const bool blur = m_activeMaterial != QStringLiteral("solid");
    KWindowEffects::enableBlurBehind(window->windowHandle(), blur, QRegion(shape.toFillPolygon().toPolygon()));
    window->setProperty("appearanceBlurRequested", blur);
}

QVariantList AppearancePreview::savedLooks() const {
    QVariantList result;
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/appearance-looks"));
    for (const auto &name : directory.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
        QFile file(directory.filePath(name));
        if (!file.open(QIODevice::ReadOnly)) continue;
        const auto look = QJsonDocument::fromJson(file.readAll()).object();
        if (QStringList{QStringLiteral("solid"), QStringLiteral("transparent"), QStringLiteral("glass")}.contains(look.value(QStringLiteral("material")).toString())
            && QStringList{QStringLiteral("light"), QStringLiteral("dark")}.contains(look.value(QStringLiteral("mode")).toString())) result.append(look.toVariantMap());
    }
    return result;
}
void AppearancePreview::saveLook(const QString &name, const QString &material, const QString &mode) {
    if (name.trimmed().isEmpty() || !QStringList{QStringLiteral("solid"), QStringLiteral("transparent"), QStringLiteral("glass")}.contains(material)
        || !QStringList{QStringLiteral("light"), QStringLiteral("dark")}.contains(mode)) return;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/appearance-looks");
    QDir().mkpath(directory);
    QSaveFile file(directory + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".json"));
    const QJsonObject look{{QStringLiteral("name"), name.trimmed().left(80)}, {QStringLiteral("material"), material}, {QStringLiteral("mode"), mode}};
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(look).toJson()) < 0 || !file.commit()) {
        m_status = i18n("Could not save this appearance preset."); Q_EMIT changed(); return;
    }
    m_status = i18n("Appearance preset saved."); Q_EMIT looksChanged(); Q_EMIT changed();
}
