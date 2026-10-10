// SPDX-License-Identifier: MIT
// Cached wallpaper blur, clipped per control. Works with software and GPU Qt Quick.
#include <QCoreApplication>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QPainterPath>
#include <QImageReader>
#include <QCache>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QQmlEngine>
#include <QQmlContext>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace {
// Three separable box passes approximate a Gaussian. Sliding sums keep the cost
// independent of radius. Edge pixels are extended, avoiding dark screen edges.
QImage boxPass(const QImage &input, int radius, bool horizontal)
{
    QImage output(input.size(), QImage::Format_ARGB32_Premultiplied);
    const int length = horizontal ? input.width() : input.height();
    const int lines = horizontal ? input.height() : input.width();
    const int divisor = 2 * radius + 1;
    for (int line = 0; line < lines; ++line) {
        auto pixel = [&](int position) {
            position = std::clamp(position, 0, length - 1);
            return horizontal ? reinterpret_cast<const QRgb *>(input.constScanLine(line))[position]
                              : reinterpret_cast<const QRgb *>(input.constScanLine(position))[line];
        };
        int red = 0, green = 0, blue = 0, alpha = 0;
        auto add = [&](QRgb value, int sign) {
            red += sign * qRed(value); green += sign * qGreen(value);
            blue += sign * qBlue(value); alpha += sign * qAlpha(value);
        };
        for (int index = -radius; index <= radius; ++index) add(pixel(index), 1);
        for (int position = 0; position < length; ++position) {
            const QRgb value = qRgba(red / divisor, green / divisor, blue / divisor, alpha / divisor);
            if (horizontal) reinterpret_cast<QRgb *>(output.scanLine(line))[position] = value;
            else reinterpret_cast<QRgb *>(output.scanLine(position))[line] = value;
            add(pixel(position - radius), -1);
            add(pixel(position + radius + 1), 1);
        }
    }
    return output;
}

QImage blurredWallpaper(const QUrl &url, const QSizeF &scene, qreal radius)
{
    if (!url.isLocalFile() || scene.isEmpty()) return {};
    // Bound memory/work at 4K/HiDPI. All controls at a given screen size share it.
    const qreal scale = std::min(qreal(0.25), 960.0 / std::max(scene.width(), scene.height()));
    const QSize size(std::max(1, qRound(scene.width() * scale)), std::max(1, qRound(scene.height() * scale)));
    const QString sourceKey = url.toString() + QString::number(QFileInfo(url.toLocalFile()).lastModified().toMSecsSinceEpoch());
    const QString key = sourceKey + QStringLiteral("|%1x%2|%3")
        .arg(scene.width()).arg(scene.height()).arg(radius);
    // QQuickPaintedItem can render on the scene graph thread. Protect shared cache.
    static QMutex mutex;
    static QCache<QString, QImage> cache(4);
    static QCache<QString, QImage> decoded(2);
    QMutexLocker lock(&mutex);
    if (auto cached = cache.object(key)) return *cached;
    QImage image;
    if (auto cached = decoded.object(sourceKey)) image = *cached;
    else {
        QImageReader reader(url.toLocalFile());
        reader.setAutoTransform(true);
        // Only the blurred layer needs this copy; the sharp Qt Quick Image keeps
        // full resolution. Reuse it during resize instead of decoding every frame.
        const QSize original = reader.size();
        if (original.width() > 1280 || original.height() > 1280)
            reader.setScaledSize(original.scaled(QSize(1280, 1280), Qt::KeepAspectRatio));
        image = reader.read();
        if (!image.isNull()) decoded.insert(sourceKey, new QImage(image));
    }
    if (image.isNull()) return {};
    QImage backdrop(size, QImage::Format_ARGB32_Premultiplied);
    backdrop.fill(Qt::transparent);
    {
        QPainter painter(&backdrop);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        // Same centered PreserveAspectCrop mapping as the sharp wallpaper Image.
        const qreal factor = std::max(scene.width() / image.width(), scene.height() / image.height());
        const QSizeF crop(scene.width() / factor, scene.height() / factor);
        painter.drawImage(QRectF(QPointF(), size), image,
            QRectF((image.width() - crop.width()) / 2, (image.height() - crop.height()) / 2, crop.width(), crop.height()));
    }
    const int pixels = std::max(1, qRound(radius * scale));
    for (int pass = 0; pass < 3; ++pass) {
        backdrop = boxPass(backdrop, pixels, true);
        backdrop = boxPass(backdrop, pixels, false);
    }
    cache.insert(key, new QImage(backdrop));
    return backdrop;
}
}

class WallpaperGlass : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY appearanceChanged)
    Q_PROPERTY(QSizeF sceneSize READ sceneSize WRITE setSceneSize NOTIFY appearanceChanged)
    Q_PROPERTY(QRectF sampleRect READ sampleRect WRITE setSampleRect NOTIFY appearanceChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY appearanceChanged)
    Q_PROPERTY(qreal blurRadius READ blurRadius WRITE setBlurRadius NOTIFY appearanceChanged)
    Q_PROPERTY(QColor tint READ tint WRITE setTint NOTIFY appearanceChanged)
public:
    explicit WallpaperGlass(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent) { setAntialiasing(true); }
    QUrl source() const { return m_source; }
    QSizeF sceneSize() const { return m_scene; }
    QRectF sampleRect() const { return m_sample; }
    qreal radius() const { return m_radius; }
    qreal blurRadius() const { return m_blur; }
    QColor tint() const { return m_tint; }
    void setSource(const QUrl &value) {
        // Custom QUrl properties receive relative URLs unchanged, unlike Image.
        // Resolve them against the component before handing them to QImageReader.
        QUrl resolved = value;
        if (!value.isEmpty() && value.isRelative()) {
            if (auto context = qmlContext(this)) resolved = context->resolvedUrl(value);
        }
        if (resolved != m_source) { m_source = resolved; changed(); }
    }
    void setSceneSize(const QSizeF &value) { if (value != m_scene) { m_scene = value; changed(); } }
    void setSampleRect(const QRectF &value) { if (value != m_sample) { m_sample = value; changed(); } }
    void setRadius(qreal value) { if (value != m_radius) { m_radius = value; changed(); } }
    void setBlurRadius(qreal value) { value = std::clamp(value, 1.0, 64.0); if (value != m_blur) { m_blur = value; changed(); } }
    void setTint(const QColor &value) { if (value != m_tint) { m_tint = value; changed(); } }
    void paint(QPainter *painter) override {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath clip;
        clip.addRoundedRect(boundingRect(), m_radius, m_radius);
        painter->setClipPath(clip);
        const QImage backdrop = blurredWallpaper(m_source, m_scene, m_blur);
        if (!backdrop.isNull()) {
            const qreal sx = backdrop.width() / m_scene.width(), sy = backdrop.height() / m_scene.height();
            painter->drawImage(boundingRect(), backdrop,
                QRectF(m_sample.x() * sx, m_sample.y() * sy, m_sample.width() * sx, m_sample.height() * sy));
        }
        painter->fillRect(boundingRect(), m_tint);
    }
signals:
    void appearanceChanged();
private:
    void changed() { emit appearanceChanged(); update(); }
    QUrl m_source;
    QSizeF m_scene;
    QRectF m_sample;
    qreal m_radius = 20, m_blur = 24;
    QColor m_tint = QColor("#60404548");
};

static void registerGlass() { qmlRegisterType<WallpaperGlass>("org.sequoia.login", 1, 0, "WallpaperGlass"); }
Q_COREAPP_STARTUP_FUNCTION(registerGlass)
#include "wallpaper-glass.moc"
