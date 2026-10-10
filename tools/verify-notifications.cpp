// SPDX-License-Identifier: MIT
// Runs only in the private session created by verify-notifications.py.
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlExpression>
#include <QQmlContext>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTimer>
#include <QTest>
#include <QSignalSpy>
#include <cstdlib>

namespace NotificationManager {
class Server { public: static Server *self(); bool init(); };
}
static void fail(const QString &message) {
    qCritical().noquote() << "FAIL:" << message;
    std::_Exit(2);
}
static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (int i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}
static QVariant evaluate(QObject *object, const QString &source) {
    QQmlExpression expression(qmlContext(object), object, source);
    auto result = expression.evaluate();
    if (expression.hasError()) fail(expression.error().toString());
    return result;
}
static QList<QQuickWindow *> popups() {
    QList<QQuickWindow *> result;
    for (auto window : QGuiApplication::allWindows())
        if (auto view = qobject_cast<QQuickWindow *>(window);
            view && view->inherits("NotificationWindow") && view->isVisible()) {
            if (view->objectName() != "sequoiaNotificationPopup") fail("Unstyled duplicate notification banner");
            result << view;
        }
    return result;
}
static void send(const QString &title, const QString &body, const QStringList &actions = {}, int timeout = 0, uchar urgency = 1) {
    auto message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");
    message.setArguments({QString("Calendar"), uint(0), QString("calendar"), title, body, actions,
        QVariantMap{{"urgency", urgency}, {"suppress-sound", true}, {"desktop-entry", QString("org.kde.merkuro.calendar")}}, timeout});
    auto reply = QDBusConnection::sessionBus().call(message, QDBus::BlockWithGui);
    if (reply.type() == QDBusMessage::ErrorMessage) fail(reply.errorMessage());
    QTest::qWait(550);
}
static QQuickWindow *onePopup() {
    auto windows = popups();
    if (windows.size() != 1) fail(QString("Expected one native banner, found %1").arg(windows.size()));
    return windows.first();
}
static QQuickItem *button(QQuickWindow *view, const QString &text) {
    for (auto item : items(view->contentItem()))
        if (item->isVisible() && item->inherits("QQuickAbstractButton") && item->property("text").toString() == text) return item;
    fail("Missing button: " + text);
    return nullptr;
}
static void click(QQuickWindow *view, QQuickItem *item) {
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
        item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
    QTest::qWait(350);
}
static void capture(QQuickWindow *view, const QString &name) {
    for (auto item : items(view->contentItem())) {
        if (!item->isVisible() || !item->inherits("QQuickAbstractButton")) continue;
        const auto bounds = item->mapRectToScene(item->boundingRect());
        if (!QRectF(0, 0, view->width(), view->height()).adjusted(-1, -1, 1, 1).contains(bounds)) fail("Button outside notification bounds");
    }
    if (!view->grabWindow().save(qEnvironmentVariable("SEQUOIA_NOTIFICATION_CAPTURE") + '-' + name + ".png")) fail("Capture failed");
}
static void start() {
    if (qEnvironmentVariableIsEmpty("SEQUOIA_NOTIFICATION_CAPTURE")) return;
    QTimer::singleShot(0, qApp, [] { if (!NotificationManager::Server::self()->init()) fail("Private notification service init"); });
    QTimer::singleShot(1600, qApp, [] {
        QQuickItem *root = nullptr;
        for (auto window : QGuiApplication::allWindows()) if (auto view = qobject_cast<QQuickWindow *>(window))
            for (auto item : items(view->contentItem())) if (item->metaObject()->indexOfProperty("inhibitedOrBroken") >= 0) root = item;
        if (!root) fail("Notification applet did not load");
        if (qEnvironmentVariableIsSet("SEQUOIA_NOTIFICATION_RTL")) QApplication::setLayoutDirection(Qt::RightToLeft);
        evaluate(root, "NotificationState.Globals.notificationSettings.inhibitNotificationsWhenFullscreen = false; NotificationState.Globals.revokeInhibitions()");
        send("Design review in 10 minutes", "Your desktop polish review starts at 3:00 PM.<br>Room 2 · Bring your latest notes.", {"open", "Open Calendar", "later", "Remind Me"});
        auto view = onePopup();
        capture(view, "actions");
        QPointer<QQuickWindow> old(view);
        click(view, button(view, "Open Calendar"));
        if (old && old->isVisible()) fail("Action did not dismiss banner");

        send("A message from Alex", "Looks good. Can you send the updated mockup?", {"inline-reply", "Reply", "later", "Later"});
        view = onePopup();
        click(view, button(view, "Reply"));
        QQuickItem *field = nullptr;
        for (auto item : items(view->contentItem())) if (item->isVisible() && item->inherits("QQuickTextField")) field = item;
        if (!field) fail("Inline reply field missing");
        field->setProperty("text", "I'll send it shortly.");
        capture(view, "reply");
        old = view;
        click(view, button(view, "Send"));
        if (old && old->isVisible()) fail("Reply did not dismiss banner");

        send("Storage almost full", "Less than 1 GB of free space remains. Free up space to keep saving your work.", {}, 0, 2);
        view = onePopup();
        capture(view, "critical");
        click(view, button(view, "Close"));
        if (!popups().isEmpty()) fail("Close did not dismiss banner");

        send("Long notification", QString("A long message stays readable and scrollable. ").repeated(35));
        view = onePopup();
        if (view->height() > 600) fail("Long body escaped height cap");
        capture(view, "long");
        click(view, button(view, "Close"));

        // Exercise the native job view using fixture progress, without copying
        // files or creating a real cancellable job on the user's desktop.
        send("Copying 24 files", "Preparing your project backup.");
        view = onePopup();
        evaluate(view, R"(
            modelInterface.jobDetails = Qt.createQmlObject('import QtQml; QtObject { property double totalBytes: 100000000; property double processedBytes: 64000000; property int processedFiles: 15; property int totalFiles: 24; property int processedDirectories: 0; property int totalDirectories: 1; property int speed: 2000000; property int elapsedTime: 32000; property url effectiveDestUrl: ""; property bool hasDetails: false; }', modelInterface);
            modelInterface.percentage = 64;
            modelInterface.jobState = NotificationManager.Notifications.JobStateRunning;
            modelInterface.suspendable = true;
            modelInterface.killable = true;
            modelInterface.notificationType = NotificationManager.Notifications.JobType;
        )");
        QTest::qWait(350);
        button(view, "Pause");
        button(view, "Cancel");
        capture(view, "progress");
        click(view, button(view, "Close"));

        send("Short update", "This notification expires automatically.", {}, 1200);
        view = onePopup();
        QTest::mouseMove(view, QPoint(-20, -20));
        QTest::qWait(1600);
        if (!popups().isEmpty()) fail("Notification timeout did not expire");

        evaluate(root, "NotificationState.Globals.notificationSettings.notificationsInhibitedUntil = new Date(Date.now() + 60000); NotificationState.Globals.notificationSettings.save(); NotificationState.Globals.checkInhibition()");
        QTest::qWait(250);
        if (!evaluate(root, "NotificationState.Globals.inhibited && NotificationManager.Server.inhibited").toBool()) fail("DND not enabled");
        send("Quiet notification", "Stored while Do Not Disturb is enabled.");
        if (!popups().isEmpty()) fail("DND displayed a normal banner");
        if (evaluate(root, "historyModel.count").toInt() < 1) fail("DND notification not retained in history");
        evaluate(root, "expanded = true");
        root->window()->resize(440, 560);
        QTest::qWait(400);
        root->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_NOTIFICATION_CAPTURE") + "-history.png");
        qWarning("PASS: native notification delivery, single banner, action, inline reply, close, critical/long text bounds, progress layout, timeout, and DND/history");
        std::_Exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
