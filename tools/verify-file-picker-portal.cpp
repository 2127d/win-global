// SPDX-License-Identifier: MIT
// Loaded only into an isolated xdg-desktop-portal-kde test process.
#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDialog>
#include <QLayout>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    std::_Exit(1);
}

static void start()
{
    if (qEnvironmentVariableIsEmpty("SEQUOIA_PICKER_PORTAL_CAPTURE")) return;
    QTimer::singleShot(300, qApp, [] {
        auto message = QDBusMessage::createMethodCall("org.freedesktop.impl.portal.desktop.kde",
            "/org/freedesktop/portal/desktop", "org.freedesktop.impl.portal.FileChooser", "OpenFile");
        const QVariantMap options{{"modal", false}, {"current_folder", qgetenv("SEQUOIA_PICKER_FIXTURE") + '\0'}};
        message.setArguments({QVariant::fromValue(QDBusObjectPath("/org/freedesktop/portal/desktop/request/sequoia/test")),
            QString("org.sequoia.Test"), QString(), QString("Open File"), options});
        // A separate connection exercises the delayed reply over the bus;
        // Qt's local-loop optimization cannot handle a delayed self-call.
        const auto client = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "picker-test-client");
        auto watcher = new QDBusPendingCallWatcher(client.asyncCall(message), qApp);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, qApp, [](QDBusPendingCallWatcher *result) {
            const auto reply = result->reply();
            if (reply.type() == QDBusMessage::ErrorMessage) {
                fprintf(stderr, "%s\n", qPrintable(reply.errorMessage()));
                fail("Portal request failed");
            }
            if (reply.arguments().isEmpty() || reply.arguments()[0].toUInt() != 1) fail("Portal did not return Cancel");
            fprintf(stderr, "PASS: actual KDE portal chooser, spacing, pill actions and cancellation\n");
            std::_Exit(0);
        });
        QTimer::singleShot(2500, qApp, [] {
            for (auto widget : QApplication::topLevelWidgets()) {
                auto dialog = qobject_cast<QDialog *>(widget);
                if (!dialog || !dialog->isVisible() || !dialog->inherits("FileDialog")) continue;
                if (!dialog->layout() || dialog->layout()->contentsMargins() != QMargins(12, 12, 12, 12))
                    fail("Portal did not receive chooser padding");
                dialog->resize(1000, 680);
                QTest::qWait(100);
                if (!dialog->grab().save(qEnvironmentVariable("SEQUOIA_PICKER_PORTAL_CAPTURE"))) fail("Portal capture failed");
                for (auto button : dialog->findChildren<QPushButton *>()) {
                    if (button->isVisible() && button->text().remove('&') == "Cancel") {
                        if (button->height() < 42) fail("Portal did not receive pill action sizing");
                        QTest::mouseClick(button, Qt::LeftButton);
                        return;
                    }
                }
                fail("Portal Cancel button missing");
            }
            fail("Native portal chooser missing");
        });
    });
    QTimer::singleShot(10000, qApp, [] { fail("Portal verification timed out"); });
}
Q_COREAPP_STARTUP_FUNCTION(start)
