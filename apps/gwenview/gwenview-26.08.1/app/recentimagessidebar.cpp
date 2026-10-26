// SPDX-License-Identifier: GPL-2.0-or-later
#include "recentimagessidebar.h"
#include "gvcore.h"
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QStyledItemDelegate>
#include <KFilePlacesModel>
#include <KLocalizedString>
#include <QLabel>
#include <QToolButton>
#include <QActionGroup>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QMenu>
#include <QMimeDatabase>
#include <QPersistentModelIndex>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

namespace Gwenview {
class RecentImageFilter : public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    void refresh() { invalidateFilter(); }
    QVariant data(const QModelIndex &index, int role) const override {
        const auto url = QSortFilterProxyModel::data(index, KFilePlacesModel::UrlRole).toUrl();
        if (role == Qt::DisplayRole) return url.fileName();
        if (role == Qt::ToolTipRole) return url.toDisplayString(QUrl::PreferLocalFile);
        return QSortFilterProxyModel::data(index, role);
    }
protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override {
        if (!GwenviewConfig::historyEnabled()) return false;
        const auto url = sourceModel()->index(row, 0, parent).data(KFilePlacesModel::UrlRole).toUrl();
        const auto kind = MimeTypeUtils::mimeTypeKind(QMimeDatabase().mimeTypeForUrl(url).name());
        return kind == MimeTypeUtils::KIND_RASTER_IMAGE || kind == MimeTypeUtils::KIND_SVG_IMAGE;
    }
};
class RecentImageHelper : public AbstractThumbnailViewHelper {
public:
    using AbstractThumbnailViewHelper::AbstractThumbnailViewHelper;
    void showContextMenu(QWidget *) override {}
    void showMenuForUrlDroppedOnViewport(QWidget *, const QList<QUrl> &) override {}
    void showMenuForUrlDroppedOnDir(QWidget *, const QList<QUrl> &, const QUrl &) override {}
};

// Keep native thumbnail loading, activation, and history, but give the sidebar
// a responsive grid rather than the browser's full-image-and-caption layout.
class RecentPreviewView : public ThumbnailView {
public:
    explicit RecentPreviewView(QWidget *parent) : ThumbnailView(parent)
    {
        setFlow(QListView::LeftToRight);
        setWrapping(true);
        setMovement(QListView::Static);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setMouseTracking(true);
        setThumbnailScaleMode(ThumbnailView::ScaleToSquare);
        updateGrid();
    }
    void setListMode(bool enabled)
    {
        setProperty("listMode", enabled);
        setFlow(enabled ? QListView::TopToBottom : QListView::LeftToRight);
        setWrapping(!enabled);
        updateGrid();
        scheduleDelayedItemsLayout();
        scrollToSelectedIndex();
    }
protected:
    void resizeEvent(QResizeEvent *event) override
    {
        ThumbnailView::resizeEvent(event);
        updateGrid();
    }
private:
    void updateGrid()
    {
        // Leave a little room for QListView's inclusive right-edge calculation.
        const int width = qMax(1, viewport()->width() - 2);
        if (property("listMode").toBool()) {
            setGridSize(QSize(width, 64));
            setThumbnailWidth(52);
            return;
        }
        const int columns = qMax(1, width / 76);
        const int cell = qMax(1, width / columns);
        setGridSize(QSize(cell, cell));
        setThumbnailWidth(qMax(48, cell - 8));
    }
};

