// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QQuickWindow>
#include <QQmlExtensionPlugin>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QDynamicPropertyChangeEvent>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

class Jungle5 : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
public:
    explicit Jungle5(QQuickItem *parent=nullptr):QQuickPaintedItem(parent) {
        setAntialiasing(true);m_clock.start();m_timer.setInterval(16);m_timer.setTimerType(Qt::PreciseTimer);
        connect(&m_timer,&QTimer::timeout,this,[this]{m_seconds=m_clock.elapsed()/1000.0;update();});
    }
    bool running() const {return m_timer.isActive();}
    void setRunning(bool active) {
        if(active==running())return;
        if(active){m_clock.restart();m_seconds=0;m_timer.start();update();}else m_timer.stop();
        emit runningChanged();
    }
    void paint(QPainter *p) override {
        QElapsedTimer timer;timer.start();
        paintJungle(*p,size(),m_seconds);
        if(qEnvironmentVariableIsSet("SEQUOIA_VINES_PROFILE"))
            qInfo("VINES_PAINT %.6f %lld",m_clock.nsecsElapsed()/1e9,static_cast<long long>(timer.nsecsElapsed()));
    }
signals:
    void runningChanged();
private:
    QTimer m_timer;QElapsedTimer m_clock;double m_seconds=0;
};

class Environment5 : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canShow READ canShow NOTIFY canShowChanged)
public:
    explicit Environment5(QObject *parent=nullptr):QObject(parent) {
        // A cheap poll also catches an already-active app entering fullscreen.
        m_poll.setInterval(250);
        connect(&m_poll,&QTimer::timeout,this,&Environment5::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::screenLockingChanged,this,&Environment5::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::hasActiveFullScreenEffectChanged,this,&Environment5::refresh);
        connect(KWin::effects,&KWin::EffectsHandler::windowActivated,this,&Environment5::refresh);
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

class VinesPlugin5 : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri) override {
        qmlRegisterType<Jungle5>(uri,1,0,"Jungle");
        qmlRegisterType<Environment5>(uri,1,0,"Environment");
    }
};
#include "plugin.moc"
