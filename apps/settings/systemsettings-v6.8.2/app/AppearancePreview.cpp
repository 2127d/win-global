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
#include <QDBusConnection>
#include <QDBusMessage>
#include <KLocalizedString>
#include <KIconTheme>
#include <KIconLoader>
#include <KSharedDataCache>
#include <QCollator>
#include <QSet>
#include <algorithm>
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
    refreshIconPacks();
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
        m_applyAccentAfterLook = false;
        m_status = i18n("Could not start the reversible material test: %1", m_process.errorString());
        Q_EMIT changed();
    });
    connect(&m_process, &QProcess::finished, this, [this](int code) {
        m_seconds = 0;
        m_keepWhenReady = false;
        updateMaterial();
        if (code != 0) m_status = i18n("The material test failed. %1", QString::fromUtf8(m_process.readAllStandardError()).left(300));
        const bool applyAccentNext = m_applyAccentAfterLook && code == 0;
        m_applyAccentAfterLook = false;
        if (applyAccentNext) applyPendingAccent();
        Q_EMIT changed();
    });
    m_accentTimeout.setSingleShot(true); m_accentTimeout.setInterval(15000);
    connect(&m_accentTimeout, &QTimer::timeout, &m_accentProcess, &QProcess::kill);
    connect(&m_accentProcess, &QProcess::started, this, [this] { m_accentTimeout.start(); });
    connect(&m_accentProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_accentTimeout.stop();
            KConfig colors(QStringLiteral("kdeglobals"), KConfig::FullConfig);
            colors.group(QStringLiteral("General")).writeEntry("accentColorFromWallpaper", m_previousWallpaperAccent, KConfig::Notify);
            colors.sync();
            m_status = i18n("Could not apply the accent color: %1", m_accentProcess.errorString());
            Q_EMIT changed();
        }
    });
    connect(&m_accentProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exit) {
        m_accentTimeout.stop();
        KConfig colors(QStringLiteral("kdeglobals"), KConfig::FullConfig);
        // KDE applies the palette; retain Sequoia's material alpha on the six
        // window/decoration surfaces it would otherwise replace from the scheme.
        for (auto it=m_accentSurfaceAlpha.cbegin(); it!=m_accentSurfaceAlpha.cend(); ++it) {
            const auto parts=it.key().split(QLatin1Char('/'));
            auto group=colors.group(parts[0]);
            QColor value=group.readEntry(parts[1], QColor());
            if (value.isValid()) { value.setAlpha(it.value()); group.writeEntry(parts[1], value, KConfig::Notify); }
        }
        if (code==0 && exit==QProcess::NormalExit) {
            auto general=colors.group(QStringLiteral("General"));
            general.writeEntry("AccentColor", m_requestedAccent, KConfig::Notify);
            general.writeEntry("LastUsedCustomAccentColor", m_requestedAccent, KConfig::Notify);
            general.writeEntry("accentColorFromWallpaper", false, KConfig::Notify);
            m_status=i18n("Accent color applied.");
        } else {
            colors.group(QStringLiteral("General")).writeEntry("accentColorFromWallpaper", m_previousWallpaperAccent, KConfig::Notify);
            m_status=i18n("Could not apply the accent color. %1", QString::fromUtf8(m_accentProcess.readAllStandardError()).left(200));
        }
        const bool saved = colors.sync();
        if (!saved) m_status = i18n("Could not save the accent color settings.");
        auto message=QDBusMessage::createSignal(QStringLiteral("/KGlobalSettings"), QStringLiteral("org.kde.KGlobalSettings"), QStringLiteral("notifyChange"));
        message << 0 << 0; QDBusConnection::sessionBus().send(message);
        updateMaterial();
        if (saved && code == 0 && exit == QProcess::NormalExit && m_pendingAccent == m_requestedAccent) {
            m_accentPending = false;
            Q_EMIT accentSelectionChanged();
        }
        Q_EMIT changed();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        revert();
        if (m_process.state()!=QProcess::NotRunning) m_process.waitForFinished(10000);
        if (m_accentProcess.state()!=QProcess::NotRunning) m_accentProcess.waitForFinished(16000);
    });
}
AppearancePreview::~AppearancePreview() {
    revert();
    if (m_process.state()!=QProcess::NotRunning) m_process.waitForFinished(10000);
    if (m_accentProcess.state()!=QProcess::NotRunning) m_accentProcess.waitForFinished(16000);
}
bool AppearancePreview::enabled() const { return qEnvironmentVariableIsSet("SEQUOIA_APPEARANCE_HUB"); }
bool AppearancePreview::shaderPreviews() const { return QQuickWindow::graphicsApi() != QSGRendererInterface::Software; }
void AppearancePreview::trial(const QString &material) {
    startLook(material, QStringLiteral("current"), false);
}
void AppearancePreview::previewLook(const QString &material, const QString &mode) { startLook(material, mode, false); }
void AppearancePreview::applyLook(const QString &material, const QString &mode) {
    if (busy() || accentBusy()) return;
    startLook(material, mode, true);
    m_applyAccentAfterLook = busy() && m_accentPending;
}
void AppearancePreview::stageAccent(const QString &name) {
    const QColor color(name);
    if (!enabled() || busy() || accentBusy() || !color.isValid() || color.alpha() != 255) return;
    m_pendingAccent = color;
    // Even the current color is an explicit choice to leave automatic accent mode.
    m_accentPending = true;
    m_status = i18n("Accent color selected. Click Apply to use it.");
    Q_EMIT accentSelectionChanged();
    Q_EMIT changed();
}
void AppearancePreview::discardAccent() {
    if (busy() || accentBusy() || !m_accentPending) return;
    m_accentPending = false;
    m_status = i18n("Unapplied accent color discarded.");
    Q_EMIT accentSelectionChanged();
    Q_EMIT changed();
}
void AppearancePreview::applyPendingAccent() {
    if (m_accentPending) applyAccent(m_pendingAccent.name());
}
void AppearancePreview::startLook(const QString &material, const QString &mode, bool keep) {
    if (!enabled() || busy() || accentBusy() || !QStringList{QStringLiteral("solid"), QStringLiteral("transparent"), QStringLiteral("glass"), QStringLiteral("restore")}.contains(material)) return;
    if (!QStringList{QStringLiteral("current"), QStringLiteral("light"), QStringLiteral("dark")}.contains(mode)) return;
    const QString script = QCoreApplication::applicationDirPath() + QStringLiteral("/appearance-material.py");
    if (!QFileInfo::exists(script)) { m_status = i18n("Install the preview helper before testing desktop materials."); Q_EMIT changed(); return; }
    m_status = i18n("Preparing a reversible desktop test…");
    m_keepWhenReady = keep;
    m_process.start(QStringLiteral("python3"), {script, material, QStringLiteral("--mode"), mode});
    Q_EMIT changed();
}
void AppearancePreview::keep() { if (m_process.state()!=QProcess::NotRunning) m_process.write("keep\n"); }
void AppearancePreview::revert() { m_applyAccentAfterLook = false; if (m_process.state()!=QProcess::NotRunning) m_process.write("revert\n"); }
void AppearancePreview::applyAccent(const QString &name) {
    const QColor accent(name);
    if (!enabled() || busy() || accentBusy() || !accent.isValid() || accent.alpha()!=255) return;
    // Match KDE's Colors settings, including the global theme's kdedefaults.
    KConfig colors(QStringLiteral("kdeglobals"), KConfig::FullConfig);
    auto general = colors.group(QStringLiteral("General"));
    const auto scheme=general.readEntry("ColorScheme", QStringLiteral("BreezeLight"));
    if (scheme.contains(QLatin1Char('/')) || QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(scheme)).isEmpty()) {
        m_status=i18n("Choose an installed color scheme in Colors before changing the accent."); Q_EMIT changed(); return;
    }
    m_accentSurfaceAlpha.clear();
    for (const QString &group : {QStringLiteral("Colors:Window"),QStringLiteral("Colors:Header"),QStringLiteral("WM")}) {
        const QStringList keys=group==QStringLiteral("WM") ? QStringList{QStringLiteral("activeBackground"),QStringLiteral("inactiveBackground")} : QStringList{QStringLiteral("BackgroundNormal"),QStringLiteral("BackgroundAlternate")};
        for (const auto &key:keys) {
            const QColor value=colors.group(group).readEntry(key,QColor());
            if (value.isValid()) m_accentSurfaceAlpha.insert(group+QLatin1Char('/')+key,value.alpha());
        }
    }
    for (const auto &key : {"AccentColor", "LastUsedCustomAccentColor", "accentColorFromWallpaper"}) {
        if (general.isEntryImmutable(key)) { m_status = i18n("Accent color settings are locked."); Q_EMIT changed(); return; }
    }
    // Stop automatic wallpaper updates before generating the custom palette.
    m_previousWallpaperAccent = general.readEntry("accentColorFromWallpaper", false);
    general.writeEntry("accentColorFromWallpaper", false, KConfig::Notify);
    if (!colors.sync()) { m_status = i18n("Could not save the accent color settings."); Q_EMIT changed(); return; }
    m_requestedAccent=accent;
    m_status=i18n("Applying accent color…");
    m_accentProcess.start(QStringLiteral("plasma-apply-colorscheme"), {QStringLiteral("--accent-color"),accent.name()});
    Q_EMIT changed();
}
namespace {
bool selectableIconPack(const QString &id, const KIconTheme &theme)
{
    // Match the Icons KCM: hide internal themes and GNOME-only packs that do
    // not implement the freedesktop icon names KDE applications need.
    return theme.isValid() && !theme.isHidden() && id != QStringLiteral("Adwaita")
        && id != QStringLiteral("Adwaita Dark") && id != QStringLiteral("HighContrast");
}
QString iconPackSample(const QString &id, const QString &icon)
{
    QStringList pending{id};
    QSet<QString> visited;
    for (int i = 0; i < pending.size(); ++i) {
        const auto name = pending.at(i);
        if (visited.contains(name)) continue;
        visited.insert(name);
        KIconTheme theme(name);
        if (!theme.isValid()) continue;
        const auto path = theme.iconPathByName(icon, 32, KIconLoader::MatchBest);
        if (!path.isEmpty()) return path;
        pending.append(theme.inherits());
    }
    return {};
}
}
void AppearancePreview::refreshIconPacks()
{
    KIconTheme::reconfigure();
    QVariantList packs;
    for (const auto &id : KIconTheme::list()) {
        KIconTheme theme(id);
        if (!selectableIconPack(id, theme)) continue;
        QVariantList samples;
        for (const auto &icon : {QStringLiteral("folder"), QStringLiteral("user-home"), QStringLiteral("user-trash"), QStringLiteral("text-x-generic"), QStringLiteral("systemsettings")}) {
            auto path = iconPackSample(id, icon);
            if (path.isEmpty()) path = iconPackSample(KIconTheme::defaultThemeName(), icon);
            samples.append(QVariantMap{{QStringLiteral("icon"), icon}, {QStringLiteral("source"), path.isEmpty() ? QVariant(icon) : QVariant(QUrl::fromLocalFile(path))}});
        }
        packs.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("name"), theme.name()},
                                 {QStringLiteral("description"), theme.description()}, {QStringLiteral("samples"), samples}});
    }
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(packs.begin(), packs.end(), [&collator](const QVariant &a, const QVariant &b) {
        return collator.compare(a.toMap().value(QStringLiteral("name")).toString(), b.toMap().value(QStringLiteral("name")).toString()) < 0;
    });
    if (packs != m_iconPacks) { m_iconPacks = packs; Q_EMIT iconPacksChanged(); }
}
void AppearancePreview::applyIconPack(const QString &id)
{
    if (!enabled() || busy() || accentBusy() || id == m_iconPack) return;
    // Revalidate against installed packs instead of accepting a path from QML.
    refreshIconPacks();
    const bool installed = std::any_of(m_iconPacks.cbegin(), m_iconPacks.cend(), [&id](const QVariant &pack) {
        return pack.toMap().value(QStringLiteral("id")).toString() == id;
    });
    if (!installed) { m_status = i18n("This icon pack is no longer available."); Q_EMIT changed(); return; }
    KConfig config(QStringLiteral("kdeglobals"), KConfig::SimpleConfig);
    auto icons = config.group(QStringLiteral("Icons"));
    if (icons.isEntryImmutable("Theme")) { m_status = i18n("Icon settings are locked."); Q_EMIT changed(); return; }
    icons.writeEntry("Theme", id, KConfig::Notify);
    if (!config.sync()) { m_status = i18n("Could not save the icon pack."); Q_EMIT changed(); return; }
    KSharedConfig::openConfig(QStringLiteral("kdeglobals"))->reparseConfiguration();
    KIconTheme::reconfigure();
    KSharedDataCache::deleteCache(QStringLiteral("icon-cache"));
    for (int group = 0; group < KIconLoader::LastGroup; ++group) KIconLoader::emitChange(KIconLoader::Group(group));
    updateMaterial();
    m_status = i18n("Icon pack applied."); Q_EMIT changed();
}
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
    const QString iconPack = colors.group(QStringLiteral("Icons")).readEntry("Theme", KIconTheme::current());
    if (iconPack != m_iconPack) { m_iconPack = iconPack; Q_EMIT iconPackChanged(); }
    const QColor accent=colors.group(QStringLiteral("General")).readEntry("AccentColor", QColor());
    const QColor previousAccent = m_accentColor;
    m_accentColor=accent.isValid() && accent.alpha()>0 ? accent : colors.group(QStringLiteral("Colors:Selection")).readEntry("BackgroundNormal", qApp->palette().color(QPalette::Highlight));
    if (!m_accentPending && previousAccent != m_accentColor) Q_EMIT accentSelectionChanged();
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
    const int radius = window->isMaximized() || window->isFullScreen() ? 0 : 20;
    shape.addRoundedRect(QRectF(window->rect()), radius, radius);
    QRegion region(shape.toFillPolygon().toPolygon());
    // This region is the application content, below the native titlebar.
    // Only the lower corners meet the outer frame; the top must join the
    // decoration squarely, as the native widget style already does for KCMs.
    region += QRect(0, 0, window->width(), radius);
    const bool blur = m_activeMaterial != QStringLiteral("solid");
    KWindowEffects::enableBlurBehind(window->windowHandle(), blur, region);
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
