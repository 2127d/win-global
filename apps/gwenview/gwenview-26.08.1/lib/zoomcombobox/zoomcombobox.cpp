// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "zoomcombobox.h"
#include "zoomcombobox_p.h"

#include <KLocalizedString>
#include <QAbstractItemView>
#include <QAction>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <QPainter>
#include <QStyleOptionComboBox>

#include <cmath>

bool fuzzyEqual(qreal a, qreal b)
{
    return (qFuzzyIsNull(a) && qFuzzyIsNull(b)) || qFuzzyCompare(a, b);
}

bool fuzzyLessEqual(qreal a, qreal b)
{
    return fuzzyEqual(a, b) || a < b;
}

bool fuzzyGreaterEqual(qreal a, qreal b)
{
    return fuzzyEqual(a, b) || a > b;
}

using namespace Gwenview;

struct LineEditSelectionKeeper {
    LineEditSelectionKeeper(QLineEdit *lineEdit)
        : m_lineEdit(lineEdit)
    {
        Q_ASSERT(m_lineEdit);
        m_cursorPos = m_lineEdit->cursorPosition();
    }

    ~LineEditSelectionKeeper()
    {
        m_lineEdit->end(false);
        m_lineEdit->cursorBackward(true, m_lineEdit->text().length() - m_cursorPos);
    }

private:
    QLineEdit *m_lineEdit;
    int m_cursorPos;
};

ZoomValidator::ZoomValidator(qreal minimum, qreal maximum, ZoomComboBox *q, ZoomComboBoxPrivate *d, QWidget *parent)
    : QValidator(parent)
    , m_minimum(minimum)
    , m_maximum(maximum)
    , m_zoomComboBox(q)
    , m_zoomComboBoxPrivate(d)
{
}

ZoomValidator::~ZoomValidator() noexcept = default;

qreal ZoomValidator::minimum() const
{
    return m_minimum;
}

void ZoomValidator::setMinimum(const qreal minimum)
{
    if (fuzzyEqual(m_minimum, minimum)) {
        return;
    }
    m_minimum = minimum;
    Q_EMIT changed();
}

qreal ZoomValidator::maximum() const
{
    return m_maximum;
}

void ZoomValidator::setMaximum(const qreal maximum)
{
    if (fuzzyEqual(m_maximum, maximum)) {
        return;
    }
    m_maximum = maximum;
    Q_EMIT changed();
}

QValidator::State ZoomValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)
    if (m_zoomComboBox->findText(input, Qt::MatchFixedString) > -1) {
        return QValidator::Acceptable;
    }

    QString copy = input.trimmed();
    copy.remove(locale().groupSeparator());
    copy.remove(locale().percent());
    const bool startsWithNumber = copy.constBegin()->isNumber();

    if (startsWithNumber || copy.isEmpty()) {
        return QValidator::Intermediate;
    }

    QValidator::State state;
    bool ok = false;
    int value = locale().toInt(copy, &ok);
    if (!ok || value < std::ceil(m_minimum * 100) || value > std::floor(m_maximum * 100)) {
        state = QValidator::Intermediate;
    } else {
        state = QValidator::Acceptable;
    }
    return state;
}

ZoomComboBoxPrivate::ZoomComboBoxPrivate(ZoomComboBox *q)
    : q_ptr(q)
    , validator(new ZoomValidator(0, 0, q, this, q))
{
}

