// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <lib/sequoiadesign.h>
#include <lib/stylesheetutils.h>
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QWidgetAction>

namespace Gwenview {
// Keep KLanguageButton's actions and selection signals. Only replace the
// unbounded menu presentation with a keyboard/wheel-scrollable native list.
class LanguageMenuList : public QObject
{
public:
    explicit LanguageMenuList(QPushButton *button) : QObject(button), mButton(button), mOriginal(button->menu())
    {
        setObjectName(QStringLiteral("languageMenuList"));
        mMenu = new QMenu(button); mMenu->setObjectName(QStringLiteral("gwenviewLanguageMenu"));
        mList = new QListWidget(mMenu); mList->setObjectName(QStringLiteral("gwenviewLanguageList"));
        mList->setFrameShape(QFrame::NoFrame);
        mList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        mList->setUniformItemSizes(true);
        auto action = new QWidgetAction(mMenu); action->setDefaultWidget(mList); mMenu->addAction(action);
        button->setMenu(mMenu);
        connect(mMenu, &QMenu::aboutToShow, this, [this] { populate(); });
        connect(mList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) { activate(item); });
        connect(mList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
            if (item) mList->scrollToItem(item, QAbstractItemView::EnsureVisible);
        });
        mMenu->setFocusProxy(mList);
        mMenu->installEventFilter(this); mList->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            auto key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Space) {
                activate(mList->currentItem()); return true;
            }
            if (key->key() == Qt::Key_Escape) { mMenu->close(); mButton->setFocus(); return true; }
            if (object == mMenu) { QApplication::sendEvent(mList, event); return true; }
        }
        return false;
    }
private:
    void populate()
    {
        mList->clear(); mActions.clear();
        const QPalette pal = mMenu->palette();
        mList->setStyleSheet(QStringLiteral("QListWidget { background: transparent; border: none; } QListWidget::item { min-height: 32px; padding: 0 10px; border-radius: 12px; } QListWidget::item:selected { background: %1; color: palette(text); } QListWidget::item:hover:!selected { background: %2; }")
            .arg(StyleSheetUtils::rgba(Sequoia::alpha(pal.color(QPalette::Highlight), 0.16)),
                 StyleSheetUtils::rgba(Sequoia::alpha(pal.color(QPalette::WindowText), 0.08))));
        int selected = 0, width = mButton->width();
        const QString current = mButton->text().remove('&');
        for (auto action : mOriginal->actions()) {
            if (action->isSeparator() || !action->isVisible()) continue;
            const QString label = action->text().remove('&');
            auto item = new QListWidgetItem(action->icon(), label, mList);
            if (!action->isEnabled()) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            if (label == current || action->isChecked()) selected = mList->count() - 1;
            width = qMax(width, mList->fontMetrics().horizontalAdvance(label) + 48);
            mActions << action;
        }
        const QSize available = mButton->screen()->availableGeometry().size();
        mList->setFixedSize(qMin(width, available.width() - 80),
                           qMin(qMin(360, available.height() * 3 / 5), qMax(32, mList->count() * 32)));
        mList->setCurrentRow(selected);
        QTimer::singleShot(0, mList, [this] { mList->setFocus(); mList->scrollToItem(mList->currentItem(), QAbstractItemView::PositionAtCenter); });
    }
    void activate(QListWidgetItem *item)
    {
        if (!item || !(item->flags() & Qt::ItemIsEnabled)) return;
        const auto action = mActions.value(mList->row(item));
        mMenu->close();
        if (action) action->trigger();
    }
    QPushButton *mButton;
    QMenu *mOriginal, *mMenu;
    QListWidget *mList;
    QList<QPointer<QAction>> mActions;
};
}