class RecentPreviewDelegate : public QStyledItemDelegate {
public:
    explicit RecentPreviewDelegate(ThumbnailView *view) : QStyledItemDelegate(view), mView(view) {}

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return mView->gridSize();
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const bool list = mView->property("listMode").toBool();
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const bool focused = option.state & QStyle::State_HasFocus;
        const QRectF row = QRectF(option.rect).adjusted(2, 2, -2, -2);
        const QRectF tile = list ? QRectF(option.rect.x() + 6, option.rect.y() + 6, 52, 52)
                                : QRectF(option.rect).adjusted(4, 4, -4, -4);
        if (tile.isEmpty()) return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (list && (selected || hovered)) {
            QColor fill = option.palette.color(QPalette::Highlight); fill.setAlpha(selected ? 30 : 16);
            painter->setBrush(fill); painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(row, 10, 10);
        }
        const QPixmap preview = mView->thumbnailForIndex(index);
        QPainterPath clip; clip.addRoundedRect(tile, 7, 7);
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->setClipPath(clip);
        painter->fillRect(tile, option.palette.brush(QPalette::AlternateBase));
        if (!preview.isNull()) {
            const qreal side = qMin(preview.width(), preview.height());
            const QRectF source((preview.width() - side) / 2, (preview.height() - side) / 2, side, side);
            painter->drawPixmap(tile, preview, source);
        }
        if (hovered) {
            QColor tint = option.palette.color(QPalette::Highlight); tint.setAlpha(24);
            painter->fillRect(tile, tint);
        }
        painter->restore();
        if (list) {
            const QRect textRect = option.rect.adjusted(70, 8, -10, -8);
            QFont nameFont = option.font; nameFont.setWeight(QFont::DemiBold);
            painter->setFont(nameFont); painter->setPen(option.palette.color(QPalette::Text));
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop,
                QFontMetrics(nameFont).elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, textRect.width()));
            const QUrl url = index.data(KFilePlacesModel::UrlRole).toUrl();
            const QString folder = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toDisplayString(QUrl::PreferLocalFile);
            QFont detailFont = option.font; detailFont.setPointSizeF(detailFont.pointSizeF() * 0.9);
            painter->setFont(detailFont);
            QColor muted = option.palette.color(QPalette::Text); muted.setAlpha(165); painter->setPen(muted);
            painter->drawText(textRect.adjusted(0, 25, 0, 0), Qt::AlignLeft | Qt::AlignTop,
                QFontMetrics(detailFont).elidedText(folder, Qt::ElideLeft, textRect.width()));
        }
        if (selected || hovered || focused) {
            QColor accent = option.palette.color(QPalette::Highlight);
            if (!selected && !focused) accent.setAlpha(100);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(accent, selected || focused ? 2 : 1));
            painter->drawRoundedRect(list ? row : tile.adjusted(-1, -1, 1, 1), list ? 10 : 8, list ? 10 : 8);
        }
        if (focused) {
            painter->setPen(QPen(option.palette.color(QPalette::Highlight), 1, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(QRectF(option.rect).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
        }
        painter->restore();
    }
private:
    ThumbnailView *mView;
};

// Draw the overflow glyph with the control palette so it stays legible even
// when an installed icon theme supplies a fixed dark monochrome asset.
class RecentMenuButton : public QToolButton {
public:
    using QToolButton::QToolButton;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown() || hasFocus()) {
            QColor fill = palette().color(QPalette::Active, QPalette::Highlight);
            fill.setAlpha(isDown() ? 35 : 20);
            painter.setBrush(fill);
            painter.setPen(hasFocus() ? QPen(palette().color(QPalette::Active, QPalette::Highlight), 1.5) : QPen(Qt::NoPen));
            painter.drawRoundedRect(QRectF(rect()).adjusted(2, 2, -2, -2), 8, 8);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().color(QPalette::ButtonText));
        const QPointF center = QRectF(rect()).center();
        for (int offset : {-5, 0, 5}) painter.drawEllipse(center + QPointF(0, offset), 1.4, 1.4);
    }
};

