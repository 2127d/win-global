// SPDX-License-Identifier: MIT
// Real input through KWin's EIS backend, restricted to our private compositor.
#include <libei.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <unistd.h>

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
                && qEnvironmentVariable("XDG_RUNTIME_DIR").contains("/.staging/gwenview-test-")
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
