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
    auto image=im.copy();if(!image.save(qEnvironmentVariable("FIREFOX_BLUR_CAPTURE")+'/'+name+".png"))qFatal("Save failed");return image;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setDesktopFileName("firefox");
    QQmlEngine engine;
    QQmlComponent backdropComponent(&engine);
    backdropComponent.setData(R"(
import QtQuick
import QtQuick.Window
Window {
    width: 1200; height: 800; color: "#17283e"
    Repeater { model: 120
        Rectangle { x: index * 10; width: 5; height: 800; color: index % 2 ? "#e08040" : "#70c0f0" }
    }
})", QUrl());
    auto backdrop = qobject_cast<QQuickWindow *>(backdropComponent.create());
    if (!backdrop) qFatal("Backdrop failed");
    backdrop->showFullScreen(); QTest::qWait(400);
    QQmlComponent paneComponent(&engine);
    paneComponent.setData(R"(
import QtQuick
import QtQuick.Window
Window {
    width: 640; height: 440; color: "transparent"; visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    Rectangle { anchors.fill: parent; color: "#50ffffff" }
    Rectangle { x: 100; y: 100; width: 440; height: 240; color: "#204080"
        Text { anchors.centerIn: parent; text: "Opaque website content"; color: "white"; font.pixelSize: 24 }
    }
})", QUrl());
    auto pane = qobject_cast<QQuickWindow *>(paneComponent.create());
    if (!pane) qFatal("Pane failed");
    const QString id = "sequoia_firefox_blur_v2";
    effect("unloadEffect", id); QTest::qWait(700);
    const auto off = capture("disabled");
    if (!effect("loadEffect", id).toBool()) qFatal("Firefox blur failed to load");
    QTest::qWait(700); const auto on = capture("enabled");
    const QRect opaque((on.width()-640)/2+110, (on.height()-440)/2+110, 420, 220);
    if (off.copy(opaque) != on.copy(opaque)) qFatal("Opaque content changed");
    long changed = 0;
    for (int y=0;y<on.height();++y) for(int x=0;x<on.width();++x) changed += on.pixel(x,y)!=off.pixel(x,y);
    if (changed < 1000) qFatal("Firefox blur did not change translucent pixels");
    effect("unloadEffect", id); QTest::qWait(500);
    if (off != capture("restored")) qFatal("Unload did not restore original pixels/corners");
    if (!effect("loadEffect", id).toBool()) qFatal("Reload failed");
    QTest::qWait(500); if (on != capture("reloaded")) qFatal("Reload changed pixels");
    pane->showFullScreen(); QTest::qWait(500);
    const auto fullscreen = capture("fullscreen-enabled");
    effect("unloadEffect", id); QTest::qWait(500);
    if (fullscreen != capture("fullscreen-disabled")) qFatal("Fullscreen was altered");
    pane->showNormal(); QTest::qWait(400);
    QGuiApplication::setDesktopFileName("org.sequoia.UnrelatedTest");
    auto other = qobject_cast<QQuickWindow *>(paneComponent.create());
    pane->hide(); QTest::qWait(500);
    const auto unrelated = capture("unrelated-disabled");
    if (!effect("loadEffect", id).toBool()) qFatal("Load failed");
    QTest::qWait(500);
    if (unrelated != capture("unrelated-enabled")) qFatal("Unrelated application was altered");
    if (qEnvironmentVariableIsSet("SEQUOIA_TEST_ALL_EFFECTS")) {
        for (const QString &name : {QString("sequoia_liquid_glass_v6"), QString("sequoia_modal_dimming")})
            if (!effect("loadEffect", name).toBool()) qFatal("Coexisting effect failed: %s",qPrintable(name));
        other->hide(); pane->show(); QTest::qWait(500); capture("all-effects");
        for (const QString &name : {id, QString("sequoia_liquid_glass_v6"), QString("sequoia_modal_dimming")})
            if (!effect("isEffectLoaded", name).toBool()) qFatal("Effect disappeared");
        puts("PASS: all three effects coexist in KWin 6.8.3");
    }
    printf("PASS: %ld blurred pixels, opaque content unchanged, exact unload/reload, fullscreen and unrelated-app exclusion\n",changed);
    delete other; delete pane; delete backdrop;
    return 0;
}
