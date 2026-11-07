// SPDX-License-Identifier: MIT
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>
#include <QImage>
#include <KWindowEffects>
#include <cstdio>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Glass Weather Test"));
    app.setDesktopFileName(QStringLiteral("org.sequoia.GlassWeatherTest"));
    QQmlEngine engine;
    QObject::connect(&engine, &QQmlEngine::quit, &app, &QGuiApplication::quit);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/glass-weather/Weather.qml")));
    auto window = qobject_cast<QQuickWindow *>(component.create());
    if (!window) {
        qCritical().noquote() << component.errorString();
        return 1;
    }
    QPainterPath shape;
    shape.addRoundedRect(QRectF(0, 0, window->width(), window->height()), 40, 40);
    const QRegion mask(shape.toFillPolygon().toPolygon());
    window->setMask(mask);
    // Requests the existing compositor material for this window only.
    // This executable never changes the active effect, palette or theme.
    KWindowEffects::enableBlurBehind(window, true, mask);
    if (auto screen = window->screen()) {
        const auto area = screen->availableGeometry();
        window->setPosition(area.center() - QPoint(window->width()/2, window->height()/2));
    }
    window->show();
    window->requestActivate();
    if (app.arguments().contains(QStringLiteral("--check"))) {
        QTimer::singleShot(1000, &app, [&] {
            const auto image = window->grabWindow();
            const auto alpha = image.pixelColor(image.width()/2, image.height()*3/4).alpha();
            const bool valid = !image.isNull() && alpha > 0 && alpha < 100
                && image.pixelColor(0, 0).alpha() == 0
                && window->width() == 240 && window->height() == 328;
            const auto path = qEnvironmentVariable("GLASS_WEATHER_CAPTURE");
            if (!path.isEmpty()) image.save(path);
            fprintf(stderr, "Weather test: 240x328, clear corners, body alpha %d/255: %s\n", alpha, valid ? "PASS" : "FAIL");
            app.exit(valid ? 0 : 1);
        });
    }
    return app.exec();
}
