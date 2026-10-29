// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QImage>
namespace Gwenview {
// Three separable box passes approximate a Gaussian. The capture is sampled at
// quarter resolution, so this stays inexpensive even during image animation.
inline QImage blurBackdrop(QImage image)
{
    constexpr int radius = 2;
    constexpr int samples = 2 * radius + 1;
    for (int pass = 0; pass < 3; ++pass) {
        for (bool horizontal : {true, false}) {
            QImage result(image.size(), image.format());
            for (int y = 0; y < image.height(); ++y) {
                auto out = reinterpret_cast<QRgb *>(result.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    int red = 0, green = 0, blue = 0;
                    for (int offset = -radius; offset <= radius; ++offset) {
                        const QRgb pixel = image.pixel(qBound(0, x + (horizontal ? offset : 0), image.width() - 1),
                                                       qBound(0, y + (horizontal ? 0 : offset), image.height() - 1));
                        red += qRed(pixel);
                        green += qGreen(pixel);
                        blue += qBlue(pixel);
                    }
                    out[x] = qRgb(red / samples, green / samples, blue / samples);
                }
            }
            image = result;
        }
    }
    return image;
}

}
