// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "floatingsurface.h"
#include <KWindowEffects>
#include <QDialog>
#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QScreen>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

namespace Gwenview {
class DialogCloseButton : public QToolButton
{
public:
    explicit DialogCloseButton(QDialog *dialog) : QToolButton(dialog)
    {
        setObjectName(QStringLiteral("dialogCloseButton"));
        setFixedSize(32, 32); setAutoRaise(true); setFocusPolicy(Qt::TabFocus);
        setText(tr("Close")); setAccessibleName(text()); setToolTip(text());
        connect(this, &QAbstractButton::clicked, dialog, &QDialog::reject);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QPointF center = QRectF(rect()).center();
        if (hasFocus()) {
            p.setPen(QPen(palette().color(QPalette::Highlight), 2)); p.setBrush(Qt::NoBrush);
            p.drawEllipse(center, 12, 12);
        }
        p.setPen(QPen(QColor("#d94c45"), 1)); p.setBrush(isDown() ? QColor("#e0443e") : QColor("#ff5f57"));
        p.drawEllipse(center, 8, 8);
        if (underMouse() || isDown() || hasFocus()) {
            p.setPen(QPen(QColor("#710e09"), 1.2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(center + QPointF(-2.5, -2.5), center + QPointF(2.5, 2.5));
            p.drawLine(center + QPointF(-2.5, 2.5), center + QPointF(2.5, -2.5));
        }
    }
};

// Weather's 24px dialog surface and centered header around the original KDE
// content. Native modality, accept/reject, focus and file operations stay intact.
class DialogShell : public QObject
{
public:
    explicit DialogShell(QDialog *dialog) : QObject(dialog), mDialog(dialog)
    {
        const bool wasVisible = dialog->isVisible();
        setObjectName(QStringLiteral("dialogShell"));
        dialog->setWindowFlag(Qt::FramelessWindowHint);
        dialog->setWindowFlag(Qt::NoDropShadowWindowHint);
        dialog->setProperty("_KDE_NET_WM_SKIP_SHADOW", true);
        dialog->setAttribute(Qt::WA_TranslucentBackground);
        dialog->setAutoFillBackground(false);
        auto content = new QWidget(dialog);
        content->setObjectName(QStringLiteral("dialogShellContent"));
        content->setLayout(dialog->layout());
        auto layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(14, 14, 14, 18); layout->setSpacing(0);
        mHeader = new QWidget(dialog);
        mHeader->setObjectName(QStringLiteral("dialogShellHeader"));
        mHeader->setFixedHeight(56);
        auto header = new QHBoxLayout(mHeader);
        header->setContentsMargins(12, 12, 12, 12); header->setSpacing(12);
        header->addWidget(new DialogCloseButton(dialog));
        mTitle = new QLabel(dialog->windowTitle(), mHeader);
        mTitle->setObjectName(QStringLiteral("dialogShellTitle"));
        mTitle->setAlignment(Qt::AlignCenter);
        mTitle->setTextFormat(Qt::PlainText);
        mTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        QFont font = mTitle->font(); font.setPixelSize(18); font.setBold(true); mTitle->setFont(font);
        header->addWidget(mTitle, 1); header->addSpacing(32);
        layout->addWidget(mHeader); layout->addWidget(content, 1);
        mHeader->installEventFilter(this); mTitle->installEventFilter(this);
        qApp->installEventFilter(this); dialog->setMouseTracking(true);
        // Some platform dialogs only expose their layout on Show. Changing
        // window flags hides a widget; restore visibility synchronously so
        // QFileDialog's helper does not mistake that for a failed native show.
        if (wasVisible) dialog->show();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        // The edge cursor belongs to the shell only. Children without an
        // explicit cursor inherit it, so clear it before entering content.
        auto target = qobject_cast<QWidget *>(object);
        if (target && target != mDialog && mDialog->isAncestorOf(target)
            && (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove))
            mDialog->unsetCursor();
        if (object == mDialog && (event->type() == QEvent::Leave || event->type() == QEvent::Hide
            || event->type() == QEvent::WindowDeactivate)) mDialog->unsetCursor();
        if (object == mHeader || object == mTitle) {
            if (object == mTitle && event->type() == QEvent::Resize) {
                mTitle->setText(mTitle->fontMetrics().elidedText(mDialog->windowTitle(), Qt::ElideRight, mTitle->width()));
                mTitle->setToolTip(mDialog->windowTitle());
            }
            if (event->type() == QEvent::MouseButtonPress) {
                auto mouse = static_cast<QMouseEvent *>(event);
                if (mouse->button() == Qt::LeftButton && mDialog->windowHandle())
                    return mDialog->windowHandle()->startSystemMove();
            }
            return false;
        }
        if (object != mDialog) return false;
        if (event->type() == QEvent::Paint) {
            QPainter p(mDialog);
            p.setCompositionMode(QPainter::CompositionMode_Source); p.fillRect(mDialog->rect(), Qt::transparent);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            paintFloatingShadow(p, body(), Sequoia::SurfaceRadius, mDialog->palette(), true, 0.65);
            paintPillSurface(p, body(), Sequoia::SurfaceRadius, mDialog->palette());
            return true;
        }
        if (event->type() == QEvent::WindowTitleChange) {
            mTitle->setText(mTitle->fontMetrics().elidedText(mDialog->windowTitle(), Qt::ElideRight, mTitle->width()));
            mTitle->setToolTip(mDialog->windowTitle());
        }
        if (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::PaletteChange) {
            QTimer::singleShot(0, this, [this] { refreshBlur(); });
            if (event->type() == QEvent::Show && mDialog->parentWidget()) {
                auto owner = mDialog->parentWidget()->window();
                const QRect available = mDialog->screen()->availableGeometry();
                QPoint position = owner->mapToGlobal(owner->rect().center()) - mDialog->rect().center();
                position.setX(qBound(available.left(), position.x(), qMax(available.left(), available.right() - mDialog->width())));
                position.setY(qBound(available.top(), position.y(), qMax(available.top(), available.bottom() - mDialog->height())));
                mDialog->move(position);
            }
        }
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
            auto mouse = static_cast<QMouseEvent *>(event);
            Qt::Edges edges;
            const QPointF point = mouse->position();
            if (mDialog->minimumWidth() < mDialog->maximumWidth()) {
                if (point.x() < 22) edges |= Qt::LeftEdge;
                if (point.x() > mDialog->width() - 22) edges |= Qt::RightEdge;
            }
            if (mDialog->minimumHeight() < mDialog->maximumHeight()) {
                if (point.y() < 22) edges |= Qt::TopEdge;
                if (point.y() > mDialog->height() - 26) edges |= Qt::BottomEdge;
            }
            if (event->type() == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton && edges && mDialog->windowHandle())
                return mDialog->windowHandle()->startSystemResize(edges);
            const bool horizontal = edges & (Qt::LeftEdge | Qt::RightEdge), vertical = edges & (Qt::TopEdge | Qt::BottomEdge);
            mDialog->setCursor(horizontal && vertical ? ((edges.testFlag(Qt::LeftEdge) == edges.testFlag(Qt::TopEdge)) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor)
                              : horizontal ? Qt::SizeHorCursor : vertical ? Qt::SizeVerCursor : Qt::ArrowCursor);
        }
        return false;
    }
private:
    QRectF body() const { return QRectF(mDialog->rect()).adjusted(14, 14, -14, -18); }
    void refreshBlur()
    {
        if (!mDialog->isVisible() || !mDialog->windowHandle() || !mDialog->windowHandle()->handle()) return;
        QPainterPath path; path.addRoundedRect(body(), Sequoia::SurfaceRadius, Sequoia::SurfaceRadius);
        const QRegion region = mDialog->palette().color(QPalette::Window).alpha() < 255
            ? QRegion(path.toFillPolygon().toPolygon()) : QRegion(QRect(-1, -1, 1, 1));
        KWindowEffects::enableBlurBehind(mDialog->windowHandle(), true, region);
    }
    QDialog *mDialog;
    QWidget *mHeader;
    QLabel *mTitle;
};
}
