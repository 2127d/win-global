// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QLayout>
#include <QPainter>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>

namespace Gwenview {
// Animate one cached frame inside the native dialog. This works on Wayland too,
// without moving the top-level window or repainting its form on every frame.
class DialogEntrance : public QWidget
{
public:
    explicit DialogEntrance(QDialog *dialog) : QWidget(dialog), mDialog(dialog)
    {
        setObjectName(QStringLiteral("dialogEntrance"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        hide();
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("dialogEntranceAnimation"));
        mAnimation->setDuration(140);
        mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        mAnimation->setEasingCurve(QEasingCurve::OutCubic);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mProgress = value.toReal(); update();
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        qApp->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == mDialog && event->type() == QEvent::Show && !mStarted) {
            mStarted = true; mPending = true;
            QTimer::singleShot(0, this, [this] {
                if (!mPending || !mDialog->isVisible()) return;
                mPending = false;
                if (mDialog->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mDialog) == 0) return;
                mFrame = mDialog->grab();
                setGeometry(mDialog->rect()); mProgress = 0;
                show(); raise(); mAnimation->start();
            });
        } else if (object == mDialog && (event->type() == QEvent::Hide || event->type() == QEvent::Resize)) {
            if (isVisible()) finish();
            if (event->type() == QEvent::Hide) mPending = false;
        } else if ((isVisible() || mPending) && (event->type() == QEvent::MouseButtonPress
                   || event->type() == QEvent::KeyPress || event->type() == QEvent::Wheel || event->type() == QEvent::TouchBegin)) {
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && widget->window() == mDialog) finish();
        }
        return QWidget::eventFilter(object, event);
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), mDialog->palette().color(QPalette::Window));
        painter.setOpacity(0.15 + 0.85 * mProgress);
        painter.drawPixmap(QPoint(0, qRound(10 * (1 - mProgress))), mFrame);
    }
private:
    void finish() { mPending = false; mAnimation->stop(); hide(); mFrame = QPixmap(); }
    QDialog *mDialog;
    QVariantAnimation *mAnimation;
    QPixmap mFrame;
    qreal mProgress = 0;
    bool mStarted = false;
    bool mPending = false;
};

inline void polishDialog(QDialog *dialog)
{
    dialog->setObjectName(QStringLiteral("gwenviewPropertiesDialog"));
    // Palette roles stay live when the desktop changes between light and dark.
    dialog->setStyleSheet(QStringLiteral(R"(
        QDialog#gwenviewPropertiesDialog { background: palette(window); }
        QTabWidget::pane { border: 1px solid palette(mid); border-radius: 12px; padding: 8px; }
        QTabBar::tab { padding: 7px 14px; margin: 2px; border: 1px solid transparent; border-radius: 10px; }
        QTabBar::tab:selected { background: palette(base); border-color: palette(mid); color: palette(text); }
        QTabBar::tab:hover:!selected { background: palette(alternate-base); }
        QPushButton { min-height: 22px; padding: 5px 16px; border: 1px solid palette(mid); border-radius: 10px; background: palette(button); color: palette(button-text); }
        QPushButton:hover { background: palette(alternate-base); }
        QPushButton:default { background: palette(highlight); border-color: palette(highlight); color: palette(highlighted-text); }
        QPushButton:pressed { border-color: palette(highlight); }
        QPushButton:focus { border-color: palette(highlight); }
        QPushButton:disabled { color: palette(mid); }
        QLineEdit { min-height: 22px; padding: 4px 8px; border: 1px solid palette(mid); border-radius: 8px; background: palette(base); color: palette(text); }
        QLineEdit:focus { border-color: palette(highlight); }
    )"));
    if (dialog->layout()) { dialog->layout()->setContentsMargins(18, 18, 18, 18); dialog->layout()->setSpacing(14); }
    new DialogEntrance(dialog);
}
}
