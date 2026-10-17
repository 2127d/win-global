// SPDX-License-Identifier: MIT
// Loaded only into the private Plasma test process.
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QTest>
#include <cstdio>

static QQuickItem *find(QQuickItem *item)
{
    if (item->objectName() == QLatin1String("sequoiaTaskStrip")) return item;
    for (auto child : item->childItems()) if (auto result = find(child)) return result;
    return nullptr;
}

static void collect(QQuickItem *item, QJsonArray &buttons, int &icons, const QJsonObject &command, int &executed)
{
    if (item->objectName() == QLatin1String("taskIcon") && item->isVisible() && item->property("valid").toBool()) ++icons;
    if (item->objectName() == QLatin1String("integratedTaskButton")) {
        QJsonObject button;
        for (const char *key : {"text", "isRunning", "isActiveTask", "isGroup", "isMinimized", "width", "height", "hovered"})
            button[QString::fromLatin1(key)] = QJsonValue::fromVariant(item->property(key));
        if (auto icon = item->findChild<QObject *>(QStringLiteral("taskIcon"))) {
            for (const char *key : {"scale", "motionOffset", "motionDuration"})
                button[QString::fromLatin1(key)] = QJsonValue::fromVariant(icon->property(key));
        }
        buttons.append(button);
        if (executed == 0 && command.value("hoverTask").toString() == item->property("text").toString()) {
            QTest::mouseMove(item->window(), item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
            ++executed;
        }
        if (executed == 0 && command.value("mouseClickTask").toString() == item->property("text").toString()
            && item->property("isRunning").toBool()) {
            QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier,
                item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
            ++executed;
        }
        if (executed == 0 && command.value("clickTask").toString() == item->property("text").toString()
            && (!command.contains("running") || command.value("running").toBool() == item->property("isRunning").toBool()))
        {
            executed += QMetaObject::invokeMethod(item, "clicked");
        }
    }
    if (executed == 0 && item->objectName() == QLatin1String("integratedApplicationLauncher") && command.value("menu").toBool())
        executed += QMetaObject::invokeMethod(item, "clicked");
    for (auto child : item->childItems()) collect(child, buttons, icons, command, executed);
}

static bool visibleClock(QQuickItem *item)
{
    if (auto applet = item->property("_plasma_applet").value<QObject *>())
        if (applet->property("pluginName").toString() == QLatin1String("org.kde.plasma.digitalclock")
            && item->isVisible() && item->width() > 10 && item->height() > 10) return true;
    for (auto child : item->childItems()) if (visibleClock(child)) return true;
    return false;
}

static void start()
{
    if (qEnvironmentVariableIsEmpty("SEQUOIA_TASKBAR_PROBE")) return;
    auto timer = new QTimer(qApp);
    QObject::connect(timer, &QTimer::timeout, [] {
        const QString directory = qEnvironmentVariable("SEQUOIA_TASKBAR_PROBE");
        QJsonObject command;
        QFile commandFile(directory + "/command.json");
        if (commandFile.open(QIODevice::ReadOnly)) {
            command = QJsonDocument::fromJson(commandFile.readAll()).object();
            commandFile.close();
        }
        QJsonObject state;
        int executed = 0;
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            auto strip = quick ? find(quick->contentItem()) : nullptr;
            if (!strip || !quick->isVisible() || !strip->isVisible()) continue;
            for (const char *key : {"effectiveIconSize", "cellLength", "taskCount", "width", "height", "contentLength", "taskLength", "horizontal", "naturalLength"})
                state[QString::fromLatin1(key)] = QJsonValue::fromVariant(strip->property(key));
            state["windowWidth"] = quick->width();
            state["windowHeight"] = quick->height();
            state["lengthMode"] = quick->property("lengthMode").toInt();
            int icons = 0;
            QJsonArray buttons;
            collect(strip, buttons, icons, command, executed);
            state["validIcons"] = icons;
            state["buttons"] = buttons;
            state["clockVisible"] = visibleClock(quick->contentItem());
            if (auto menu = strip->findChild<QObject *>(QStringLiteral("integratedApplicationMenu"))) {
                state["menuVisible"] = menu->property("visible").toBool();
                state["applicationCount"] = menu->property("applicationCount").toInt();
            }
            if (QFile::exists(directory + "/capture")) {
                quick->grabWindow().save(directory + "/panel.png");
                QFile::remove(directory + "/capture");
            }
        }
        // A startup task can be replaced during a frame. Keep the command until
        // its actual delegate exists; never consume a click without dispatching it.
        if (executed) {
            fprintf(stderr, "TEST dispatched %s\n", QJsonDocument(command).toJson(QJsonDocument::Compact).constData());
            commandFile.remove();
        }
        QSaveFile file(directory + "/state.json");
        if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(state).toJson()); file.commit(); }
    });
    timer->start(200);
}
Q_COREAPP_STARTUP_FUNCTION(start)