RecentImagesSidebar::RecentImagesSidebar(GvCore *core, QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("recentImagesSidebar"));
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto heading = new QLabel(i18n("Recent images"), this);
    heading->setObjectName(QStringLiteral("recentImagesHeading"));
    auto font = heading->font(); font.setBold(true); heading->setFont(font);
    auto header = new QHBoxLayout;
    header->setSpacing(6); header->addWidget(heading);
    auto count = new QLabel(this);
    count->setObjectName(QStringLiteral("recentImagesCount"));
    auto countFont = count->font(); countFont.setPointSizeF(countFont.pointSizeF() * 0.9);
    count->setFont(countFont); header->addWidget(count); header->addStretch();
    auto more = new RecentMenuButton(this);
    more->setObjectName(QStringLiteral("recentImagesMenuButton"));
    more->setToolTip(i18n("Recent image options")); more->setAccessibleName(more->toolTip());
    more->setAutoRaise(true); more->setFixedSize(28, 28);
    more->setPopupMode(QToolButton::InstantPopup);
    header->addWidget(more); layout->addLayout(header);
    auto filter = new RecentImageFilter(this);
    filter->setSourceModel(core->recentFilesModel());
    auto view = new RecentPreviewView(this);
    view->setObjectName(QStringLiteral("recentImagesView"));
    view->setAccessibleName(i18n("Recent images"));
    view->setModel(filter);
    view->setThumbnailViewHelper(new RecentImageHelper(view));
    view->setItemDelegate(new RecentPreviewDelegate(view));
    view->setCreateThumbnailsForRemoteUrls(false);
    auto provider = new ThumbnailProvider;
    provider->setParent(view);
    view->setThumbnailProvider(provider);
    view->setFrameShape(QFrame::NoFrame);
    view->setAutoFillBackground(false);
    view->viewport()->setAutoFillBackground(false);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setFocusPolicy(Qt::StrongFocus);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(view, 1);
    auto empty = new QLabel(this);
    empty->setObjectName(QStringLiteral("recentImagesEmpty"));
    empty->setWordWrap(true); empty->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(empty, 1);
    auto refresh = [filter, view, empty, count] {
        count->setText(QString::number(filter->rowCount()));
        count->setAccessibleName(i18np("1 recent image", "%1 recent images", filter->rowCount()));
        const bool enabled = GwenviewConfig::historyEnabled();
        const bool hasImages = enabled && filter->rowCount() > 0;
        view->setVisible(hasImages); empty->setVisible(!hasImages);
        empty->setText(enabled ? i18n("Images you open will appear here.") : i18n("Image history is disabled in Settings."));
    };
    connect(filter, &QAbstractItemModel::rowsInserted, this, refresh);
    connect(filter, &QAbstractItemModel::rowsRemoved, this, refresh);
    connect(filter, &QAbstractItemModel::modelReset, this, refresh);
    connect(GwenviewConfig::self(), &KCoreConfigSkeleton::configChanged, this, [filter, refresh] { filter->refresh(); refresh(); });
    connect(view, &ThumbnailView::indexActivated, this, [this](const QModelIndex &index) {
        if (index.isValid()) Q_EMIT urlSelected(index.data(KFilePlacesModel::UrlRole).toUrl());
    });
    auto menu = new QMenu(more);
    menu->setObjectName(QStringLiteral("recentImagesMenu"));
    more->setMenu(menu);
    auto viewGroup = new QActionGroup(menu);
    auto grid = menu->addAction(i18n("Thumbnail grid"));
    grid->setObjectName(QStringLiteral("recentGridView"));
    auto list = menu->addAction(i18n("File names and folders"));
    list->setObjectName(QStringLiteral("recentListView"));
    for (auto action : {grid, list}) { action->setCheckable(true); viewGroup->addAction(action); }
    KConfigGroup settings(KSharedConfig::openConfig(), QStringLiteral("SideBar"));
    const bool listMode = settings.readEntry("RecentImagesListView", false);
    grid->setChecked(!listMode); list->setChecked(listMode); view->setListMode(listMode);
    connect(viewGroup, &QActionGroup::triggered, this, [view, list](QAction *action) {
        const bool enabled = action == list;
        view->setListMode(enabled);
        KConfigGroup settings(KSharedConfig::openConfig(), QStringLiteral("SideBar"));
        settings.writeEntry("RecentImagesListView", enabled); settings.sync();
    });
    menu->addSeparator();
    auto remove = menu->addAction(i18n("Remove selected from recents"));
    remove->setObjectName(QStringLiteral("removeRecentImage"));
    auto clear = menu->addAction(i18n("Clear recent images"));
    clear->setObjectName(QStringLiteral("clearRecentImages"));
    clear->setToolTip(i18n("Clear the history without deleting any image files."));
    connect(menu, &QMenu::aboutToShow, this, [view, filter, remove, clear] {
        remove->setEnabled(view->currentIndex().isValid());
        clear->setEnabled(filter->rowCount() > 0);
    });
    connect(remove, &QAction::triggered, this, [this, view, filter] {
        const auto index = view->currentIndex();
        if (!index.isValid()) return;
        const auto url = index.data(KFilePlacesModel::UrlRole).toUrl();
        filter->sourceModel()->removeRow(filter->mapToSource(index).row());
        Q_EMIT recentFileRemoved(url);
    });
    connect(clear, &QAction::triggered, this, [this, filter] {
        filter->sourceModel()->removeRows(0, filter->sourceModel()->rowCount());
        Q_EMIT recentFilesCleared();
    });
    connect(view, &QWidget::customContextMenuRequested, this, [view, menu](const QPoint &pos) {
        view->setCurrentIndex(view->indexAt(pos));
        menu->exec(view->viewport()->mapToGlobal(pos));
    });
    refresh();
}
}
#include "moc_recentimagessidebar.cpp"
