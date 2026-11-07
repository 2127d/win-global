// SPDX-License-Identifier: GPL-2.0-or-later
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusUnixFileDescriptor>
#include <QTemporaryFile>
#include <QImage>
#include <QTest>
#include <QTimer>
#include <QFont>
#include <QProcess>
#include <QPainterPath>
#include <KWindowEffects>
#include <cstdio>

static QVariant effect(const QString &method, const QString &name) {
    auto m = QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.kde.kwin.Effects", method);
    m << name;
    auto r = QDBusConnection::sessionBus().call(m, QDBus::Block, 5000);
    if (r.type() == QDBusMessage::ErrorMessage) qFatal("Effect call: %s", qPrintable(r.errorMessage()));
    return r.arguments().isEmpty() ? QVariant() : r.arguments().first();
}
class Controls : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void glass(bool enabled) {
        effect("unloadEffect", enabled ? "blur" : "sequoia_liquid_glass_v6");
        const QString name = enabled ? "sequoia_liquid_glass_v6" : "blur";
        if (!effect("isEffectLoaded", name).toBool() && !effect("loadEffect", name).toBool()) qFatal("Effect failed to load");
    }
    void edgeThickness(double amount) {
        QProcess::execute("kwriteconfig6", {"--file", "kwinrc", "--group", "Effect-sequoia_liquid_glass", "--key", "EdgeThickness", QString::number(amount)});
        effect("reconfigureEffect", "sequoia_liquid_glass_v6");
    }
    void geometry(bool enabled) {
        for (const auto &pair : {qMakePair(QString("Refraction"), QString(enabled ? "34" : "0")),
                                qMakePair(QString("LensStrength"), QString(enabled ? "0.09" : "0"))}) {
            QProcess::execute("kwriteconfig6", {"--file", "kwinrc", "--group", "Effect-sequoia_liquid_glass", "--key", pair.first, pair.second});
        }
        effect("reconfigureEffect", "sequoia_liquid_glass_v6");
    }
    Q_INVOKABLE void refraction(double amount) {
        QProcess::execute("kwriteconfig6", {"--file", "kwinrc", "--group", "Effect-sequoia_liquid_glass", "--key", "Refraction", QString::number(amount)});
        effect("reconfigureEffect", "sequoia_liquid_glass_v6");
    }
    Q_INVOKABLE void light(double angle) {
        QProcess::execute("kwriteconfig6", {"--file", "kwinrc", "--group", "Effect-sequoia_liquid_glass", "--key", "LightAzimuth", QString::number(angle)});
        effect("reconfigureEffect", "sequoia_liquid_glass_v6");
    }
};
static QImage capture(const QString &name) {
    QTemporaryFile f;
    if (!f.open()) qFatal("Capture temporary file");
    auto m = QDBusMessage::createMethodCall("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2", "org.kde.KWin.ScreenShot2", "CaptureWorkspace");
    m << QVariantMap{{"hide-caller-windows", false}, {"include-cursor", false}} << QVariant::fromValue(QDBusUnixFileDescriptor(f.handle()));
    auto r = QDBusConnection::sessionBus().call(m, QDBus::Block, 5000);
    if (r.type() == QDBusMessage::ErrorMessage) qFatal("Screenshot: %s",qPrintable(r.errorMessage()));
    auto map=qdbus_cast<QVariantMap>(r.arguments().first());
    int bytes=map["height"].toInt()*map["stride"].toInt();
    for(int i=0;f.size()<bytes && i<200;++i) QTest::qWait(10);
    f.seek(0);auto b=f.readAll();if(b.size()!=bytes)qFatal("Incomplete screenshot");
    QImage im(reinterpret_cast<const uchar*>(b.constData()),map["width"].toInt(),map["height"].toInt(),map["stride"].toInt(),QImage::Format(map["format"].toInt()));
    auto image=im.copy();if(!image.save(qEnvironmentVariable("GLASS_CAPTURE")+'/'+name+".png"))qFatal("Save failed");return image;
}
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv); app.setFont(QFont("Noto Sans", 10)); app.setApplicationName("Sequoia Liquid Glass Lab");
    Controls controls; QQmlEngine engine; engine.rootContext()->setContextProperty("lab", &controls);
    QQmlComponent background(&engine);
    background.setData(R"(
import QtQuick
import QtQuick.Window
Window {
    id: backdrop; width: 1200; height: 800; visible: true
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnBottomHint
    color: "#234e85"
    property real phase: 0
    property bool spotVisible: false
    property real spotX: 650
    Rectangle { z: 2; x: backdrop.spotX; y: 580; width: 24; height: 24; color: "#ff1493"; visible: backdrop.spotVisible }
    Rectangle { anchors.fill: parent; gradient: Gradient {
        GradientStop { position: 0; color: "#15335b" }
        GradientStop { position: 0.48; color: "#397b9e" }
        GradientStop { position: 1; color: "#c09c8c" }
    } }
    Rectangle { x: 80 + backdrop.phase; y: -80; width: 400; height: 1100; rotation: 32; color: "#54c6cc"; opacity: 0.5 }
    Rectangle { x: 780 + backdrop.phase; y: -80; width: 170; height: 1100; rotation: 32; color: "#e3c994"; opacity: 0.8 }
    Repeater { model: 45; Rectangle { x: index * 32 + backdrop.phase; width: 1; height: backdrop.height; color: "#38ffffff" } }
    Repeater { model: 30; Rectangle { y: index * 32; height: 1; width: backdrop.width; color: "#38ffffff" } }
    Text { x: 30; y: 25; text: "SEQUOIA / MATERIAL LAB"; color: "#ddffffff"; font.pixelSize: 13; font.letterSpacing: 2 }
    Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 24; anchors.horizontalCenter: parent.horizontalCenter; text: "Live compositor backdrop  ·  Close this preview to return to your desktop"; color: "white"; font.pixelSize: 13 }
})",QUrl());
    auto backdrop=qobject_cast<QQuickWindow*>(background.create());
    if(!backdrop)qFatal("Background: %s",qPrintable(background.errorString()));
    backdrop->showFullScreen(); QTest::qWait(400);
    if(qEnvironmentVariableIsSet("GLASS_BACKDROP_ONLY")) {
        if(qEnvironmentVariableIsSet("GLASS_CAPTURE")) QTimer::singleShot(12000, &app, [&] {
            capture("discover-liquid-glass"); controls.glass(false); QTest::qWait(600);
            capture("discover-standard-blur"); app.quit();
        });
        return app.exec();
    }
    QQmlComponent pane(&engine);
    pane.setData(R"(
import QtQuick
import QtQuick.Window
import QtQuick.Controls
Window {
    id: pane; width: 820; height: 540; visible: true; color: "transparent"
    title: "Sequoia Liquid Glass — Experimental"
    flags: Qt.Window | Qt.FramelessWindowHint
    property bool glassEnabled: true
    Rectangle { anchors.fill: parent; radius: 28; color: "#38e4edf6"; border.color: "#50ffffff" }
    Column {
        x: 40; y: 35; spacing: 13
        Text { text: "Liquid glass"; color: "#ffffff"; font.pixelSize: 34; font.bold: true }
        Text { text: "Actual backdrop refraction. Directional light. Sharp app content."; color: "#f5ffffff"; font.pixelSize: 15 }
    }
    Rectangle {
        x: 40; y: 140; width: 220; height: 300; radius: 22; color: "#f3f5f8"
        Text { x: 22; y: 22; text: "Discover"; color: "#202b3c"; font.pixelSize: 21; font.bold: true }
        Repeater { model: ["Home", "Installed", "Updates", "Settings", "About"]
            Rectangle { x: 12; y: 64 + index * 43; width: 196; height: 36; radius: 18; color: index === 0 ? "#d9e7fa" : "transparent"
                Text { x: 17; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: index === 0 ? "#286bbd" : "#273244"; font.pixelSize: 14 }
            }
        }
    }
    Rectangle {
        x: 282; y: 140; width: 498; height: 165; radius: 22; color: "#f3f5f8"
        Text { x: 24; y: 24; text: "Your content stays crisp"; font.pixelSize: 22; font.bold: true; color: "#202b3c" }
        Text { x: 24; y: 67; width: 450; wrapMode: Text.WordWrap; text: "The shader runs behind the app. This opaque card and its text are composed afterward, so they never bend or blur."; font.pixelSize: 15; color: "#566273" }
    }
    Text { x: 305; y: 343; text: "Move the light to inspect the glass rim"; color: "white"; font.pixelSize: 15 }
    Slider { x: 295; y: 375; width: 470; from: 0; to: 360; value: 225; onMoved: lab.light(value) }
    Text { x: 305; y: 428; text: "Refraction"; color: "white"; font.pixelSize: 14 }
    Slider { x: 400; y: 415; width: 365; from: 0; to: 48; value: 34; onMoved: lab.refraction(value) }
    Row { x: 40; y: 475; spacing: 12
        Button { text: pane.glassEnabled ? "Liquid glass on" : "Standard blur"; onClicked: { pane.glassEnabled = !pane.glassEnabled; lab.glass(pane.glassEnabled) } }
        Button { text: "Close experiment"; onClicked: Qt.quit() }
    }
})",QUrl());
    auto window=qobject_cast<QQuickWindow*>(pane.create());
    if(!window)qFatal("Pane: %s",qPrintable(pane.errorString()));
    QPainterPath mask;mask.addRoundedRect(QRectF(0,0,820,540),28,28);
    KWindowEffects::enableBlurBehind(window,true,QRegion(mask.toFillPolygon().toPolygon()));
    controls.glass(true);QTest::qWait(800);
    if(qEnvironmentVariableIsSet("GLASS_CAPTURE")) {
        const auto first=capture("liquid-glass");
        if(qEnvironmentVariableIsSet("GLASS_TOGGLE_CHECK")) {
            QImage stockWithGlassLoaded;
            for(int cycle=0;cycle<3;++cycle) {
                if(!effect("loadEffect", "blur").toBool())qFatal("Could not enable stock blur");
                QTest::qWait(350);
                stockWithGlassLoaded=capture(QString("stock-glass-paused-%1").arg(cycle));
                effect("unloadEffect", "blur");
                QTest::qWait(350);
                if(!KWindowEffects::isEffectAvailable(KWindowEffects::BlurBehind))qFatal("Blur capability lost after stock blur toggle");
                if(first!=capture(QString("toggle-restored-%1").arg(cycle)))qFatal("Glass pixels changed after stock blur toggle");
            }
            effect("loadEffect", "blur");QTest::qWait(350);
            effect("unloadEffect", "sequoia_liquid_glass_v6");QTest::qWait(350);
            if(!KWindowEffects::isEffectAvailable(KWindowEffects::BlurBehind))qFatal("Removing glass disabled stock blur capability");
            if(stockWithGlassLoaded!=capture("stock-alone"))qFatal("Stock blur was double processed while glass was loaded");
            effect("unloadEffect", "blur");QTest::qWait(350);
            if(KWindowEffects::isEffectAvailable(KWindowEffects::BlurBehind))qFatal("Blur capability remained after both providers were removed");
            controls.glass(true);QTest::qWait(500);
            if(!KWindowEffects::isEffectAvailable(KWindowEffects::BlurBehind) || first!=capture("glass-reenabled"))qFatal("Glass did not recover after both providers were removed");
            printf("PASS: three stock blur toggles, exact glass recovery, no double processing, capability ownership and re-enable\n");
            return 0;
        }
        controls.edgeThickness(0);QTest::qWait(400);const auto thin=capture("thin-rim");
        controls.edgeThickness(7);QTest::qWait(400);
        controls.geometry(false);QTest::qWait(500);const auto flat=capture("zero-warp");
        controls.geometry(true);QTest::qWait(500);
        controls.glass(false);QTest::qWait(500);const auto stock=capture("standard-blur");
        controls.glass(true); controls.light(45);QTest::qWait(500);const auto lit=capture("light-opposite");
        controls.light(225);backdrop->setProperty("phase",17);QTest::qWait(500);const auto moved=capture("backdrop-moved");
        backdrop->setProperty("phase",0);QTest::qWait(500);const auto restored=capture("backdrop-restored");
        backdrop->setProperty("spotVisible",true);QTest::qWait(400);capture("partial-damage-a");
        backdrop->setProperty("spotX",930);QTest::qWait(400);capture("partial-damage-b");
        backdrop->setProperty("spotVisible",false);QTest::qWait(400);const auto clean=capture("partial-damage-restored");
        if(first!=clean)qFatal("Partial backdrop updates left stale lens pixels");
        const QRect opaqueCard((first.width()-820)/2+306, (first.height()-540)/2+160, 440, 130);
        if(first.copy(opaqueCard)!=stock.copy(opaqueCard) || first.copy(opaqueCard)!=lit.copy(opaqueCard) || first.copy(opaqueCard)!=thin.copy(opaqueCard))
            qFatal("Opaque app content changed under the shader");
        long changed=0,relit=0,updated=0,stale=0,warped=0,polished=0;
        for(int y=0;y<first.height();++y)for(int x=0;x<first.width();++x){changed+=first.pixel(x,y)!=stock.pixel(x,y);polished+=first.pixel(x,y)!=thin.pixel(x,y);warped+=first.pixel(x,y)!=flat.pixel(x,y);relit+=first.pixel(x,y)!=lit.pixel(x,y);updated+=first.pixel(x,y)!=moved.pixel(x,y);stale+=first.pixel(x,y)!=restored.pixel(x,y);}
        printf("Shader difference: %ld pixels; lighting response: %ld; backdrop update: %ld\n",changed,relit,updated);
        printf("Opaque content unchanged; restored backdrop differs by %ld pixels\n",stale);
        printf("Refraction-only difference: %ld pixels\n",warped);
        printf("Polished rim difference: %ld pixels\n",polished);
        if(polished<1000 || warped<1000 || stale>200 || changed<1000 || relit<100 || updated<1000)qFatal("Glass rendering checks failed");
        return 0;
    }
    return app.exec();
}
#include "preview.moc"
