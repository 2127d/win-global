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
#include <QAbstractItemModel>
#include <QQmlListReference>
#include <cstdio>

static QQuickItem *find(QQuickItem *item, const QString &name = QStringLiteral("sequoiaTaskStrip"))
{
    if (!item) return nullptr;
    if (item->objectName() == name) return item;
    for (auto child : item->childItems()) if (auto result = find(child, name)) return result;
    return nullptr;
}

static void collect(QQuickItem *item, QJsonArray &buttons, int &icons, const QJsonObject &command, int &executed)
{
    if (item->objectName() == QLatin1String("taskIcon") && item->isVisible() && item->property("valid").toBool()) ++icons;
    if (item->objectName() == QLatin1String("integratedTaskButton")) {
        QJsonObject button;
        button["hasBackground"] = item->property("background").value<QObject *>() != nullptr;
        for (const char *key : {"text", "isRunning", "isActiveTask", "isGroup", "isMinimized", "width", "height", "hovered"})
            button[QString::fromLatin1(key)] = QJsonValue::fromVariant(item->property(key));
        if (auto icon = item->findChild<QObject *>(QStringLiteral("taskIcon"))) {
            for (const char *key : {"scale", "motionOffset", "motionDuration", "glowStrength", "glowRendered"})
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
    for (auto child : item->childItems()) collect(child, buttons, icons, command, executed);
}

static bool visibleApplet(QQuickItem *item, const QString &pluginName)
{
    if (auto applet = item->property("_plasma_applet").value<QObject *>())
        if (applet->property("pluginName").toString() == pluginName
            && item->isVisible() && item->width() > 10 && item->height() > 10) return true;
    for (auto child : item->childItems()) if (visibleApplet(child, pluginName)) return true;
    return false;
}

static QQuickItem *withProperty(QQuickItem *item, const char *name)
{
    if (item->property(name).isValid()) return item;
    for (auto child : item->childItems()) if (auto result = withProperty(child, name)) return result;
    return nullptr;
}

static void appletGeometry(QQuickItem *item, QJsonArray &applets)
{
    if (auto applet = item->property("_plasma_applet").value<QObject *>()) {
        const auto pos = item->mapToScene(QPointF());
        QJsonObject geometry{{"plugin", applet->property("pluginName").toString()},
            {"x", pos.x()}, {"y", pos.y()}, {"width", item->width()}, {"height", item->height()},
            {"inLauncherSlot", item->parentItem() && item->parentItem()->objectName() == "launcherSlot"}};
        if (auto icon = find(item, QStringLiteral("startMenuLauncherIcon"))) {
            geometry["iconWidth"] = icon->width();
            geometry["iconHeight"] = icon->height();
            geometry["glowStrength"] = icon->property("glowStrength").toDouble();
            geometry["glowRendered"] = icon->property("glowRendered").toBool();
            if (auto button = find(item, QStringLiteral("startMenuButton")))
                geometry["hasBackground"] = button->property("background").value<QObject *>() != nullptr;
        }
        applets.append(geometry);
    }
    for (auto child : item->childItems()) appletGeometry(child, applets);
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
            if (quick && quick->isVisible()) {
                if (auto picker = withProperty(quick->contentItem(), "currentPlugin")) {
                    state["alternativesVisible"] = true;
                    if (auto list = withProperty(picker, "model")) {
                        auto model = qobject_cast<QAbstractItemModel *>(list->property("model").value<QObject *>());
                        if (model) {
                            const auto roles = model->roleNames();
                            const int pluginRole = roles.key("pluginName", -1);
                            QJsonArray plugins;
                            for (int i = 0; i < model->rowCount(); ++i)
                                plugins.append(model->data(model->index(i, 0), pluginRole).toString());
                            state["alternatives"] = plugins;
                            const QString selected = command.value("selectAlternative").toString();
                            if (!selected.isEmpty() && plugins.contains(selected) && !executed) {
                                picker->setProperty("currentPlugin", selected);
                                executed += QMetaObject::invokeMethod(picker, "savePluginAndClose");
                            }
                        }
                    }
                    if (command.contains("cancelAlternatives") && !executed) {
                        QTest::keyClick(quick, Qt::Key_Escape);
                        ++executed;
                    }
                    // Closing a picker can destroy its window during event
                    // dispatch. Never inspect that window again afterward.
                    continue;
                }
            }
            auto strip = quick ? find(quick->contentItem()) : nullptr;
            if (!strip || !quick->isVisible() || !strip->isVisible()) continue;
            auto panel = strip->parentItem();
            if (command.contains("hoverLauncher") && !executed) {
                if (auto button = find(panel, QStringLiteral("startMenuButton"))) {
                    QTest::mouseMove(quick, button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
                    ++executed;
                }
            }
            if (command.contains("addLauncher") && !executed) {
                auto applet = panel->property("_plasma_applet").value<QObject *>();
                QQmlListReference actions(applet, "contextualActions");
                for (qsizetype i = 0; i < actions.count(); ++i) {
                    auto action = actions.at(i);
                    if (action->objectName() == "addLauncherAction")
                        executed += QMetaObject::invokeMethod(action, "trigger");
                }
            }
            QJsonArray applets;
            appletGeometry(quick->contentItem(), applets);
            state["applets"] = applets;
            const auto stripPos = strip->mapToScene(QPointF());
            state["taskX"] = stripPos.x();
            state["taskY"] = stripPos.y();
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
            state["clockVisible"] = visibleApplet(quick->contentItem(), QStringLiteral("org.kde.plasma.digitalclock"));
            state["launcherWidgetVisible"] = visibleApplet(quick->contentItem(), QStringLiteral("org.kde.plasma.kickoff"));
            state["builtInMenu"] = strip->findChild<QObject *>(QStringLiteral("integratedApplicationMenu")) != nullptr
                || strip->findChild<QObject *>(QStringLiteral("integratedApplicationLauncher")) != nullptr;
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
