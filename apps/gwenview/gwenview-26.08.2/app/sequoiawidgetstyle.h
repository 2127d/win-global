// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <lib/sequoiadesign.h>
#include <QAbstractButton>
#include <QApplication>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOption>
#include <QVariantAnimation>

namespace Gwenview {
class ControlFeedback : public QObject
{
public:
    explicit ControlFeedback(QWidget *widget) : QObject(widget), mWidget(widget)
    {
        setObjectName(QStringLiteral("sequoiaControlFeedback"));
        mAnimation = new QVariantAnimation(this);
        mAnimation->setDuration(Sequoia::ShortDuration);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mValue = value.toReal(); mWidget->update();
        });
        widget->setAttribute(Qt::WA_Hover); widget->installEventFilter(this);
    }
    qreal value() const { return mValue; }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange) {
            const qreal target = event->type() != QEvent::Leave && mWidget->isEnabled() && mWidget->underMouse() ? 1 : 0;
            mAnimation->stop();
            if (mWidget->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mWidget) == 0) {
                mValue = target; mWidget->update();
            } else {
                mAnimation->setStartValue(mValue); mAnimation->setEndValue(target); mAnimation->start();
            }
        }
        return false;
    }
private:
    QWidget *mWidget;
    QVariantAnimation *mAnimation;
    qreal mValue = 0;
};