ZoomComboBox::ZoomComboBox(QWidget *parent)
    : QComboBox(parent)
    , d_ptr(new ZoomComboBoxPrivate(this))
{
    Q_D(ZoomComboBox);

    d->validator->setObjectName(QLatin1String("zoomValidator"));
    setValidator(d->validator);

    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    // QLocale::percent() will return a QString in Qt 6.
    // Qt encourages using QString(locale().percent()) in QLocale documentation.
    const int percentLength = QString(locale().percent()).length();
    setMinimumContentsLength(locale().toString(9999).length() + percentLength);

    connect(lineEdit(), &QLineEdit::textEdited, this, [this, d](const QString &text) {
        const bool startsWithNumber = text.constBegin()->isNumber();
        int matchedIndex = -1;
        if (startsWithNumber) {
            matchedIndex = findText(text, Qt::MatchFixedString);
        } else {
            // check if there is more than 1 match
            for (int i = 0, n = count(); i < n; ++i) {
                if (itemText(i).startsWith(text, Qt::CaseInsensitive)) {
                    if (matchedIndex != -1) {
                        // there is more than 1 match
                        return;
                    }
                    matchedIndex = i;
                }
            }
        }
        if (matchedIndex != -1) {
            LineEditSelectionKeeper selectionKeeper(lineEdit());
            updateCurrentIndex();
            if (matchedIndex == currentIndex()) {
                updateDisplayedText();
            } else {
                activateAndChangeZoomTo(matchedIndex);
            }
        } else if (startsWithNumber) {
            bool ok = false;
            qreal value = valueFromText(text, &ok);
            if (ok && value > 0 && fuzzyLessEqual(value, maximum())) {
                // emulate autocompletion for valid values that aren't predefined
                while (value < minimum()) {
                    value *= 10;
                    if (value > maximum()) {
                        // autocompletion cannot be emulated for this value
                        return;
                    }
                }
                LineEditSelectionKeeper selectionKeeper(lineEdit());
                if (fuzzyEqual(value, d->value)) {
                    updateDisplayedText();
                } else {
                    d->lastCustomZoomValue = value;
                    activateAndChangeZoomTo(-1);
                }
            }
        }
    });
    // Browsing presets must not change the image or its layout while the popup
    // is scrolling. Commit only on activation (click or Enter), like a normal
    // combo box; cancelling then leaves the current scale untouched.
    view()->installEventFilter(this);
    lineEdit()->installEventFilter(this);
    connect(this, qOverload<int>(&ZoomComboBox::activated), this, &ZoomComboBox::activateAndChangeZoomTo);
}

ZoomComboBox::~ZoomComboBox() noexcept = default;

void ZoomComboBox::setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction)
{
    Q_D(ZoomComboBox);
    d->setActions(zoomToFitAction, zoomToFillAction, actualSizeAction);

    connect(zoomToFitAction, &QAction::toggled, this, &ZoomComboBox::updateDisplayedText);
    connect(zoomToFillAction, &QAction::toggled, this, &ZoomComboBox::updateDisplayedText);
}

void ZoomComboBoxPrivate::setActions(QAction *zoomToFitAction, QAction *zoomToFillAction, QAction *actualSizeAction)
{
    Q_Q(ZoomComboBox);
    q->clear();
    // Add items reversed, so there's a consistent behavior between zoom slider and this combo box
    q->addItem(actualSizeAction->iconText(), QVariant::fromValue(actualSizeAction));
    q->addItem(zoomToFillAction->iconText(), QVariant::fromValue(zoomToFillAction));
    q->addItem(zoomToFitAction->iconText(), QVariant::fromValue(zoomToFitAction));

    mZoomToFitAction = zoomToFitAction;
    mZoomToFillAction = zoomToFillAction;
    mActualSizeAction = actualSizeAction;
}

qreal ZoomComboBox::value() const
{
    Q_D(const ZoomComboBox);
    return d->value;
}

void ZoomComboBox::setValue(qreal value)
{
    Q_D(ZoomComboBox);
    d->value = value;

    updateDisplayedText();
}

qreal ZoomComboBox::minimum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->minimum();
}

void ZoomComboBox::setMinimum(qreal minimum)
{
    Q_D(ZoomComboBox);
    if (fuzzyEqual(this->minimum(), minimum)) {
        return;
    }
    d->validator->setMinimum(minimum);
    const QSignalBlocker blocker(this);
    // Presets are descending. Always insert below 100% at the same position;
    // decrementing the insertion index interleaves them with the larger scales.
    for (int i = count() - 1; i >= 0; --i) {
        if (!itemData(i).value<QAction *>() && itemData(i).toReal() < 1.0) removeItem(i);
    }
    const int insertion = findData(QVariant::fromValue(d->mActualSizeAction)) + 1;
    for (qreal value = minimum * 2; value > 0 && value < 1.0; value *= 2) {
        insertItem(insertion, textFromValue(value), QVariant::fromValue(value));
    }
    d->value = qMax(minimum, d->value);
    updateCurrentIndex();
    updateDisplayedText();
}

qreal ZoomComboBox::maximum() const
{
    Q_D(const ZoomComboBox);
    return d->validator->maximum();
}

void ZoomComboBox::setMaximum(qreal maximum)
{
    Q_D(ZoomComboBox);
    if (fuzzyEqual(this->maximum(), maximum)) {
        return;
    }
    d->validator->setMaximum(maximum);
    const QSignalBlocker blocker(this);
    for (int i = count() - 1; i >= 0; --i) {
        if (!itemData(i).value<QAction *>() && itemData(i).toReal() > 1.0) removeItem(i);
    }
    for (qreal value = 2.0; value < maximum; value *= 2) {
        insertItem(0, textFromValue(value), QVariant::fromValue(value));
    }
    if (maximum > 1.0) insertItem(0, textFromValue(maximum), QVariant::fromValue(maximum));
    d->value = qMin(d->value, maximum);
    updateCurrentIndex();
    updateDisplayedText();
}

