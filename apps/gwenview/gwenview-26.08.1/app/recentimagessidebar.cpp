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
        const QRectF tile = QRectF(option.rect).adjusted(4, 4, -4, -4);
        if (tile.isEmpty()) return;
        const QPixmap preview = mView->thumbnailForIndex(index);
        QPainterPath clip;
        clip.addRoundedRect(tile, 7, 7);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->setClipPath(clip);
        painter->fillRect(tile, option.palette.brush(QPalette::AlternateBase));
        if (!preview.isNull()) {
            // Center-crop even small source images that the thumbnail cache
            // returns at their original aspect ratio. Never stretch the image.
            const qreal side = qMin(preview.width(), preview.height());
            const QRectF source((preview.width() - side) / 2, (preview.height() - side) / 2, side, side);
            painter->drawPixmap(tile, preview, source);
        }
        if (option.state & QStyle::State_MouseOver)
            painter->fillRect(tile, QColor(255, 255, 255, 24));
        painter->restore();
        if (option.state & (QStyle::State_Selected | QStyle::State_HasFocus)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(option.palette.color(QPalette::Highlight), 2));
            painter->drawRoundedRect(tile.adjusted(-1, -1, 1, 1), 8, 8);
            painter->restore();
        }
    }
private:
    ThumbnailView *mView;
};

RecentImagesSidebar::RecentImagesSidebar(GvCore *core, QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("recentImagesSidebar"));
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto heading = new QLabel(i18n("Recent images"), this);
    heading->setObjectName(QStringLiteral("recentImagesHeading"));
    auto font = heading->font(); font.setBold(true); heading->setFont(font);
    layout->addWidget(heading);
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
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(view, 1);
    auto empty = new QLabel(this);
    empty->setObjectName(QStringLiteral("recentImagesEmpty"));
    empty->setWordWrap(true); empty->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(empty, 1);
    auto refresh = [filter, view, empty] {
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
    connect(view, &QWidget::customContextMenuRequested, this, [this, view, filter](const QPoint &pos) {
        const QPersistentModelIndex index = view->indexAt(pos);
        const auto url = index.data(KFilePlacesModel::UrlRole).toUrl();
        QMenu menu(this);
        auto remove = menu.addAction(i18n("Remove from recent images"));
        remove->setEnabled(index.isValid());
        auto clear = menu.addAction(i18n("Clear recent images"));
        const auto chosen = menu.exec(view->viewport()->mapToGlobal(pos));
        if (chosen == remove && index.isValid()) {
            filter->sourceModel()->removeRow(filter->mapToSource(index).row());
            Q_EMIT recentFileRemoved(url);
        } else if (chosen == clear) {
            filter->sourceModel()->removeRows(0, filter->sourceModel()->rowCount());
            Q_EMIT recentFilesCleared();
        }
    });
    refresh();
}
}
#include "moc_recentimagessidebar.cpp"
