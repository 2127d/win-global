// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "glassrim.h"
#include <QPainter>
#include <QPainterPath>

namespace SequoiaGlass {

// The native counterpart of the component library's ActionPill/Panel material.
// QStyle owns painting only: never replace a control, its input, or its layout.
struct ControlSurface {
    QColor fill;
    QColor foreground;
    QColor accent;
    qreal radius = 0;
    bool glass = false;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool active = true;
    // Saturated faces use HighlightedText in the existing native label painter.
    // Keep these faces opaque; a translucent accent could erase text contrast.
    bool opaque = false;
    qreal glassOpacity = .44;
};

inline void paintControl(QPainter *painter, const QRectF &rect, const ControlSurface &surface)
{
    if (!painter || rect.isEmpty() || !surface.fill.isValid()) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    const qreal radius = qBound(0., surface.radius, qMin(rect.width(), rect.height()) / 2);
    QColor fill = surface.fill;
    if (surface.glass && !surface.opaque) {
        const qreal opacity = surface.enabled
            ? (surface.pressed ? .64 : surface.hovered ? .54 : surface.glassOpacity) : .30;
        fill.setAlphaF(opacity);
    }
    // SourceOver reveals the containing surface, never clears its content or
    // captures a desktop/backdrop. Compositor glass remains the window's job.
    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawRoundedRect(rect, radius, radius);

    QColor border = surface.focused ? surface.accent : surface.foreground;
    if (!surface.focused) border.setAlphaF(surface.enabled ? (surface.glass ? .16 : .12) : .06);
    const qreal penWidth = surface.focused ? 2 : 1;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(border, penWidth));
    const qreal inset = penWidth / 2;
    painter->drawRoundedRect(rect.adjusted(inset, inset, -inset, -inset),
                            qMax(0., radius - inset), qMax(0., radius - inset));
    if (surface.glass) {
        paintRim(painter, rect, radius, Settings::instance()->intensity() * (surface.enabled ? 1 : .45), surface.active);
    }
    painter->restore();
}
}
