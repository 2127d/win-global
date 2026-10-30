// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QColor>
#include <QPalette>

namespace Gwenview::Sequoia {
// Match WeatherSurface, LocationSurface, SettingsCard and WeatherAction in
// apps/kweather. Keep the hierarchy shared by every Gwenview presentation.
inline constexpr qreal SurfaceRadius = 24;
inline constexpr qreal CardRadius = 22;
inline constexpr qreal ControlRadius = 12;
inline constexpr qreal RowRadius = 13;
inline constexpr int ShortDuration = 100;
inline constexpr int LongDuration = 200;
inline QColor alpha(QColor color, qreal opacity) { color.setAlphaF(opacity); return color; }
inline QColor border(const QPalette &palette) { return alpha(palette.color(QPalette::WindowText), 0.13); }
inline QColor tint(QColor base, const QColor &ink, qreal amount)
{
    return QColor::fromRgbF(base.redF() * (1 - amount) + ink.redF() * amount,
                           base.greenF() * (1 - amount) + ink.greenF() * amount,
                           base.blueF() * (1 - amount) + ink.blueF() * amount, base.alphaF());
}
}
