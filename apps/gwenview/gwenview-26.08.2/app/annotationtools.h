// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QDockWidget>
#include <QBoxLayout>
#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

namespace Gwenview {
// Keep kImageAnnotator's original title widget and docking behavior. Its custom
// title layout does not account for the desktop style's outer dock margins.
class AnnotationTools : public QObject
{
public:
    explicit AnnotationTools(QDockWidget *dock) : QObject(dock), mDock(dock)
    {
        setObjectName(QStringLiteral("annotationToolsPolish"));
        dock->installEventFilter(this);
        connect(dock, &QDockWidget::featuresChanged, this, [this] { refresh(); });
        refresh();
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::Resize || event->type() == QEvent::StyleChange
            || event->type() == QEvent::PaletteChange || event->type() == QEvent::LayoutRequest) {
            if (!mPending) {
                mPending = true;
                QTimer::singleShot(0, this, [this] { mPending = false; refresh(); });
            }
        }
        return false;
    }
private:
    void refresh()
    {
        mDock->setContentsMargins(0, 0, 0, 0);
        auto content = mDock->widget();
        if (content) {
            content->setContentsMargins(8, 8, 8, 8);
            for (auto button : content->findChildren<QToolButton *>()) button->setIconSize(QSize(18, 18));
            const bool horizontal = mDock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
            const auto crossAlignment = horizontal ? Qt::AlignVCenter : Qt::AlignHCenter;
            if (auto box = qobject_cast<QBoxLayout *>(content->layout())) {
                box->setAlignment((horizontal ? Qt::AlignLeft : Qt::AlignTop) | crossAlignment);
                for (int i = 0; i < box->count(); ++i) box->itemAt(i)->setAlignment(crossAlignment);
                box->activate();
            }
            // kImageAnnotator's flow layouts always place rows at their top
            // left and do not implement alignment. Keep their sizing/wrapping,
            // then align each row on the toolbar's cross axis.
            for (auto host : content->findChildren<QWidget *>()) {
                auto layout = host->layout();
                if (!layout || QByteArray(layout->metaObject()->className()) != "QLayout"
                    || !QByteArray(host->metaObject()->className()).startsWith("kImageAnnotator::")) continue;
                host->installEventFilter(this);
                if (!host->isVisible()) continue;
                layout->setGeometry(host->contentsRect());
                QList<QList<QWidget *>> rows;
                QRect bounds;
                for (int i = 0; i < layout->count(); ++i) {
                    auto item = layout->itemAt(i);
                    auto child = item->widget();
                    if (!child || child->isHidden()) continue;
                    if (rows.isEmpty() || rows.last().first()->y() != child->y()) rows.append(QList<QWidget *>());
                    rows.last().append(child);
                    bounds = bounds.united(child->geometry());
                }
                const int verticalOffset = horizontal ? host->contentsRect().center().y() - bounds.center().y() : 0;
                for (const auto &row : rows) {
                    QRect rowBounds;
                    for (auto child : row) rowBounds = rowBounds.united(child->geometry());
                    const int dx = horizontal ? 0 : host->contentsRect().center().x() - rowBounds.center().x();
                    for (auto child : row)
                        child->move(child->x() + dx, rowBounds.center().y() - (child->height() - 1) / 2 + verticalOffset);
                }
            }
        }
        auto handle = mDock->titleBarWidget();
        if (!handle || !handle->layout()) return;
        handle->layout()->setContentsMargins(12, 12, 12, 12);
        auto label = handle->findChild<QLabel *>();
        if (!label) return;
        const bool vertical = mDock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
        const QSize size = vertical ? QSize(4, 18) : QSize(18, 4);
        const qreal scale = mDock->devicePixelRatioF();
        QColor ink = mDock->palette().color(QPalette::WindowText); ink.setAlphaF(0.30);
        if (mGripKey == label->pixmap().cacheKey() && mGripSize == size && mGripColor == ink && mGripScale == scale) return;
        QPixmap grip(size * scale); grip.setDevicePixelRatio(scale); grip.fill(Qt::transparent);
        QPainter painter(&grip); painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen); painter.setBrush(ink);
        painter.drawRoundedRect(QRectF(QPointF(), QSizeF(size)), 2, 2); painter.end();
        label->setPixmap(grip); label->setFixedSize(size);
        label->setAlignment(Qt::AlignCenter);
        mGripKey = grip.cacheKey(); mGripSize = size; mGripColor = ink; mGripScale = scale;
    }
    QDockWidget *mDock;
    bool mPending = false;
    qint64 mGripKey = 0;
    QSize mGripSize;
    QColor mGripColor;
    qreal mGripScale = 0;
};
}