qreal ZoomComboBox::valueFromText(const QString &text, bool *ok) const
{
    Q_D(const ZoomComboBox);

    const QLocale l = locale();
    QString s = text;
    s.remove(l.groupSeparator());

    if (s.endsWith(l.percent())) {
        s = s.chopped(1);
    }

    if (s.startsWith(l.percent())) {
        s = s.remove(0, 1);
    }

    return l.toDouble(s, ok) / 100.0;
}

QString ZoomComboBox::textFromValue(const qreal value) const
{
    Q_D(const ZoomComboBox);

    QLocale l = locale();
    l.setNumberOptions(QLocale::OmitGroupSeparator);

    QString formattedValue = l.toString(qRound(value * 100));
    return i18nc("Percent value", "%1%", formattedValue);
}

void ZoomComboBox::updateDisplayedText()
{
    Q_D(ZoomComboBox);
    if (d->mZoomToFitAction->isChecked()) {
        lineEdit()->setText(d->mZoomToFitAction->iconText());
    } else if (d->mZoomToFillAction->isChecked()) {
        lineEdit()->setText(d->mZoomToFillAction->iconText());
    } else if (d->mActualSizeAction->isChecked()) {
        lineEdit()->setText(d->mActualSizeAction->iconText());
    } else {
        lineEdit()->setText(textFromValue(d->value));
    }
}

void Gwenview::ZoomComboBox::showPopup()
{
    updateCurrentIndex();
    // Custom zoom values have no matching preset. Selecting index -1 clears
    // an editable combo's text, so restore the current scale before opening.
    updateDisplayedText();

    // We don't want to emit a QComboBox::highlighted event just because the popup is opened.
    const QSignalBlocker blocker(this);
    QComboBox::showPopup();
}

bool ZoomComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == lineEdit() && event->type() == QEvent::Wheel && !view()->isVisible()) {
        wheelEvent(static_cast<QWheelEvent *>(event));
        return true;
    }
    if (watched == view()) {
        switch (event->type()) {
        case QEvent::ShortcutOverride: {
            if (view()->isVisibleTo(this)) {
                auto keyEvent = static_cast<QKeyEvent *>(event);
                if (keyEvent->key() == Qt::Key_Escape) {
                    event->accept();
                }
            }
            break;
        }
        default:
            break;
        }
    }

    return QComboBox::eventFilter(watched, event);
}

void ZoomComboBox::focusOutEvent(QFocusEvent *event)
{
    Q_D(ZoomComboBox);
    // Should the user have started typing a custom value
    // that was out of our range, then we have a temporary
    // state that is illegal. This is needed to allow a user
    // to type a zoom with multiple keystrokes, but when the
    // user leaves focus we should reset to the last known 'good'
    // zoom value.
    if (d->lastSelectedIndex == -1)
        setValue(d->lastCustomZoomValue);

    QComboBox::focusOutEvent(event);
}

void ZoomComboBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp: {
        updateCurrentIndex();
        if (currentIndex() != -1) {
            break;
        }
        moveCurrentIndex(event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown);
        return;
    }
    default:
        break;
    }

    QComboBox::keyPressEvent(event);
}

void ZoomComboBox::wheelEvent(QWheelEvent *event)
{
    Q_D(ZoomComboBox);
    if (view()->isVisible()) {
        QComboBox::wheelEvent(event);
        return;
    }
    // Accumulate high-resolution wheels/trackpads. A direction reversal starts
    // a fresh step, rather than spending the first gesture cancelling residue.
    const bool pixels = event->angleDelta().y() == 0 && event->pixelDelta().y() != 0;
    const int delta = pixels ? event->pixelDelta().y() : event->angleDelta().y();
    if (event->phase() == Qt::ScrollBegin || pixels != d->wheelUsesPixels || delta * d->wheelRemainder < 0)
        d->wheelRemainder = 0;
    d->wheelUsesPixels = pixels;
    d->wheelRemainder += delta;
    const int threshold = pixels ? 40 : 120;
    const int steps = d->wheelRemainder / threshold;
    d->wheelRemainder %= threshold;
    for (int i = 0; i < qMin(qAbs(steps), count()); ++i) moveCurrentIndex(steps < 0);
    if (event->phase() == Qt::ScrollEnd) d->wheelRemainder = 0;
    event->accept();
}

