// SPDX-License-Identifier: GPL-2.0-or-later
#include "sequoiaelevenfilepicker.h"
#include "../../../../desktop/glass-material/glassrim.h"

#include <KColorUtils>
#include <QDialog>
#include <QLayout>
#include <QPainter>
#include <QPointer>
#include <QTimer>
#include <QWidget>

namespace SequoiaEleven::FilePicker
{
static bool isChooser(const QWidget *widget)
{
    if (!qobject_cast<const QDialog *>(widget)) return false;
    if (widget->inherits("QFileDialog") || widget->inherits("KDEPlatformFileDialog")
        || widget->inherits("KDirSelectDialog")) return true;
    // xdg-desktop-portal-kde uses FileDialog, containing a KFileWidget.
    // Requiring both avoids capturing unrelated dialogs with a generic name.
    if (widget->inherits("FileDialog")) {
        for (const auto child : widget->findChildren<QWidget *>())
            if (child->inherits("KFileWidget")) return true;
    }
    return false;
}

bool contains(const QWidget *widget)
{
    for (auto ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
        if (qobject_cast<const QDialog *>(ancestor)) return isChooser(ancestor);
        // Stop at an ordinary app window, even when it has a chooser parent.
        if (ancestor->inherits("QMainWindow")) return false;
    }
    return false;
}

void polish(QWidget *widget)
{
    if (!contains(widget)) return;
    auto dialog = widget;
    while (dialog && !qobject_cast<QDialog *>(dialog)) dialog = dialog->parentWidget();
    if (!dialog || dialog->property("_sequoia_picker_pending").toBool()) return;
    // KDE can polish a dialog before finishing its layout, then overwrite its
    // margins. Schedule once after construction; children also discover it.
    dialog->setProperty("_sequoia_picker_pending", true);
    const QPointer<QStyle> style = widget->style();
    QTimer::singleShot(0, dialog, [dialog, style] {
        dialog->setProperty("_sequoia_picker_pending", QVariant());
        auto layout = dialog->layout();
        if (!style || dialog->style() != style || !layout
            || layout->property("_sequoia_picker_margins").isValid()) return;
        layout->setProperty("_sequoia_picker_margins", QVariant::fromValue(layout->contentsMargins()));
        layout->setProperty("_sequoia_picker_spacing", layout->spacing());
        // Shared library Tokens.panelPadding / Tokens.gap.
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(8);
    });
}

void unpolish(QWidget *widget)
{
    auto layout = widget->layout();
    if (!layout || !layout->property("_sequoia_picker_margins").isValid()) return;
    layout->setContentsMargins(layout->property("_sequoia_picker_margins").value<QMargins>());
    layout->setSpacing(layout->property("_sequoia_picker_spacing").toInt());
    layout->setProperty("_sequoia_picker_margins", QVariant());
    layout->setProperty("_sequoia_picker_spacing", QVariant());
}

void paintPill(QPainter *painter, const QStyleOption *option, QPalette::ColorRole role, bool solid)
{
    const auto &palette = option->palette;
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hover = enabled && (option->state & QStyle::State_MouseOver);
    const bool down = enabled && (option->state & (QStyle::State_Sunken | QStyle::State_On));
    const bool focus = enabled && (option->state & QStyle::State_HasFocus);
#ifdef SEQUOIA_GLASS_TRIAL
    const bool glass = !solid;
#else
    Q_UNUSED(solid);
    const bool glass = false;
#endif
    // Native counterparts of the library's ActionPill and CircleButton. Opaque
    // surfaces in Solid/Transparent, shared optical rim in Liquid Glass.
    QColor fill = KColorUtils::mix(palette.color(role), palette.color(QPalette::Highlight), down ? .20 : hover ? .10 : 0);
    fill.setAlphaF(glass ? (down ? .64 : hover ? .54 : .44) : 1.0);
    QColor border = palette.color(focus ? QPalette::Highlight : QPalette::WindowText);
    if (!focus) border.setAlphaF(enabled ? .12 : .06);
    const QRectF frame = QRectF(option->rect).adjusted(1, 1, -1, -1);
    const qreal radius = qMin(frame.width(), frame.height()) / 2;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(border, focus ? 2 : 1));
    painter->setBrush(fill);
    painter->drawRoundedRect(frame, radius, radius);
    if (glass)
        SequoiaGlass::paintRim(painter, frame, radius, SequoiaGlass::Settings::instance()->intensity(), option->state & QStyle::State_Active);
    painter->restore();
}
}
