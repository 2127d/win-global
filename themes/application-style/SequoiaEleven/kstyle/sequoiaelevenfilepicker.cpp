// SPDX-License-Identifier: GPL-2.0-or-later
#include "sequoiaelevenfilepicker.h"
#include "sequoiaelevenpickerpreview.h"
#include "../../../../desktop/glass-material/glassrim.h"

#include <KColorUtils>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QDialog>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractItemDelegate>
#include <QComboBox>
#include <QCursor>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QScreen>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

namespace SequoiaEleven::FilePicker
{
QRect breadcrumbHoverRect(const QWidget *widget)
{
    const int height = qMin(32, widget->height() - 8);
    QRect rect(0, 0, widget->width(), height);
    rect.moveCenter(widget->rect().center());
    if (widget->inherits("KDEPrivate::KUrlNavigatorDropDownButton")) {
        rect.setWidth(height);
        rect.moveCenter(widget->rect().center());
    } else if (widget->inherits("KDEPrivate::KUrlNavigatorToggleButton")) {
        rect.setWidth(height);
        rect.moveLeft(widget->layoutDirection() == Qt::RightToLeft ? 8 : widget->width() - height - 8);
    }
    return rect;
}
namespace {
class PickerMenuDelegate : public QStyledItemDelegate
{
public:
    explicit PickerMenuDelegate(QAbstractItemView *view) : QStyledItemDelegate(view), original(view->itemDelegate()) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    { auto size = QStyledItemDelegate::sizeHint(option, index); return QSize(size.width() + 16, qMax(32, size.height() + 8)); }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem item(option); initStyleOption(&item, index);
        painter->save(); painter->setRenderHint(QPainter::Antialiasing);
        if (item.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
            painter->setPen(Qt::NoPen); painter->setBrush(item.palette.color(QPalette::Highlight));
            painter->drawRoundedRect(QRectF(item.rect).adjusted(0, 1, 0, -1), 10, 10);
            item.palette.setColor(QPalette::Text, item.palette.color(QPalette::HighlightedText));
        }
        item.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus);
        item.backgroundBrush = Qt::NoBrush; item.rect.adjust(8, 0, -8, 0);
        const auto style = item.widget ? item.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &item, painter, item.widget);
        painter->restore();
    }
    QPointer<QAbstractItemDelegate> original;
};

