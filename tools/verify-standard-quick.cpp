// SPDX-License-Identifier: GPL-2.0-or-later
// Compile using Qt6Widgets, Qt6QuickControls2, Qt6Quick, Qt6Qml, Qt6Test.
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyle>
#include <QTest>
#include <cstdio>

static void require(bool ok, const char *message) { if (!ok) qFatal("%s", message); }
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addOptions({{"output", "Output directory", "path"}, {"qml", "Probe source", "path"},
        {"glass", "Glass style"}, {"dark", "Dark palette"}, {"baseline", "Old plugin"}, {"show", "Keep preview open"}});
    parser.process(app);
    auto style = app.style();
    const bool dark = parser.isSet("dark"), glass = parser.isSet("glass");
    require(style->objectName().compare(glass ? "sequoiaelevenglass" : "sequoiaeleven", Qt::CaseInsensitive) == 0, "Wrong QStyle loaded");
    if (!parser.isSet("baseline")) require(style->property("sequoiaSharedControls").toBool(), "New shared painter missing");
    else QQuickStyle::setStyle("org.kde.desktop");
    require(QQuickStyle::name() == "org.kde.desktop", "Generic Quick bridge was not selected without a makeover");
    auto palette = app.palette();
    palette.setColor(QPalette::Window, dark ? QColor("#252629") : QColor("#f2f3f5"));
    palette.setColor(QPalette::Base, dark ? QColor("#242528") : QColor("#ffffff"));
    palette.setColor(QPalette::Button, dark ? QColor("#303135") : QColor("#ffffff"));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        palette.setColor(role, dark ? QColor("#f2f3f5") : QColor("#24262b"));
    palette.setColor(QPalette::Highlight, QColor("#74a7e7"));
    palette.setColor(QPalette::HighlightedText, QColor("#101820"));
    app.setPalette(palette);
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(parser.value("qml")));
    require(!engine.rootObjects().isEmpty(), "Ordinary Quick controls failed to load");
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    require(window, "Probe did not create a window");
    auto item = [window](const char *name) {
        auto result = window->findChild<QQuickItem *>(QString::fromLatin1(name));
        require(result, name); return result;
    };
    QTest::qWait(150);
    item("action")->forceActiveFocus(); QTest::keyClick(window, Qt::Key_Space);
    require(window->property("activations").toInt() == 1, "Button keyboard activation failed");
    auto disabled = item("disabled");
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, disabled->mapToScene(disabled->boundingRect().center()).toPoint());
    require(window->property("activations").toInt() == 1, "Disabled control activated");
    item("field")->forceActiveFocus(); QTest::keyClick(window, Qt::Key_A);
    require(item("field")->property("text").toString() == "a", "Text editing failed");
    item("slider")->forceActiveFocus(); auto value = item("slider")->property("value").toDouble();
    QTest::keyClick(window, Qt::Key_Right);
    require(item("slider")->property("value").toDouble() > value, "Slider keyboard input failed");
    item("check")->forceActiveFocus(); QTest::keyClick(window, Qt::Key_Space);
    require(item("check")->property("checked").toBool(), "Checkbox keyboard input failed");
    // Prove these are KDE's unmodified native StyleItems, not component-library controls.
    require(item("action")->property("background").value<QObject *>()->property("elementType").toString() == "button", "Button no longer uses the standard QStyle bridge");
    QTest::qWait(100);
    const auto stem = QString("quick-%1-%2").arg(dark ? "dark" : "light", glass ? "glass" : "solid");
    QDir output(parser.value("output")); output.mkpath(".");
    require(window->grabWindow().save(output.filePath(stem + ".png")), "Screenshot failed");
    QJsonObject report{{"style", style->objectName()}, {"quick_style", QQuickStyle::name()},
        {"keyboard", true}, {"text_input", true}, {"slider", true}, {"checkbox", true}, {"disabled", true},
        {"standard_style_item", true}, {"app_specific_overlays", false}, {"platform", app.platformName()}};
    QFile result(output.filePath(stem + ".json")); require(result.open(QIODevice::WriteOnly), "Report failed");
    result.write(QJsonDocument(report).toJson()); result.close();
    std::puts(("PASS " + QJsonDocument(report).toJson(QJsonDocument::Compact)).constData()); std::fflush(stdout);
    if (parser.isSet("show")) return app.exec();
    window->close();
    return 0;
}
