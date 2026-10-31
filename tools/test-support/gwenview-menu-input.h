// SPDX-License-Identifier: MIT
// Real input through KWin's EIS backend, restricted to our private compositor.
#include <libei.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <unistd.h>
#include <QTemporaryFile>
#include <QDBusArgument>
#include <QDBusVirtualObject>
#include <QDBusPendingCall>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

struct GwenviewWindowProbe : QDBusVirtualObject {
    QJsonArray windows;
    bool received = false;
    QString introspect(const QString &) const override { return {}; }
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override {
        windows = QJsonDocument::fromJson(message.arguments().first().toString().toUtf8()).array();
        received = true;
        connection.send(message.createReply());
        return true;
    }
};

static QJsonArray gwenviewNativeWindows(const QString &setup = {})
{
    require(qEnvironmentVariable("WAYLAND_DISPLAY") == "gwenview-menu-test", "window probe restricted to private compositor");
    auto bus = QDBusConnection::sessionBus();
    GwenviewWindowProbe probe;
    require(bus.registerVirtualObject("/GwenviewWindowProbe", &probe), "register private geometry probe");
    QTemporaryFile script; require(script.open(), "private geometry script");
    script.write(setup.toUtf8());
    script.write(QString("callDBus('%1', '/GwenviewWindowProbe', 'org.gwenview.Test', 'windows', JSON.stringify(workspace.windowList().map(w => ({title:w.caption, x:w.bufferGeometry.x, y:w.bufferGeometry.y, width:w.bufferGeometry.width, height:w.bufferGeometry.height}))));").arg(bus.baseService()).toUtf8());
    script.flush();
    auto load = QDBusMessage::createMethodCall("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "loadScript");
    load << script.fileName() << QStringLiteral("gwenview-native-geometry");
    const auto reply = bus.call(load);
    require(reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty(), "load private geometry script");
    auto run = QDBusMessage::createMethodCall("org.kde.KWin", "/Scripting/Script" + QString::number(reply.arguments().first().toInt()), "org.kde.kwin.Script", "run");
    bus.asyncCall(run);
    for (int i=0; i<50 && !probe.received; ++i) QTest::qWait(20);
    auto unload = QDBusMessage::createMethodCall("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "unloadScript");
    unload << QStringLiteral("gwenview-native-geometry"); bus.call(unload);
    bus.unregisterObject("/GwenviewWindowProbe");
    require(probe.received, "receive compositor geometry");
    return probe.windows;
}

static QRect gwenviewNativeRect(QWidget *window)
{
    const auto windows = gwenviewNativeWindows();
    for (const auto &entry : windows) {
        const auto data = entry.toObject();
        if (data["title"].toString().contains(QString(window->windowTitle()).remove("[*]").trimmed()))
            return QRect(qRound(data["x"].toDouble()), qRound(data["y"].toDouble()), qRound(data["width"].toDouble()), qRound(data["height"].toDouble()));
    }
    require(false, "find actual compositor window");
    return {};
}

struct GwenviewMenuInput {
    ei *context = nullptr;
    ei_device *pointer = nullptr;
    ei_device *buttons = nullptr;
    int cookie = 0;
    QDBusMessage call(const QString &method) {
        return QDBusMessage::createMethodCall("org.kde.KWin", "/org/kde/KWin/EIS/RemoteDesktop", "org.kde.KWin.EIS.RemoteDesktop", method);
    }
    GwenviewMenuInput() {
        require(qEnvironmentVariable("WAYLAND_DISPLAY") == "gwenview-menu-test"
                && qEnvironmentVariable("XDG_RUNTIME_DIR").startsWith(QDir::tempPath() + "/gwenview-test-")
                && qEnvironmentVariableIsSet("SEQUOIA_GWENVIEW_NATIVE_INPUT"), "input restricted to private test compositor");
        auto request = call("connectToEIS");
        request << 2;
        const auto reply = QDBusConnection::sessionBus().call(request);
        require(reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() == 2, "private KWin EIS connection");
        auto fd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments()[0]);
        cookie = reply.arguments()[1].toInt();
        context = ei_new_sender(nullptr);
        ei_configure_name(context, "Gwenview private menu test");
        require(ei_setup_backend_fd(context, dup(fd.fileDescriptor())) == 0, "EIS client setup");
        for (int attempt = 0; attempt < 100 && (!pointer || !buttons); ++attempt) {
            dispatch();
            QTest::qWait(20);
        }
        require(pointer && buttons, "virtual pointer and buttons ready");
    }
    void dispatch() {
        ei_dispatch(context);
        while (auto event = ei_get_event(context)) {
            if (ei_event_get_type(event) == EI_EVENT_SEAT_ADDED) {
                ei_seat_bind_capabilities(ei_event_get_seat(event), EI_DEVICE_CAP_POINTER, EI_DEVICE_CAP_POINTER_ABSOLUTE, EI_DEVICE_CAP_BUTTON, nullptr);
            } else if (ei_event_get_type(event) == EI_EVENT_DEVICE_RESUMED) {
                auto device = ei_event_get_device(event);
                ei_device_start_emulating(device, 0);
                if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE)) pointer = ei_device_ref(device);
                if (ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON)) buttons = ei_device_ref(device);
            }
            ei_event_unref(event);
        }
    }
    void move(QPoint point, qreal scale) {
        ei_device_pointer_motion_absolute(pointer, point.x()*scale, point.y()*scale);
        ei_device_frame(pointer, ei_now(context));
        dispatch();
    }
    void button(bool down) {
        ei_device_button_button(buttons, 0x110, down);
        ei_device_frame(buttons, ei_now(context));
        dispatch();
    }
    ~GwenviewMenuInput() {
        ei_device_unref(pointer);
        ei_device_unref(buttons);
        ei_unref(context);
        auto request = call("disconnect");
        request << cookie;
        QDBusConnection::sessionBus().call(request);
    }
};

static QImage gwenviewCompositorCapture()
{
    QTemporaryFile file; require(file.open(), "native screen capture storage");
    auto request = QDBusMessage::createMethodCall("org.kde.KWin.ScreenShot2", "/org/kde/KWin/ScreenShot2", "org.kde.KWin.ScreenShot2", "CaptureWorkspace");
    request << QVariantMap{{"native-resolution", false}, {"hide-caller-windows", false}} << QVariant::fromValue(QDBusUnixFileDescriptor(file.handle()));
    auto reply = QDBusConnection::sessionBus().call(request, QDBus::Block, 5000);
    require(reply.type() == QDBusMessage::ReplyMessage, "capture actual compositor pixels");
    const auto info = qdbus_cast<QVariantMap>(reply.arguments().first());
    const int bytes = info["height"].toInt() * info["stride"].toInt();
    for(int i=0;i<100 && file.size()<bytes;++i) QTest::qWait(10);
    file.seek(0);const auto data=file.readAll(); require(data.size()==bytes,"complete native capture");
    return QImage(reinterpret_cast<const uchar *>(data.constData()),info["width"].toInt(),info["height"].toInt(),info["stride"].toInt(),QImage::Format(info["format"].toInt())).copy();
}
