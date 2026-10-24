// SPDX-License-Identifier: GPL-2.0-or-later
#include "modalbridge.h"
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusUnixFileDescriptor>
#include <QTemporaryFile>
#include <QImage>
#include <QTest>
#include <QDir>
#include <cstdio>
#include <qpa/qplatformwindow_p.h>
#include <wayland-client-core.h>
#include <QProcess>

QImage rawCapture(bool active)
{
    QTemporaryFile file;
    if (!file.open()) qFatal("cannot open capture file");
    auto message = QDBusMessage::createMethodCall("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2", "org.kde.KWin.ScreenShot2", active ? "CaptureActiveWindow" : "CaptureArea");
    if (!active) message << 0 << 0 << uint(1280) << uint(800);
    message << QVariantMap{{"hide-caller-windows", false}, {"include-decoration", true}, {"include-shadow", false}} << QVariant::fromValue(QDBusUnixFileDescriptor(file.handle()));
    const auto reply = QDBusConnection::sessionBus().call(message, QDBus::Block, 5000);
    if (reply.type() == QDBusMessage::ErrorMessage) qFatal("capture: %s", qPrintable(reply.errorMessage()));
    auto map = qdbus_cast<QVariantMap>(reply.arguments().first());
    const int bytes = map["height"].toInt() * map["stride"].toInt();
    for (int i=0; file.size() < bytes && i<100; ++i) QTest::qWait(10);
    file.seek(0);
    auto data = file.readAll();
    if (data.size() != bytes) qFatal("incomplete capture");
    QImage image(reinterpret_cast<const uchar *>(data.constData()), map["width"].toInt(), map["height"].toInt(), map["stride"].toInt(), QImage::Format(map["format"].toInt()));
    image = image.copy();
    return image;
}
QImage capture(const QString &name)
{
    static QRect bounds;
    static int header = 0;
    if (!header) header = rawCapture(true).height() - 360;
    auto screen = rawCapture(false);
    if (name == "before" || name == "other-window" || name == "after-destroy") {
        int top = 0;
        while (top < screen.height() && screen.pixelColor(640,top).red() < 253) ++top;
        int left = 0;
        while (left < screen.width() && screen.pixelColor(left,top+10).red() < 253) ++left;
        bounds = QRect(left,top-header,500,360+header);
    }
    auto image=screen.copy(bounds);
    image.save(qEnvironmentVariable("MODAL_CAPTURE") + '/' + name + ".png");
    return image;
}
void effect(const QString &method)
{
    auto message=QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.kde.kwin.Effects", method);
    message << "sequoia_modal_dimming";
    auto reply=QDBusConnection::sessionBus().call(message);
    if(reply.type()==QDBusMessage::ErrorMessage) qFatal("effect: %s",qPrintable(reply.errorMessage()));
    QTest::qWait(250);
}
int main(int argc, char **argv)
{
    setbuf(stdout, nullptr);
    QGuiApplication app(argc,argv);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import QtQuick.Window
Window {
    width: 500; height: 360; visible: true; title: "Modal dimming test"
    property real sequoiaModalAmount: 0
    color: "white"
    Rectangle { anchors.fill: parent; color: "black"; opacity: sequoiaModalAmount }
    Rectangle { anchors.centerIn: parent; width: 200; height: 120; radius: 20; color: "white"; border.color: "gray" }
})", QUrl());
    auto first=qobject_cast<QQuickWindow *>(component.create());
    auto second=qobject_cast<QQuickWindow *>(component.create());
    if (!first || !second) qFatal("QML: %s", qPrintable(component.errorString()));
    second->hide();
    new SequoiaModalBridge(first);
    new SequoiaModalBridge(second);
    QTest::qWait(600);
    first->requestActivate(); QTest::qWait(300);
    auto before=capture("before");
    first->setProperty("sequoiaModalAmount", .3); QTest::qWait(400);
    auto dim=capture("embedded");
    const int x=before.width()/4, y=10;
    printf("decoration before=%d after=%d client=%d popup=%d\n", before.pixelColor(x,y).red(),dim.pixelColor(x,y).red(),dim.pixelColor(50,70).red(),dim.pixelColor(dim.width()/2,dim.height()/2).red());
    if (dim.pixelColor(x,y).red() > before.pixelColor(x,y).red() - 30) qFatal("decoration did not dim");
    if (dim.pixelColor(dim.width()/2,dim.height()/2).red() < 245) qFatal("popup was dimmed");
    second->show(); second->requestActivate(); QTest::qWait(300);
    auto other=capture("other-window");
    if (other.pixelColor(x,y).red() < before.pixelColor(x,y).red()*.85) qFatal("another window was dimmed");
    second->hide(); first->requestActivate(); QTest::qWait(250);
    // First window geometry remains unchanged. Re-establish the crop with its
    // temporarily undimmed client; all assertions still use compositor pixels.
    first->setProperty("sequoiaModalAmount",0); QTest::qWait(150); capture("before");
    first->setProperty("sequoiaModalAmount",.3); QTest::qWait(150);
    effect("unloadEffect");
    auto unloaded=capture("unloaded");
    if (unloaded.pixelColor(x,y).red() < before.pixelColor(x,y).red()-5) qFatal("unload left dimming behind");
    effect("loadEffect"); QTest::qWait(250);
    auto reloaded=capture("reloaded");
    if (reloaded.pixelColor(x,y).red() > before.pixelColor(x,y).red()-30) qFatal("bridge did not resubmit after reload");
    first->setProperty("sequoiaModalAmount", 0); QTest::qWait(300);
    auto restored=capture("restored");
    if (restored.pixelColor(x,y).red() < before.pixelColor(x,y).red()-5) qFatal("closing did not restore title bar");
    auto surface = first->nativeInterface<QNativeInterface::Private::QWaylandWindow>()->surface();
    const uint surfaceId = wl_proxy_get_id(reinterpret_cast<wl_proxy *>(surface));
    auto report = QDBusMessage::createMethodCall("org.sequoia.ModalDimming", "/org/sequoia/ModalDimming",
                                                 "org.sequoia.ModalDimming1", "SetDecorationDim");
    report << surfaceId << .3;
    auto owner = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "modal-owner-test");
    auto accepted = owner.call(report);
    if (accepted.arguments().isEmpty() || !accepted.arguments().first().toBool()) qFatal("valid report rejected");
    QTest::qWait(200);
    auto owned = capture("owned");
    if (owned.pixelColor(x,y).red() > before.pixelColor(x,y).red()-30) qFatal("owner report did not dim");
    QDBusConnection::disconnectFromBus("modal-owner-test");
    owner = QDBusConnection("disconnected");
    QTest::qWait(250);
    auto disconnected = capture("owner-disconnected");
    if (disconnected.pixelColor(x,y).red() < before.pixelColor(x,y).red()-5) qFatal("owner disconnect left stale dimming");
    QProcess foreign;
    foreign.start("qdbus-qt6", {"org.sequoia.ModalDimming", "/org/sequoia/ModalDimming", "org.sequoia.ModalDimming1.SetDecorationDim", QString::number(surfaceId), "0.3"});
    while (!foreign.waitForFinished(10)) QCoreApplication::processEvents();
    if (foreign.readAllStandardOutput().trimmed() != "false") qFatal("foreign process report was not rejected");
    second->setTransientParent(first);
    second->setModality(Qt::WindowModal);
    second->resize(300,200);
    second->show(); QTest::qWait(500);
    auto native=capture("native-modal");
    printf("native decoration=%d client=%d\n",native.pixelColor(x,y).red(),native.pixelColor(5,100).red());
    if (native.pixelColor(x,y).red() > before.pixelColor(x,y).red()-30) qFatal("native modal parent did not dim");
    if (native.pixelColor(5,100).red() < 170) qFatal("native modal was dimmed twice");
    second->hide(); QTest::qWait(400);
    auto nativeClosed=capture("native-closed");
    if (nativeClosed.pixelColor(x,y).red() < before.pixelColor(x,y).red()-5) qFatal("native modal parent remained dim");
    second->setTransientParent(nullptr); second->setModality(Qt::NonModal); second->resize(500,360);
    first->setProperty("sequoiaModalAmount",.3); QTest::qWait(100);
    delete first;
    second->show(); second->requestActivate(); QTest::qWait(250);
    capture("after-destroy");
    delete second;
    puts("PASS: decoration, popup, separate windows, native modal, owner disconnect, foreign-process rejection, close, destroy, unload and reload");
}
