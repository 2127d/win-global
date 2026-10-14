// SPDX-License-Identifier: MIT
// Run under application basenames to exercise the actual native style plugin.
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QStyleFactory>
#include <QStyle>
#include <QQuickStyle>
#include <cstdio>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    // A second style construction must not duplicate import paths.
    delete QStyleFactory::create(QStringLiteral("SequoiaEleven"));
    QQmlEngine engine;
    // The generic platform has no KDE control-style integration. Set its stock
    // style explicitly for fallback probes after observing startup's selection.
    const auto selectedStyle = QQuickStyle::name();
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    QQmlComponent page(&engine);
    page.setData("import QtQuick\nimport org.kde.kirigami as Kirigami\nKirigami.Page {}", QUrl());
    auto item = page.create();
    QJsonObject result{{"qml", qEnvironmentVariable("QML_IMPORT_PATH")},
                      {"qml2", qEnvironmentVariable("QML2_IMPORT_PATH")},
                      {"style", qEnvironmentVariable("QT_QUICK_CONTROLS_STYLE")},
                      {"selectedStyle", selectedStyle},
                      {"executable", QCoreApplication::applicationFilePath()}};
    if (item) {
        auto background = item->property("background").value<QQuickItem *>();
        result["background"] = background ? background->objectName() : "missing";
    } else {
        QString errors;
        for (const auto &error : page.errors()) errors += error.toString() + '\n';
        result["error"] = errors;
    }
    puts(QJsonDocument(result).toJson(QJsonDocument::Compact).constData());
    delete item;
    return result.contains("error") ? 1 : 0;
}
