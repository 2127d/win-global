// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QImage>
#include <QPalette>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc != 4) return 2;
    auto palette = app.palette();
    palette.setColor(QPalette::Window, QColor(37,38,41,168));
    palette.setColor(QPalette::WindowText, Qt::white);
    app.setPalette(palette);
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    QQmlApplicationEngine engine;
    engine.addImportPath(QString::fromLocal8Bit(argv[2]));
    bool warnings = false;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app, [&](const auto &) { warnings = true; });
    engine.load(QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1])));
    if (engine.rootObjects().isEmpty()) return 3;
    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 4;
    QTimer::singleShot(500, &app, [&] {
        const auto image = window->grabWindow();
        app.exit(!warnings && !image.isNull() && image.save(QString::fromLocal8Bit(argv[3])) ? 0 : 5);
    });
    return app.exec();
}