// Weather's palette, radii and control states, with KDE still supplying labels,
// sizing, accessibility, shortcut handling and all complex-control hit testing.
class SequoiaWidgetStyle : public QProxyStyle
{
public:
    explicit SequoiaWidgetStyle(const QString &base) : QProxyStyle(base) { setObjectName(base); }
    void polish(QWidget *widget) override
    {
        QProxyStyle::polish(widget);
        if ((qobject_cast<QAbstractButton *>(widget) || widget->inherits("QLineEdit") || widget->inherits("QComboBox") || widget->inherits("QAbstractSpinBox"))
            && !widget->findChild<QObject *>(QStringLiteral("sequoiaControlFeedback"), Qt::FindDirectChildrenOnly))
            new ControlFeedback(widget);
    }
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override
    {
        if (widget && widget->inherits("QCheckBox")) {
            if (metric == PM_IndicatorWidth) return 38;
            if (metric == PM_IndicatorHeight) return 22;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget = nullptr) const override
    {
        QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (type == CT_CheckBox && widget && widget->inherits("QCheckBox")) {
            // The base desktop style uses a compact indicator in its label
            // metrics. Reserve the extra space for Weather's 38px switch.
            result.rwidth() += 18; result.setHeight(qMax(24, result.height()));
        }
        return result;
    }
    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override
    {
        if (widget && widget->inherits("QCheckBox")) {
            if (element == SE_CheckBoxIndicator)
                return visualRect(option->direction, option->rect, QRect(option->rect.left(), option->rect.center().y() - 10, 38, 22));
            if (element == SE_CheckBoxContents || element == SE_CheckBoxFocusRect)
                return visualRect(option->direction, option->rect, option->rect.adjusted(46, 0, 0, 0));
            if (element == SE_CheckBoxClickRect) return option->rect;
        }
        return QProxyStyle::subElementRect(element, option, widget);
    }
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override
    {
        if (element == PE_PanelButtonCommand || element == PE_PanelButtonTool) {
            panel(option, painter, widget, false, element == PE_PanelButtonTool && (option->state & State_AutoRaise)); return;
        }
        if (element == PE_PanelLineEdit || element == PE_FrameLineEdit) {
            if (auto frame = qstyleoption_cast<const QStyleOptionFrame *>(option); frame && frame->lineWidth == 0) return;
            panel(option, painter, widget, true); return;
        }
        if (element == PE_IndicatorCheckBox && widget && widget->inherits("QCheckBox")) {
            painter->save(); painter->setRenderHint(QPainter::Antialiasing);
            const QRectF track = QRectF(option->rect).adjusted(1, 1, -1, -1);
            const bool checked = option->state & State_On, partial = option->state & State_NoChange;
            QColor fill = checked || partial ? option->palette.color(QPalette::Highlight) : Sequoia::alpha(option->palette.color(QPalette::WindowText), 0.20);
            if (!(option->state & State_Enabled)) fill.setAlphaF(0.35);
            painter->setPen(Qt::NoPen); painter->setBrush(fill); painter->drawRoundedRect(track, track.height()/2, track.height()/2);
            const bool right = checked != (option->direction == Qt::RightToLeft);
            const qreal radius = track.height()/2 - 2;
            const QPointF center(partial ? track.center().x() : right ? track.right() - radius - 2 : track.left() + radius + 2, track.center().y());
            painter->setBrush(Qt::white); painter->drawEllipse(center, radius, radius);
            if (option->state & State_HasFocus) {
                painter->setBrush(Qt::NoBrush); painter->setPen(QPen(option->palette.color(QPalette::Highlight), 2));
                painter->drawRoundedRect(QRectF(option->rect), 11, 11);
            }
            painter->restore(); return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override
    {
        if (element == CE_CheckBox && widget && widget->inherits("QCheckBox")) {
            if (auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                auto indicator = *button; indicator.rect = subElementRect(SE_CheckBoxIndicator, option, widget);
                drawPrimitive(PE_IndicatorCheckBox, &indicator, painter, widget);
                auto label = *button; label.rect = subElementRect(SE_CheckBoxContents, option, widget);
                QProxyStyle::drawControl(CE_CheckBoxLabel, &label, painter, widget); return;
            }
        }
        if (element == CE_PushButton) {
            if (auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                panel(option, painter, widget, false, button->features & QStyleOptionButton::Flat,
                      button->features & QStyleOptionButton::DefaultButton);
                auto label = *button; label.state &= ~(State_HasFocus | State_On | State_Sunken);
                QProxyStyle::drawControl(CE_PushButtonLabel, &label, painter, widget); return;
            }
        }
        if (element == CE_MenuItem) {
            if (auto item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
                auto copy = *item;
                if (item->state & State_Selected) {
                    painter->save(); painter->setRenderHint(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen); painter->setBrush(Sequoia::alpha(item->palette.color(QPalette::Highlight), 0.14));
                    painter->drawRoundedRect(QRectF(item->rect).adjusted(1, 1, -1, -1), Sequoia::RowRadius, Sequoia::RowRadius);
                    painter->restore(); copy.state &= ~State_Selected;
                }
                QProxyStyle::drawControl(element, &copy, painter, widget); return;
            }
        }
        if (element == CE_ComboBoxLabel) {
            if (auto combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                auto label = *combo; label.state &= ~(State_HasFocus | State_On | State_Sunken);
                QProxyStyle::drawControl(element, &label, painter, widget); return;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override
    {
        if (control == CC_ComboBox) {
            if (auto combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
                panel(option, painter, widget, combo->editable);
                auto arrow = *combo; arrow.subControls = SC_ComboBoxArrow; arrow.state &= ~(State_HasFocus | State_On | State_Sunken);
                QProxyStyle::drawComplexControl(control, &arrow, painter, widget); return;
            }
        }
        if (control == CC_SpinBox) {
            if (auto spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
                panel(option, painter, widget, true);
                auto arrows = *spin; arrows.subControls &= ~SC_SpinBoxFrame;
                QProxyStyle::drawComplexControl(control, &arrows, painter, widget); return;
            }
        }
        if (control == CC_ToolButton) {
            if (auto button = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
                const bool flat = button->state & State_AutoRaise;
                panel(option, painter, widget, false, flat);
                auto label = *button; label.state &= ~(State_HasFocus | State_On | State_Sunken);
                label.rect = subControlRect(CC_ToolButton, option, SC_ToolButton, widget);
                QProxyStyle::drawControl(CE_ToolButtonLabel, &label, painter, widget);
                if (button->features & QStyleOptionToolButton::MenuButtonPopup) {
                    QStyleOption arrow; arrow.rect = subControlRect(CC_ToolButton, option, SC_ToolButtonMenu, widget).adjusted(3, 3, -3, -3);
                    arrow.palette = option->palette; arrow.state = option->state;
                    QProxyStyle::drawPrimitive(PE_IndicatorArrowDown, &arrow, painter, widget);
                }
                return;
            }
        }
        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }
private:
    void panel(const QStyleOption *option, QPainter *painter, const QWidget *widget, bool field, bool flat = false, bool primary = false) const
    {
        const bool enabled = option->state & State_Enabled, pressed = enabled && !field && (option->state & State_Sunken);
        const bool selected = primary || (option->state & State_On);
        qreal hover = enabled && (option->state & State_MouseOver) ? 1 : 0;
        if (widget) if (auto feedback = static_cast<ControlFeedback *>(widget->findChild<QObject *>(QStringLiteral("sequoiaControlFeedback"), Qt::FindDirectChildrenOnly))) hover = feedback->value();
        const auto palette = option->palette;
        QColor fill = field ? palette.color(QPalette::Base) : palette.color(QPalette::Window);
        if (flat) fill = Qt::transparent;
        const QColor ink = palette.color(QPalette::WindowText);
        if (selected) fill = Sequoia::tint(palette.color(QPalette::Window), palette.color(QPalette::Highlight), 0.16);
        else if (flat) fill = Sequoia::alpha(ink, pressed ? 0.14 : 0.08 * hover);
        else fill = Sequoia::tint(fill, ink, pressed ? 0.10 : 0.04 * hover);
        if (!enabled) fill.setAlphaF(fill.alphaF() * 0.55);
        const bool focus = enabled && (option->state & State_HasFocus);
        const QRectF rect = QRectF(option->rect).adjusted(1, 1, -1, -1);
        const qreal radius = qMin(Sequoia::ControlRadius, rect.height()/2);
        painter->save(); painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(focus ? QPen(palette.color(QPalette::Highlight), 2) : flat ? QPen(Qt::NoPen) : QPen(Sequoia::border(palette), 1));
        painter->setBrush(fill); painter->drawRoundedRect(rect, radius, radius); painter->restore();
    }
};
}
