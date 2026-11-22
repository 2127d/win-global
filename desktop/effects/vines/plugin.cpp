// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include "metrics.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QQuickWindow>
#include <QQmlExtensionPlugin>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QFileInfo>
#include <KConfig>
#include <KConfigGroup>
#include <algorithm>
#include <cmath>
#include <QDynamicPropertyChangeEvent>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

class Jungle9 : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(double swingSpeed MEMBER m_speed)
    Q_PROPERTY(double pauseSeconds MEMBER m_pause)
public:
    explicit Jungle9(QQuickItem *parent=nullptr):QQuickPaintedItem(parent) {
        m_metrics=new VinesFrameMetrics9(this);setAntialiasing(true);m_clock.start();
        connect(this,&QQuickItem::windowChanged,this,[this](QQuickWindow *window) {
            if(!window)return;
            connect(window,&QQuickWindow::frameSwapped,this,[this] {
                m_metrics->swapped();
                if(m_profile)qInfo("VINES_SWAP %.6f",m_clock.nsecsElapsed()/1e9);
            },Qt::DirectConnection);
        });
    }
    bool running() const {return m_running;}
    void setRunning(bool active) {
        if(active==m_running)return;
        m_running=active;
        if(active){m_clock.restart();m_seconds=0;update();}
        emit runningChanged();
    }
    // Called by Qt Quick's FrameAnimation in its animation phase, before
    // scene synchronization. No independent timer can miss that phase.
    Q_INVOKABLE void advance(double seconds) {
        if(!m_running)return;
        m_seconds=seconds;update();
    }
    void paint(QPainter *p) override {
        QElapsedTimer timer;timer.start();
        paintJungle(*p,size(),m_seconds,m_speed,m_pause);
        m_metrics->painted(timer.nsecsElapsed());
        if(m_profile)qInfo("VINES_PAINT %.6f %lld",m_clock.nsecsElapsed()/1e9,static_cast<long long>(timer.nsecsElapsed()));
    }
signals:
    void runningChanged();
private:
    VinesFrameMetrics9 *m_metrics=nullptr;
    double m_speed=1,m_pause=2.6;
    QElapsedTimer m_clock;double m_seconds=0;bool m_running=false;
    const bool m_profile=qEnvironmentVariableIntValue("SEQUOIA_VINES_PROFILE")!=0;
};

class Environment9 : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canShow READ canShow NOTIFY canShowChanged)
    Q_PROPERTY(double swingSpeed READ swingSpeed NOTIFY settingsChanged)
    Q_PROPERTY(double pauseSeconds READ pauseSeconds NOTIFY settingsChanged)
public:
    explicit Environment9(QObject *parent=nullptr):QObject(parent) {
        // A cheap poll also catches an already-active app entering fullscreen.
        m_poll.setInterval(250);
        connect(&m_poll,&QTimer::timeout,this,&Environment9::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::screenLockingChanged,this,&Environment9::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::hasActiveFullScreenEffectChanged,this,&Environment9::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::windowActivated,this,&Environment9::refresh);
        m_poll.start();refresh();
        // Generic KWin script settings save kwinrc without notifying running
        // scripts. Watch changes (including atomic replacement), never poll.
        m_configPath=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/kwinrc";
        m_watcher.addPath(QFileInfo(m_configPath).absolutePath());
        connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,&Environment9::readSettings);
        connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,&Environment9::readSettings);
        readSettings();
    }
    double swingSpeed() const {return m_speed;}
    double pauseSeconds() const {return m_pause;}
    bool canShow() const {return m_canShow;}
    Q_INVOKABLE void prepareWindow(QWindow *window) {
        window->setProperty("KWIN_SKIP_CLOSE_ANIMATION",true);
        window->setProperty("kwin_shadow_enabled",false);
        window->setProperty("sequoiaNoBlur",true);
        window->setProperty("kwin_blur",QVariant());
        window->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        // An empty valid blur region means the whole window. Remove the
        // request entirely, including requests made by styles after showing.
        if(event->type()==QEvent::DynamicPropertyChange
            && static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName()=="kwin_blur"
            && object->property("kwin_blur").isValid())
            object->setProperty("kwin_blur",QVariant());
        return QObject::eventFilter(object,event);
    }
private:
    void readSettings() {
        if(QFileInfo::exists(m_configPath) && !m_watcher.files().contains(m_configPath))m_watcher.addPath(m_configPath);
        KConfig config(m_configPath,KConfig::SimpleConfig);
        const KConfigGroup group(&config,"Script-sequoia-vines-v9");
        const double speed=std::clamp(group.readEntry("SwingSpeed",100),25,300)/100.0;
        double pause=group.readEntry("PauseSeconds",2.6);
        pause=std::isfinite(pause)?std::clamp(pause,0.0,3600.0):2.6;
        if(speed!=m_speed || pause!=m_pause) {m_speed=speed;m_pause=pause;emit settingsChanged();}
    }
    void refresh() {
        const auto active=KWin::effects->activeWindow();
        const bool show=!KWin::effects->isScreenLocked() && !KWin::effects->hasActiveFullScreenEffect()
            && !(active && active->isFullScreen());
        if(show!=m_canShow){m_canShow=show;emit canShowChanged();}
    }
signals:
    void canShowChanged();
    void settingsChanged();
private:
    QFileSystemWatcher m_watcher;QString m_configPath;double m_speed=1,m_pause=2.6;
    QTimer m_poll;bool m_canShow=false;
};

class VinesPlugin9 : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override {
        qmlRegisterType<Jungle9>(uri,1,0,"Jungle");
        qmlRegisterType<Environment9>(uri,1,0,"Environment");
    }
};
#include "plugin.moc"
