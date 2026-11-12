// SPDX-License-Identifier: GPL-2.0-or-later
#include "DesktopRenderController.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QUrl>
#include <QImageReader>
#include <QDir>
#include <KLocalizedString>

DesktopRenderController::DesktopRenderController(QObject *parent) : QObject(parent) {
    setObjectName(QStringLiteral("desktopRenderer"));
    auto screen = QGuiApplication::primaryScreen();
    // Wayland's first Qt screen can be a rotated secondary display. Prefer an
    // available landscape display rather than rendering its portrait geometry.
    if (screen && screen->size().width() < screen->size().height()) {
        QScreen *landscape = nullptr;
        for (auto candidate : QGuiApplication::screens()) {
            const auto size = candidate->size();
            if (size.width() > size.height()
                && (!landscape || size.width() * size.height() > landscape->size().width() * landscape->size().height()))
                landscape = candidate;
        }
        if (landscape) screen = landscape;
    }
    m_size = screen ? screen->size() : QSize(1600,900);
    // Keep native resolution for ordinary screens; only bound very large ones.
    if (m_size.width() > 3840 || m_size.height() > 2160)
        m_size.scale(QSize(3840,2160), Qt::KeepAspectRatio);
    m_fingerprint = fingerprint();
    m_dispatch.setSingleShot(true); m_dispatch.setInterval(300);
    m_debounce.setSingleShot(true); m_debounce.setInterval(750);
    connect(&m_dispatch,&QTimer::timeout,this,&DesktopRenderController::startNext);
    connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,[this]{ m_debounce.start(); });
    connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,[this]{ m_debounce.start(); });
    connect(&m_debounce,&QTimer::timeout,this,[this]{
        const auto next=fingerprint();
        if (next!=m_fingerprint) { m_fingerprint=next; refresh(); }
    });
    connect(&m_worker,&QProcess::finished,this,[this](int code,QProcess::ExitStatus status){
        if (!m_closing && m_runningGeneration==m_generation) {
            if (status==QProcess::NormalExit && code==0 && QImageReader(m_output).canRead())
                m_images.insert(m_runningKey,QUrl::fromLocalFile(m_output));
            else m_errors.insert(m_runningKey,i18n("Desktop preview could not be rendered. Refresh to try again."));
        } else {
            for (const auto &suffix : {QString(),QStringLiteral(".json"),QStringLiteral(".log")}) QFile::remove(m_output+suffix);
        }
        m_runningKey.clear();
        Q_EMIT changed();
        if (!m_closing) m_dispatch.start();
    });
    connect(&m_worker,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){
        if (error!=QProcess::FailedToStart) return;
        m_errors.insert(m_runningKey,i18n("The desktop preview renderer could not start."));
        m_runningKey.clear(); Q_EMIT changed(); m_dispatch.start();
    });
    connect(qApp,&QCoreApplication::aboutToQuit,this,&DesktopRenderController::stop);
}

DesktopRenderController::~DesktopRenderController() { stop(); }
void DesktopRenderController::stop() {
    m_closing=true; m_dispatch.stop(); m_debounce.stop(); m_queue.clear();
    if (m_worker.state()!=QProcess::NotRunning) {
        m_worker.terminate();
        if (!m_worker.waitForFinished(6000)) { m_worker.kill(); m_worker.waitForFinished(1000); }
    }
}

QByteArray DesktopRenderController::fingerprint() {
    const auto directory=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!m_watcher.directories().contains(directory)) m_watcher.addPath(directory);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const auto &name : {"kdeglobals","kwinrc","sequoiaelevenrc","plasmarc","plasmashellrc","plasma-org.kde.plasma.desktop-appletsrc","gwenviewrc","kweather/kweatherrc"}) {
        const auto path=directory+QLatin1Char('/')+QLatin1String(name);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) { hash.addData(file.readAll()); if (!m_watcher.files().contains(path)) m_watcher.addPath(path); }
    }
    return hash.result();
}

void DesktopRenderController::request(const QString &material,const QString &mode,bool priority) {
    if (m_closing || !QStringList{QStringLiteral("solid"),QStringLiteral("transparent"),QStringLiteral("glass")}.contains(material) || !QStringList{QStringLiteral("current"),QStringLiteral("light"),QStringLiteral("dark")}.contains(mode)) return;
    const auto key=material+QLatin1Char('/')+mode;
    if (m_images.contains(key) || m_errors.contains(key) || (m_runningKey==key && m_runningGeneration==m_generation)) return;
    if (qEnvironmentVariable("SEQUOIA_DESKTOP_RENDERING")==QStringLiteral("0")) return;
    if (m_queue.contains(key)) { if (!priority) return; m_queue.removeAll(key); }
    if (priority) m_queue.prepend(key); else m_queue.append(key);
    m_dispatch.start();
}

void DesktopRenderController::refresh() {
    if (m_closing) return;
    ++m_generation; m_queue.clear(); m_images.clear(); m_errors.clear();
    if (m_worker.state()!=QProcess::NotRunning) m_worker.terminate();
    for (const auto &name : QDir(m_directory.path()).entryList(QDir::Files)) {
        const QString path=m_directory.path()+QLatin1Char('/')+name;
        if (m_worker.state()==QProcess::NotRunning || !path.startsWith(m_output)) QFile::remove(path);
    }
    Q_EMIT changed(); Q_EMIT invalidated();
}

void DesktopRenderController::startNext() {
    if (m_closing || m_worker.state()!=QProcess::NotRunning || m_queue.isEmpty()) return;
    m_runningKey=m_queue.takeFirst(); m_runningGeneration=m_generation;
    const auto parts=m_runningKey.split(QLatin1Char('/'));
    m_output=m_directory.path()+QStringLiteral("/%1-%2-%3.png").arg(m_generation).arg(parts[0],parts[1]);
    const auto helper=QCoreApplication::applicationDirPath()+QStringLiteral("/appearance-desktop-render.py");
    m_worker.start(QStringLiteral("python3"),{helper,QStringLiteral("--material"),parts[0],QStringLiteral("--mode"),parts[1],QStringLiteral("--output"),m_output,QStringLiteral("--width"),QString::number(m_size.width()),QStringLiteral("--height"),QString::number(m_size.height())});
    Q_EMIT changed();
}