void ZoomComboBox::paintEvent(QPaintEvent *event)
{
    if (!property("floatingZoomPill").toBool()) {
        QComboBox::paintEvent(event);
        return;
    }
    // Keep the native editable field and arrow hit geometry, but give the
    // floating variant a continuous capsule instead of the style's square edge.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF bounds = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool dark = palette().color(QPalette::Button).lightness() < 128;
    painter.setBrush(palette().color(QPalette::Button));
    painter.setPen(QPen(hasFocus() || lineEdit()->hasFocus() ? palette().color(QPalette::Highlight)
                        : QColor(0, 0, 0, dark ? 85 : 32), 1));
    painter.drawRoundedRect(bounds, bounds.height() / 2, bounds.height() / 2);
    QStyleOptionComboBox option;
    initStyleOption(&option);
    const QRect arrow = style()->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, this);
    const QPointF center = arrow.center();
    painter.setPen(QPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText),
                        1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(QPolygonF{center + QPointF(-3.5, -1.5), center + QPointF(0, 2), center + QPointF(3.5, -1.5)});
}

void ZoomComboBox::updateCurrentIndex()
{
    Q_D(ZoomComboBox);
    const int count = this->count();
    if (d->mZoomToFitAction->isChecked()) {
        setCurrentIndex(count - 1); // 'zoom to fit' is always last
        d->lastSelectedIndex = count - 1;
    } else if (d->mZoomToFillAction->isChecked()) {
        setCurrentIndex(count - 2);
        d->lastSelectedIndex = count - 2; // 'zoom to fill' is always last - 1
    } else if (d->mActualSizeAction->isChecked()) {
        const int actualSizeActionIndex = findData(QVariant::fromValue(d->mActualSizeAction));
        setCurrentIndex(actualSizeActionIndex);
        d->lastSelectedIndex = actualSizeActionIndex;
    } else {
        // Now is a good time to save the zoom value that was selected before the user changes it through the popup.
        d->lastCustomZoomValue = d->value;

        // Highlight the correct index if the current zoom value exists as an option in the popup.
        // If it doesn't exist, it is set to -1.
        d->lastSelectedIndex = findText(textFromValue(d->value));
        setCurrentIndex(d->lastSelectedIndex);
    }
}

void ZoomComboBox::moveCurrentIndex(bool decrease)
{
    Q_D(ZoomComboBox);
    // Compare actual scale values, not rounded labels or action indices. Fit
    // and Fill can resolve to arbitrary percentages, and the image may clamp
    // a requested scale; neither should send the next wheel step backwards.
    int next = -1;
    qreal closest = decrease ? -1 : maximum() + 1;
    const qreal tolerance = qMax(qreal(1e-8), qAbs(d->value) * 1e-6);
    for (int i = 0; i < count(); ++i) {
        const auto action = itemData(i).value<QAction *>();
        if (action && action != d->mActualSizeAction) continue;
        const qreal scale = action ? 1.0 : itemData(i).toReal();
        if (scale < minimum() || scale > maximum()) continue;
        if (decrease ? scale < d->value - tolerance && scale > closest
                     : scale > d->value + tolerance && scale < closest) {
            next = i;
            closest = scale;
        }
    }
    if (next < 0 && decrease && d->value > minimum() + tolerance)
        next = findData(QVariant::fromValue(d->mZoomToFitAction));
    if (next < 0) return;
    setCurrentIndex(next);
    Q_EMIT activated(next);
    updateDisplayedText();
}

void ZoomComboBox::changeZoomTo(int index)
{
    if (index < 0) {
        Q_D(ZoomComboBox);
        Q_EMIT zoomChanged(d->lastCustomZoomValue);
        return;
    }

    QVariant itemData = this->itemData(index);
    auto action = itemData.value<QAction *>();
    if (action) {
        if (!action->isCheckable() || !action->isChecked()) {
            action->trigger();
        }
    } else if (itemData.canConvert(QMetaType::QReal)) {
        Q_EMIT zoomChanged(itemData.toReal());
    }
}

void ZoomComboBox::activateAndChangeZoomTo(int index)
{
    Q_D(ZoomComboBox);
    d->lastSelectedIndex = index;

    // The user has explicitly selected this zoom value so we
    // remember it the same way as if they had typed it themselves.
    QVariant itemData = this->itemData(index);
    if (!itemData.value<QAction *>() && itemData.canConvert(QMetaType::QReal)) {
        d->lastCustomZoomValue = itemData.toReal();
    }

    changeZoomTo(index);
}

#include "moc_zoomcombobox.cpp"

#include "moc_zoomcombobox_p.cpp"
