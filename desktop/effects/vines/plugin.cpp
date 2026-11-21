// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include "metrics.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QQuickWindow>
#include <QQmlExtensionPlugin>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QDynamicPropertyChangeEvent>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

class Jungle7 : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
public:
    explicit Jungle7(QQuickItem *parent=nullptr):QQuickPaintedItem(parent) {
        m_metrics=new VinesFrameMetrics7(this);setAntialiasing(true);m_clock.start();
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
        paintJungle(*p,size(),m_seconds);
        m_metrics->painted(timer.nsecsElapsed());
        if(m_profile)qInfo("VINES_PAINT %.6f %lld",m_clock.nsecsElapsed()/1e9,static_cast<long long>(timer.nsecsElapsed()));
    }
signals:
    void runningChanged();
private:
    VinesFrameMetrics7 *m_metrics=nullptr;
    QElapsedTimer m_clock;double m_seconds=0;bool m_running=false;
    const bool m_profile=qEnvironmentVariableIntValue("SEQUOIA_VINES_PROFILE")!=0;
};

class Environment7 : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canShow READ canShow NOTIFY canShowChanged)
public:
    explicit Environment7(QObject *parent=nullptr):QObject(parent) {
        // A cheap poll also catches an already-active app entering fullscreen.
        m_poll.setInterval(250);
        connect(&m_poll,&QTimer::timeout,this,&Environment7::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::screenLockingChanged,this,&Environment7::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::hasActiveFullScreenEffectChanged,this,&Environment7::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::windowActivated,this,&Environment7::refresh);
        m_poll.start();refresh();
    }
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
    void refresh() {
        const auto active=KWin::effects->activeWindow();
        const bool show=!KWin::effects->isScreenLocked() && !KWin::effects->hasActiveFullScreenEffect()
            && !(active && active->isFullScreen());
        if(show!=m_canShow){m_canShow=show;emit canShowChanged();}
    }
signals:
    void canShowChanged();
private:
    QTimer m_poll;bool m_canShow=false;
};

class VinesPlugin7 : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override {
        qmlRegisterType<Jungle7>(uri,1,0,"Jungle");
        qmlRegisterType<Environment7>(uri,1,0,"Environment");
    }
};
#include "plugin.moc"
