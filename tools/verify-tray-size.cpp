// SPDX-License-Identifier: MIT
// Run only in the disposable session created by verify-widget-archives.py.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlContext>
#include <QQmlExpression>
#include <QTest>
#include <QTimer>
#include <QFile>
#include <cstdlib>

static QList<QQuickItem *> items(QQuickItem *root)
{
    QList<QQuickItem *> result{root};
    for (qsizetype i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}

static void require(bool passed, const char *message)
{
    if (!passed) { qCritical("FAIL: %s", message); std::_Exit(1); }
}

static QVariant evaluate(QQuickItem *item, const QString &code)
{
    QQmlExpression expression(qmlContext(item), item, code);
    const auto value = expression.evaluate();
    if (expression.hasError()) qCritical() << expression.error();
    require(!expression.hasError(), "QML expression");
    return value;
}

static QQuickItem *find(const QString &name)
{
    for (auto window : QGuiApplication::allWindows())
        if (auto quick = qobject_cast<QQuickWindow *>(window))
            for (auto item : items(quick->contentItem()))
                if (item->objectName() == name && item->isVisible()) return item;
    return nullptr;
}

static void clickButton(QQuickWindow *window, const QString &text)
{
    for (auto item : items(window->contentItem())) {
        if (item->isVisible() && item->isEnabled()
            && item->property("text").toString().remove('&') == text) {
            require(QMetaObject::invokeMethod(item, "clicked"), "settings button activation");
            QTest::qWait(500);
            return;
        }
    }
    qCritical() << "Missing settings button" << text;
    std::_Exit(1);
}

static void startup()
{
    if (!qEnvironmentVariableIsSet("SEQUOIA_VERIFY_TRAY_SIZE")) return;
    qApp->setQuitOnLastWindowClosed(false);
    QTimer::singleShot(1500, qApp, [] {
        for (auto window : QGuiApplication::allWindows())
            if (auto quick = qobject_cast<QQuickWindow *>(window))
                for (auto item : items(quick->contentItem()))
                    if (auto state = item->property("systemTrayState").value<QObject *>()) {
                        state->setProperty("expanded", true);
                        return;
                    }
        require(false, "tray state");
    });
    QTimer::singleShot(3500, qApp, [] {
        auto popup = find("sequoiaControlCenter");
        require(popup, "Control Center popup");
        auto size = [popup] { return QSize(qRound(popup->width()), qRound(popup->height())); };
        require(size() == QSize(297, 451), "unchanged default dashboard size");
        auto open = [popup](const QString &name) {
            require(QMetaObject::invokeMethod(popup, "openApplet", Q_ARG(QVariant, name)), "open detail menu");
            QTest::qWait(500);
            require(!popup->property("compactDashboard").toBool(), "detail menu active");
        };
        auto home = [popup] {
            QMetaObject::invokeMethod(popup, "goHome");
            QTest::qWait(500);
        };
        open("org.sequoia.volume");
        const auto audioSize = size();
        open("org.sequoia.networkmanagement");
        const auto networkSize = size();
        home();

        auto settings = [popup] {
            evaluate(popup, "Plasmoid.internalAction('configure').trigger()");
            QTest::qWait(1100);
            auto page = find("sequoiaSizeSettings");
            require(page, "native Size settings page");
            return page;
        };
        auto page = settings();
        require(page->property("cfg_controlCenterWidth").toInt() == 297
            && page->property("cfg_controlCenterHeight").toInt() == 451, "schema defaults loaded in settings");
        auto edit = [page](const QString &name, int value) {
            auto input = page->findChild<QObject *>(name);
            require(input, "size input");
            input->setProperty("value", value);
            require(QMetaObject::invokeMethod(input, "valueModified"), "size edit signal");
        };
        edit("controlCenterWidthInput", 420);
        edit("controlCenterHeightInput", 580);
        clickButton(page->window(), "Apply");
        require(evaluate(popup, "Plasmoid.configuration.controlCenterWidth").toInt() == 420
            && evaluate(popup, "Plasmoid.configuration.controlCenterHeight").toInt() == 580, "Apply saves both dimensions");
        page->window()->close();
        evaluate(popup, "systemTrayState.expanded = true");
        QTest::qWait(500);
        require(size() == QSize(420, 580), "configured dashboard dimensions");
        require(popup->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_VERIFY_TRAY_SIZE") + ".popup.png"), "popup capture");
        open("org.sequoia.volume");
        qInfo() << "Sound baseline / after resize:" << audioSize << size();
        require(size() == audioSize, "Sound keeps its original size");
        open("org.sequoia.networkmanagement");
        qInfo() << "Network baseline / after resize:" << networkSize << size();
        require(size() == networkSize, "Network keeps its original size");
        home();
        require(size() == QSize(420, 580), "Back restores configured dashboard size");
        evaluate(popup, "systemTrayState.expanded = false");
        QTest::qWait(150);
        evaluate(popup, "systemTrayState.expanded = true");
        QTest::qWait(500);
        require(size() == QSize(420, 580), "reopening restores configured size");

        page = settings();
        require(page->property("cfg_controlCenterWidth").toInt() == 420
            && page->property("cfg_controlCenterHeight").toInt() == 580, "reopened settings retain saved dimensions");
        QFile saved(qEnvironmentVariable("XDG_CONFIG_HOME") + "/plasmawindowedrc");
        require(saved.open(QFile::ReadOnly), "saved settings file");
        const auto config = saved.readAll();
        require(config.contains("controlCenterWidth=420") && config.contains("controlCenterHeight=580"), "dimensions persisted to disk");
        require(page->window()->grabWindow().save(qEnvironmentVariable("SEQUOIA_VERIFY_TRAY_SIZE") + ".settings.png"), "settings capture");
        page->setProperty("cfg_controlCenterWidth", 1600);
        page->setProperty("cfg_controlCenterHeight", 1600);
        clickButton(page->window(), "Apply");
        require(evaluate(popup, "dashboardWidth <= Screen.width - 60 && dashboardHeight <= Screen.height - 60").toBool(), "large dimensions fit screen");
        clickButton(page->window(), "Reset to Default Size");
        require(page->property("cfg_controlCenterWidth").toInt() == 297
            && page->property("cfg_controlCenterHeight").toInt() == 451, "Defaults restores original size");
        clickButton(page->window(), "Apply");
        page->window()->close();
        evaluate(popup, "systemTrayState.expanded = true");
        QTest::qWait(500);
        require(size() == QSize(297, 451), "default size applied");
        qInfo("PASS: tray size settings, Apply, persistence, reopen, screen bounds, Defaults and independent detail sizes");
        std::_Exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
