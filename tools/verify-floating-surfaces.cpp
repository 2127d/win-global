// SPDX-License-Identifier: MIT
#include "../apps/gwenview/gwenview-26.08.1/app/floatingsurface.h"
#include "../apps/gwenview/gwenview-26.08.1/app/smoothviewercorners.h"
#include <QApplication>
#include <QWidget>
#include <QDir>
#include <QElapsedTimer>
#include <set>
#include <cstdio>
#include <cstdlib>
using namespace Gwenview;
static void require(bool value, const char *message) { if (!value) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); } }
class ImageProbe : public QWidget {
public: using QWidget::QWidget;
protected: void paintEvent(QPaintEvent *) override {
    QPainter paint(this);
    paint.fillRect(rect(), QColor("#5577aa"));
    for (int y = 0; y < height(); y += 8) for (int x = 0; x < width(); x += 8)
        paint.fillRect(QRect(x, y, 4, 4), QColor("#eec233"));
}
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const QString output = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("/tmp/gwenview-surfaces");
    QDir().mkpath(output);
    for (bool dark : {false, true}) for (qreal dpr : {1.0, 1.25, 1.5, 2.0}) {
        QPalette palette;
        palette.setColor(QPalette::Window, dark ? QColor("#292a2c") : QColor("#f7f7f8"));
        const QRectF pill(20, 20, 180, 44);
        QImage surface(QSize(qCeil(220 * dpr), qCeil(84 * dpr)), QImage::Format_ARGB32_Premultiplied);
        surface.setDevicePixelRatio(dpr); surface.fill(Qt::transparent);
        { QPainter painter(&surface); paintPillSurface(painter, pill, 22, palette); }
        std::set<int> coverage;
        for (int y = qCeil(20 * dpr); y < qFloor(64 * dpr); ++y)
            for (int x = qFloor(19 * dpr); x < qCeil(42 * dpr); ++x) {
                const int alpha = qAlpha(surface.pixel(x, y));
                if (alpha > 0 && alpha < 255) coverage.insert(alpha);
            }
        require(coverage.size() > 12, "rounded ends retain a smooth range of edge coverage");
        require(surface.pixelColor(QPoint(20 * dpr, 20 * dpr)).alpha() == 0, "pill corners stay transparent");
        require(surface.pixelColor(QPoint(100 * dpr, 42 * dpr)).alpha() == 255, "solid surface stays opaque");
        const auto reference = surface.copy();
        surface.fill(Qt::transparent);
        { QPainter painter(&surface); paintPillSurface(painter, pill, 22, palette); }
        require(surface == reference, "cached rendering matches first paint exactly");
        QImage shadow(surface.size(), surface.format()); shadow.setDevicePixelRatio(dpr); shadow.fill(Qt::transparent);
        { QPainter painter(&shadow); paintFloatingShadow(painter, pill, 22, palette, true); }
        require(shadow.pixelColor(QPoint(100 * dpr, 42 * dpr)).alpha() == 0, "shadow never muddies glass interior");
        require(shadow.pixelColor(QPoint(100 * dpr, 67 * dpr)).alpha() > 0, "soft shadow remains visible outside capsule");
        { QPainter painter(&shadow); painter.drawImage(QPoint(), surface); }
        require(shadow.save(output + QStringLiteral("/pill-%1-%2.png").arg(dark ? "dark" : "light").arg(dpr)), "save scaled pill samples");
        // The shared rendering cache must eliminate repeated high-resolution paints.
        int renders = 0;
        QPainter painter(&surface);
        const QString key = QStringLiteral("cache-probe/%1/%2").arg(dark).arg(dpr);
        QElapsedTimer timer; timer.start();
        for (int i = 0; i < 100; ++i)
            paintSmoothSurface(painter, pill, key, [&](QPainter &p) { ++renders; p.fillRect(pill, Qt::red); });
        require(renders == 1, "stable surfaces rasterize once across repeated frames");
        fprintf(stderr, "INFO: %s %.2fx: %zu edge levels; 100 cached paints %lldms\n", dark ? "dark" : "light", dpr, coverage.size(), timer.elapsed());
    }
    QWidget canvas; canvas.setAttribute(Qt::WA_TranslucentBackground); canvas.resize(150, 120);
    ImageProbe viewer(&canvas); viewer.setGeometry(10, 10, 130, 100); viewer.setAutoFillBackground(true);
    canvas.show(); app.processEvents();
    const QImage original = canvas.grab().toImage();
    auto effect = new SmoothViewerCorners(&viewer); viewer.setGraphicsEffect(effect); app.processEvents();
    const QImage rounded = canvas.grab().toImage();
    const qreal scale = canvas.devicePixelRatioF();
    std::set<int> alpha;
    for (int y = qCeil(10 * scale); y < qFloor(27 * scale); ++y)
        for (int x = qCeil(10 * scale); x < qFloor(27 * scale); ++x) {
            const int a = rounded.pixelColor(x, y).alpha();
            if (a > 0 && a < 255) alpha.insert(a);
        }
    require(alpha.size() > 10, "viewer corners use graded alpha, not a binary mask");
    require(rounded.pixelColor(QPoint(10 * scale, 10 * scale)).alpha() == 0, "viewer corners expose the real parent background");
    const QRect interior(QPoint(30 * scale, 30 * scale), QPoint(120 * scale, 90 * scale));
    require(original.copy(interior) == rounded.copy(interior), "viewer interior pixels remain unchanged");
    effect->setEnabled(false); app.processEvents();
    require(canvas.grab().toImage() == original, "fullscreen bypass restores exact original pixels");
    require(rounded.save(output + QStringLiteral("/viewer-corners-%1.png").arg(scale)), "capture rounded viewer corner coverage");
    fprintf(stderr, "PASS: viewer corner alpha, unchanged interior and fullscreen bypass at %.2fx\n", scale);
    fprintf(stderr, "PASS: smooth light/dark edges at 100/125/150/200%%, clear shadows and cached painting\n");
}
