// SPDX-License-Identifier: GPL-2.0-or-later
#include "../motion.h"
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QWindow>
#include <QDBusConnection>
#include <QDBusMessage>
#include <cstdlib>
static void require(bool value,const char *message) {
    if(!value){fprintf(stderr,"FAIL: %s\n",message);std::_Exit(2);}
}
#include "../../../../tools/test-support/gwenview-menu-input.h"
class Backdrop : public QWidget {
public:
    int clicks=0;
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);QLinearGradient gradient(0,0,width(),height());
        gradient.setColorAt(0,QColor("#1b4645"));gradient.setColorAt(1,QColor("#408d7f"));p.fillRect(rect(),gradient);
        p.setPen(QColor(255,255,255,12));
        for(int x=0;x<width();x+=80)p.drawLine(x,0,x,height());
        for(int y=0;y<height();y+=80)p.drawLine(0,y,width(),y);
        p.setPen(QColor("#dfedcf"));p.setFont(QFont("sans",23));p.drawText(QRect(50,570,width()-100,60),Qt::AlignCenter,"Jungle Vines");
        p.setPen(QColor("#b3d5be"));p.setFont(QFont("sans",12));p.drawText(QRect(50,630,width()-100,40),Qt::AlignCenter,"A little canopy for your desktop");
    }
    void mousePressEvent(QMouseEvent *) override {++clicks;}
};
static QVariant effect(const QString &method) {
    auto request=QDBusMessage::createMethodCall("org.kde.KWin","/Effects","org.kde.kwin.Effects",method);
    request<<QStringLiteral("sequoia-vines-v5");auto reply=QDBusConnection::sessionBus().call(request);
    require(reply.type()==QDBusMessage::ReplyMessage,"effect D-Bus call");return reply.arguments().isEmpty()?QVariant():reply.arguments().first();
}
static int changed(const QImage &a,const QImage &b,const QRect &region) {
    int count=0;for(int y=region.top();y<=region.bottom();++y)for(int x=region.left();x<=region.right();++x)
        if(a.pixel(x,y)!=b.pixel(x,y))++count;
    return count;
}
static QRect monkeyBounds(const QImage &image) {
    QRect bounds;
    for(int y=65;y<350;++y)for(int x=0;x<image.width();++x) {
        const auto c=image.pixelColor(x,y);
        if(c.red()>100 && c.red()>c.green()*1.6 && c.red()>c.blue()*1.6)
            bounds=bounds.united(QRect(x,y,1,1));
    }
    return bounds;
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);Backdrop window;window.setWindowTitle("Vines input probe");window.resize(1400,900);window.showMaximized();
    QTimer::singleShot(1800,&app,[&] {
        require(effect("loadEffect").toBool() || effect("isEffectLoaded").toBool(),"load Jungle Vines effect");
        QElapsedTimer elapsed;elapsed.start();
        const auto timing=junglePose(1600,0);
        QTest::qWait(3500);
        const auto folder=qEnvironmentVariable("VINES_CAPTURE");
        const auto first=gwenviewCompositorCapture();require(first.save(folder+"/swing-1.png"),"save first swing frame");
        QTest::qWait(2000);const auto second=gwenviewCompositorCapture();second.save(folder+"/swing-2.png");
        require(changed(first,second,QRect(0,0,1600,450))>2500,"monkey and vines animate");
        const auto firstMonkey=monkeyBounds(first),secondMonkey=monkeyBounds(second);
        require(!firstMonkey.isEmpty() && !secondMonkey.isEmpty() && secondMonkey.center().x()>firstMonkey.center().x()+140,
                "monkey travels to later handholds across the screen");
        QTest::qWait(qMax(1,int((1.8+timing.journey+2.4)*1000)-int(elapsed.elapsed())));
        const auto exited=gwenviewCompositorCapture();exited.save(folder+"/monkey-exited.png");
        require(monkeyBounds(exited).isEmpty(),"monkey completely exits the screen before its next pass");
        QTest::qWait(qMax(1,int((timing.cycle+1.1)*1000)-int(elapsed.elapsed())));
        const auto descending=gwenviewCompositorCapture();descending.save(folder+"/vines-returning.png");
        require(monkeyBounds(descending).isEmpty() && changed(exited,descending,QRect(0,0,1600,200))>2500,
                "vines descend again before monkey returns");
        QTest::qWait(qMax(1,int((timing.cycle+3.5)*1000)-int(elapsed.elapsed())));
        const auto returning=gwenviewCompositorCapture();returning.save(folder+"/monkey-returning.png");
        require(!monkeyBounds(returning).isEmpty(),"monkey returns for another crossing");
        GwenviewMenuInput input;input.move(QPoint(800,220),1);input.button(true);QTest::qWait(40);input.button(false);QTest::qWait(100);
        require(window.clicks>0,"click passes through monkey and vines to the application");
        require(window.isActiveWindow(),"canopy does not steal application focus");
        auto toggle=QDBusMessage::createMethodCall("org.kde.kglobalaccel","/component/kwin","org.kde.kglobalaccel.Component","invokeShortcut");
        toggle<<QStringLiteral("SequoiaToggleVines");
        require(QDBusConnection::sessionBus().call(toggle).type()==QDBusMessage::ReplyMessage,"invoke registered vines shortcut");
        QTest::qWait(500);const auto hidden=gwenviewCompositorCapture();hidden.save(folder+"/toggle-off.png");
        require(changed(exited,hidden,QRect(0,0,1600,200))==0,"vines fully retract between crossings");
        require(changed(first,hidden,QRect(1000,345,550,35))==0,"transparent canopy does not blur the wallpaper behind empty pixels");
        require(changed(first,hidden,QRect(0,65,150,220))>300 && changed(first,hidden,QRect(1450,65,150,220))>300,
                "vines extend along both ends of the top edge");
        effect("unloadEffect");QTest::qWait(500);const auto unloaded=gwenviewCompositorCapture();
        require(changed(hidden,unloaded,QRect(250,40,1100,400))<10,"toggle hides entire canopy and unload leaves no orphan window");
        require(effect("loadEffect").toBool(),"reload vines effect");QTest::qWait(1700);
        const auto reloaded=gwenviewCompositorCapture();require(changed(hidden,reloaded,QRect(250,40,1100,400))>2500,"toggle test actually removed visible art");
        window.showFullScreen();QTest::qWait(900);const auto full=gwenviewCompositorCapture();full.save(folder+"/fullscreen.png");
        effect("unloadEffect");QTest::qWait(500);const auto clean=gwenviewCompositorCapture();
        require(changed(full,clean,QRect(250,0,1100,450))<10,"fullscreen app hides canopy");
        fprintf(stderr,"PASS: full-width vines, traveling monkey, offscreen exit and retracted canopy, no background blur, click-through, focus, shortcut, fullscreen hiding and reload\n");app.quit();
    });
    return app.exec();
}