class Chrome : public QObject
{
public:
    explicit Chrome(QWidget *dialog) : QObject(dialog) { setObjectName(QStringLiteral("sequoiaPickerChrome")); }
    void decorate(QWidget *widget)
    {
        // Keep KDE's delegate: it also owns section-header and eject hit tests.
        // Its icons/text are centered using the same row rect as our pill.
        if (widget->inherits("KFilePlacesView") && !widget->property("_sequoia_picker_places_palette").isValid()) {
            widget->setProperty("_sequoia_picker_places_palette", widget->palette());
            widget->installEventFilter(this);
            sidebarPalette(widget);
        }
        if (widget->inherits("KDEPrivate::KUrlNavigatorDropDownButton")) {
            if (!widget->property("_sequoia_picker_arrow_widths").isValid()) {
                widget->setProperty("_sequoia_picker_arrow_widths", QSize(widget->minimumWidth(), widget->maximumWidth()));
                widget->setFixedWidth(36);
                widget->installEventFilter(this);
            }
        }
        if (widget->inherits("KDEPrivate::KUrlNavigatorToggleButton") && !widget->property("_sequoia_picker_edit_minimum").isValid()) {
            widget->setProperty("_sequoia_picker_edit_minimum", widget->minimumWidth());
            widget->setMinimumWidth(48);
            widget->installEventFilter(this);
        }
        if (widget->inherits("QComboBoxPrivateContainer")) {
            auto combo = qobject_cast<QComboBox *>(widget->parentWidget());
            if (combo && contains(combo)) {
                if (!widget->property("_sequoia_picker_filter_popup").isValid()) {
                    widget->setProperty("_sequoia_picker_filter_popup", true);
                    if (auto frame = qobject_cast<QFrame *>(widget)) {
                        frame->setProperty("_sequoia_picker_popup_frame", frame->frameStyle());
                        frame->setFrameShape(QFrame::NoFrame);
                    }
                    auto view = combo->view();
                    view->setItemDelegate(new PickerMenuDelegate(view));
                    view->setProperty("_sequoia_picker_popup_frame", view->frameStyle());
                    view->setProperty("_sequoia_picker_popup_fill", view->autoFillBackground());
                    view->viewport()->setProperty("_sequoia_picker_popup_fill", view->viewport()->autoFillBackground());
                    view->setFrameShape(QFrame::NoFrame);
                    view->setAutoFillBackground(false);
                    view->viewport()->setAutoFillBackground(false);
                    if (widget->layout()) {
                        widget->layout()->setProperty("_sequoia_picker_popup_padding", QVariant::fromValue(widget->layout()->contentsMargins()));
                        widget->layout()->setContentsMargins(8, 8, 8, 8);
                    }
                    widget->installEventFilter(this);
                    if (widget->isVisible()) fitPopup(widget);
                }
            }
        }
        if (auto header = qobject_cast<QHeaderView *>(widget); header && header->orientation() == Qt::Horizontal) {
            auto viewport = header->viewport();
            if (!viewport->property("_sequoia_picker_header_mask").isValid()) {
                viewport->setProperty("_sequoia_picker_header_mask", QVariant::fromValue(viewport->mask()));
                viewport->installEventFilter(this);
            }
            roundHeader(viewport);
        }
        if (auto frame = qobject_cast<QFrame *>(widget); frame && frame->frameShape() == QFrame::HLine) {
            // The two native dividers border the file view. Keep other lines.
            for (auto sibling : frame->parentWidget()->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
                const int before = sibling->y() - frame->geometry().bottom();
                const int after = frame->y() - sibling->geometry().bottom();
                if (sibling->inherits("KDirOperator") && ((before >= 0 && before <= 8) || (after >= 0 && after <= 8))) {
                    if (!frame->property("_sequoia_picker_separator").isValid()) {
                        frame->setProperty("_sequoia_picker_separator", !frame->isHidden());
                        frame->installEventFilter(this);
                    }
                    frame->hide();
                }
            }
        }
    }
    void restore(QWidget *dialog)
    {
        for (auto widget : dialog->findChildren<QWidget *>()) {
            widget->removeEventFilter(this);
            if (widget->property("_sequoia_picker_places_palette").isValid()) {
                widget->setPalette(widget->property("_sequoia_picker_places_palette").value<QPalette>());
                widget->setProperty("_sequoia_picker_places_palette", QVariant());
            }
            if (widget->property("_sequoia_picker_edit_minimum").isValid()) {
                widget->setMinimumWidth(widget->property("_sequoia_picker_edit_minimum").toInt());
                widget->setProperty("_sequoia_picker_edit_minimum", QVariant());
            }
            if (widget->property("_sequoia_picker_filter_popup").isValid()) {
                restorePopupSize(widget);
                if (auto frame = qobject_cast<QFrame *>(widget)) {
                    frame->setFrameStyle(frame->property("_sequoia_picker_popup_frame").toInt());
                    frame->setProperty("_sequoia_picker_popup_frame", QVariant());
                }
                auto combo = qobject_cast<QComboBox *>(widget->parentWidget());
                if (combo) {
                    auto view = combo->view();
                    if (auto delegate = dynamic_cast<PickerMenuDelegate *>(view->itemDelegate())) {
                        view->setItemDelegate(delegate->original); delegate->deleteLater();
                    }
                    view->setFrameStyle(view->property("_sequoia_picker_popup_frame").toInt());
                    for (auto area : {static_cast<QWidget *>(view), view->viewport()}) {
                        area->setAutoFillBackground(area->property("_sequoia_picker_popup_fill").toBool());
                        area->setProperty("_sequoia_picker_popup_fill", QVariant());
                    }
                    view->setProperty("_sequoia_picker_popup_frame", QVariant());
                }
                if (widget->layout()) {
                    widget->layout()->setContentsMargins(widget->layout()->property("_sequoia_picker_popup_padding").value<QMargins>());
                    widget->layout()->setProperty("_sequoia_picker_popup_padding", QVariant());
                }
                widget->setProperty("_sequoia_picker_filter_popup", QVariant());
            }
            if (widget->property("_sequoia_picker_arrow_widths").isValid()) {
                const auto widths = widget->property("_sequoia_picker_arrow_widths").toSize();
                widget->setMinimumWidth(widths.width()); widget->setMaximumWidth(widths.height());
                widget->setProperty("_sequoia_picker_arrow_widths", QVariant());
            }
            if (widget->property("_sequoia_picker_header_mask").isValid()) {
                widget->setMask(widget->property("_sequoia_picker_header_mask").value<QRegion>());
                widget->setProperty("_sequoia_picker_header_mask", QVariant());
            }
            if (widget->property("_sequoia_picker_separator").isValid()) {
                widget->setVisible(widget->property("_sequoia_picker_separator").toBool());
                widget->setProperty("_sequoia_picker_separator", QVariant());
            }
        }
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        auto widget = static_cast<QWidget *>(object);
        if (widget->property("_sequoia_picker_places_palette").isValid() && event->type() == QEvent::PaletteChange)
            sidebarPalette(widget);
        if (widget->property("_sequoia_picker_edit_minimum").isValid()
            && (event->type() == QEvent::HoverMove || event->type() == QEvent::Enter || event->type() == QEvent::Leave))
            widget->update();
        if (widget->property("_sequoia_picker_header_mask").isValid() && event->type() == QEvent::Resize)
            roundHeader(widget);
        if (widget->property("_sequoia_picker_separator").isValid() && event->type() == QEvent::Show)
            widget->hide();
        if (widget->property("_sequoia_picker_filter_popup").isValid()) {
            if (event->type() == QEvent::Show) {
                fitPopup(widget);
                QTimer::singleShot(0, widget, [widget] {
                    if (widget->isVisible() && widget->property("_sequoia_picker_filter_popup").toBool()) fitPopup(widget);
                });
            } else if (event->type() == QEvent::Hide) {
                restorePopupSize(widget);
            } else if (event->type() == QEvent::Paint) {
                QPainter painter(widget);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.fillRect(widget->rect(), Qt::transparent);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                QColor fill = widget->palette().color(QPalette::Window);
#ifdef SEQUOIA_GLASS_TRIAL
                fill.setAlphaF(.82);
#else
                fill.setAlpha(255);
#endif
                QColor border = widget->palette().color(QPalette::WindowText); border.setAlphaF(.14);
                painter.setBrush(fill); painter.setPen(QPen(border, 1));
                const QRectF surface = QRectF(widget->rect()).adjusted(.5, .5, -.5, -.5);
                painter.drawRoundedRect(surface, popupRadius(widget), popupRadius(widget));
#ifdef SEQUOIA_GLASS_TRIAL
                SequoiaGlass::paintRim(&painter, surface, popupRadius(widget), SequoiaGlass::Settings::instance()->intensity());
#endif
                return true;
            }
        }
        if ((widget->property("_sequoia_picker_arrow_widths").isValid() || widget->property("_sequoia_picker_edit_minimum").isValid()) && event->type() == QEvent::Paint) {
            QPainter painter(widget);
            QStyleOption option; option.initFrom(widget);
            const auto button = qobject_cast<QAbstractButton *>(widget);
            const bool edit = widget->property("_sequoia_picker_edit_minimum").isValid();
            option.rect = breadcrumbHoverRect(widget);
            const QPointF center = QRectF(option.rect).center();
            if (button && button->isDown()) option.state |= QStyle::State_Sunken;
            const bool hovered = option.rect.contains(widget->mapFromGlobal(QCursor::pos())) && widget->underMouse();
            if (hovered || widget->hasFocus() || (button && button->isDown()))
                paintPill(&painter, &option);
            if (edit) {
                if (widget->underMouse() || widget->hasFocus() || (button && button->isChecked())) {
                    const auto icon = QIcon::fromTheme(button && button->isChecked() ? QStringLiteral("dialog-ok") : QStringLiteral("open-for-editing"));
                    icon.paint(&painter, QRect(qRound(center.x() - 8), qRound(center.y() - 8), 16, 16), Qt::AlignCenter,
                               widget->isEnabled() ? QIcon::Normal : QIcon::Disabled);
                }
            } else {
                // Draw around the exact circle center; native arrow metrics add
                // an asymmetric inset intended for other control shapes.
                const qreal direction = widget->layoutDirection() == Qt::RightToLeft ? -1 : 1;
                QPainterPath arrow;
                arrow.moveTo(center + QPointF(-2 * direction, -4));
                arrow.lineTo(center + QPointF(2 * direction, 0));
                arrow.lineTo(center + QPointF(-2 * direction, 4));
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(QPen(option.palette.color(QPalette::WindowText), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                painter.drawPath(arrow);
            }
            return true;
        }
        return false;
    }
private:
    static void sidebarPalette(QWidget *widget)
    {
        auto palette = widget->palette();
        for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
            palette.setColor(group, QPalette::HighlightedText, palette.color(group, QPalette::Text));
        if (palette != widget->palette()) widget->setPalette(palette);
    }
    static void restorePopupSize(QWidget *popup)
    {
        if (!popup->property("_sequoia_picker_popup_minimum").isValid()) return;
        popup->setMinimumSize(popup->property("_sequoia_picker_popup_minimum").toSize());
        popup->setMaximumSize(popup->property("_sequoia_picker_popup_maximum").toSize());
        popup->setProperty("_sequoia_picker_popup_minimum", QVariant());
        popup->setProperty("_sequoia_picker_popup_maximum", QVariant());
    }
    static void fitPopup(QWidget *popup)
    {
        auto combo = qobject_cast<QComboBox *>(popup->parentWidget());
        if (!combo || !combo->count()) return;
        auto view = combo->view();
        int width = 0, height = 0;
        const int visibleRows = qMin(combo->count(), combo->maxVisibleItems());
        for (int row = 0; row < combo->count(); ++row) {
            const auto index = combo->model()->index(row, combo->modelColumn(), combo->rootModelIndex());
            QStyleOptionViewItem option; option.initFrom(view); option.font = view->font();
            const QSize hint = view->itemDelegate()->sizeHint(option, index);
            width = qMax(width, hint.width());
            if (row < visibleRows) height += qMax(hint.height(), view->sizeHintForRow(row));
        }
        const QPoint origin = combo->mapToGlobal(QPoint(0, combo->height() + 4));
        auto screen = QGuiApplication::screenAt(origin);
        if (!screen) screen = combo->screen();
        const QRect available = screen->availableGeometry().adjusted(8, 8, -8, -8);
        width = qBound(64, width + 32 + (visibleRows < combo->count() ? view->verticalScrollBar()->sizeHint().width() : 0), available.width());
        height = qBound(1, height + 16, available.height());
        if (!popup->property("_sequoia_picker_popup_minimum").isValid()) {
            popup->setProperty("_sequoia_picker_popup_minimum", popup->minimumSize());
            popup->setProperty("_sequoia_picker_popup_maximum", popup->maximumSize());
        }
        popup->setFixedSize(width, height);
        if (popup->layout()) popup->layout()->activate();
        const int overhead = popup->height() - view->viewport()->height();
        height = qMin(available.height(), height - 16 + overhead);
        popup->setFixedSize(width, height);
        int x = combo->layoutDirection() == Qt::RightToLeft ? origin.x() + combo->width() - width : origin.x();
        int y = origin.y();
        if (y + height > available.bottom()) y = combo->mapToGlobal(QPoint()).y() - height - 4;
        popup->move(qBound(available.left(), x, available.right() - width + 1), qBound(available.top(), y, available.bottom() - height + 1));
    }
    static void roundHeader(QWidget *viewport)
    {
        const QRect rect = viewport->rect();
        if (rect.isEmpty()) return;
        const qreal radius = qMin(10, rect.height() / 2);
        QPainterPath path;
        path.moveTo(0, rect.height());
        path.lineTo(0, radius);
        path.quadTo(0, 0, radius, 0);
        path.lineTo(rect.width() - radius, 0);
        path.quadTo(rect.width(), 0, rect.width(), radius);
        path.lineTo(rect.width(), rect.height());
        path.closeSubpath();
        viewport->setMask(QRegion(path.toFillPolygon().toPolygon()));
    }
};
}
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

int popupRadius(const QWidget *widget)
{
    return widget && widget->inherits("QComboBoxPrivateContainer") && contains(widget) ? 16 : 0;
}

void paintNavigation(QPainter *painter, const QStyleOption *option)
{
    if (!(option->state & (QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus))) return;
    QStyleOption pill(*option); pill.rect.adjust(2, 2, -2, -2);
    if (option->state & QStyle::State_Selected) pill.state |= QStyle::State_On;
    paintPill(painter, &pill, QPalette::Window);
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
        if (!style || dialog->style() != style || !layout) return;
        if (!layout->property("_sequoia_picker_margins").isValid()) {
            layout->setProperty("_sequoia_picker_margins", QVariant::fromValue(layout->contentsMargins()));
            layout->setProperty("_sequoia_picker_spacing", layout->spacing());
        }
        // Shared library Tokens.panelPadding / Tokens.gap.
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(8);
        auto chrome = static_cast<Chrome *>(dialog->findChild<QObject *>(QStringLiteral("sequoiaPickerChrome"), Qt::FindDirectChildrenOnly));
        if (!chrome) chrome = new Chrome(dialog);
        for (auto child : dialog->findChildren<QWidget *>()) chrome->decorate(child);
        installPreview(dialog);
    });
}

void unpolish(QWidget *widget)
{
    if (auto chrome = static_cast<Chrome *>(widget->findChild<QObject *>(QStringLiteral("sequoiaPickerChrome"), Qt::FindDirectChildrenOnly))) {
        chrome->restore(widget);
        delete chrome;
        // Replacing a preview rebuilds KDE's view. Wait until QApplication has
        // finished iterating the old widgets during a style switch.
        QTimer::singleShot(0, widget, [widget] {
            if (!widget->style()->objectName().startsWith(QLatin1String("sequoia"), Qt::CaseInsensitive))
                restorePreview(widget);
        });
    }
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
