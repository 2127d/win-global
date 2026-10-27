// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "floatingsurface.h"
#include "fullscreencontrols.h"
#include "browsemainpage.h"

// Qt
#include <QActionGroup>
#include <QDropEvent>
#include <QMenu>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStyleOptionViewItem>
#include <QSlider>
#include <KDirLister>

// KF
#include <KActionCategory>
#include <KActionCollection>
#include <KActionMenu>
#include <KDirModel>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KIconLoader>
#include <KLocalizedString>
#include <KUrlMimeData>
#include <KUrlNavigator>

// Local
#include <fileoperations.h>
#include <filtercontroller.h>
#include <gvcore.h>
#include <lib/archiveutils.h>
#include <lib/document/documentfactory.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/semanticinfo/tagmodel.h>
#include <lib/sorting.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <mimetypeutils.h>
#include <ui_browsemainpage.h>
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "lib/semanticinfo/semanticinfodirmodel.h"
#endif

namespace Gwenview
{
class BrowseUrlNavigator : public KUrlNavigator
{
public:
    using KUrlNavigator::KUrlNavigator;
    QSize minimumSizeHint() const override
    {
        // KUrlNavigator can collapse breadcrumbs, but its preferred minimum
        // otherwise keeps the outer window as wide as the entire path.
        return QSize(80, KUrlNavigator::minimumSizeHint().height());
    }
};

// Extend the native delegate so thumbnail generation, editing, drag/drop,
// context actions and metadata remain Gwenview's own implementation.
class BrowsePreviewDelegate : public PreviewItemDelegate
{
public:
    explicit BrowsePreviewDelegate(ThumbnailView *view) : PreviewItemDelegate(view) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save(); painter->setRenderHint(QPainter::Antialiasing);
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const bool focused = option.state & QStyle::State_HasFocus;
        const QRectF card = QRectF(option.rect).adjusted(2, 2, -2, -2);
        QColor accent = option.palette.color(QPalette::Active, QPalette::Highlight);
        QColor fill = selected || hovered ? accent : option.palette.color(QPalette::WindowText);
        fill.setAlpha(selected ? 24 : hovered ? 12 : 4);
        QColor edge = selected || focused ? accent : option.palette.color(QPalette::WindowText);
        edge.setAlpha(selected || focused ? 170 : hovered ? 35 : 16);
        painter->setPen(QPen(edge, selected ? 1.5 : 1)); painter->setBrush(fill);
        painter->drawRoundedRect(card, 10, 10);
        painter->restore();
        auto native = option;
        native.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus);
        PreviewItemDelegate::paint(painter, native, index);
        if (focused) {
            painter->save(); painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush); painter->setPen(QPen(accent, 1, Qt::DashLine));
            painter->drawRoundedRect(card.adjusted(2, 2, -2, -2), 8, 8); painter->restore();
        }
    }
};

class BrowseGallerySurface : public QWidget
{
public:
    using QWidget::QWidget;
    void setFloating(bool floating) { mFloating = floating; update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!mFloating) return;
        QPainter painter(this);
        paintFloatingPanel(painter, QRectF(rect()).adjusted(FloatingPanelInset, FloatingPanelInset,
                                                         -FloatingPanelInset, -FloatingPanelInset), palette());
    }
private:
    bool mFloating = false;
};

inline Sorting::Enum sortingFromSortAction(const QAction *action)
{
    Q_ASSERT(action);
    return Sorting::Enum(action->data().toInt());
}

struct BrowseMainPagePrivate : public Ui_BrowseMainPage {
    BrowseMainPage *q = nullptr;
    GvCore *mGvCore = nullptr;
    QTimer *mPresentationTimer = nullptr;
    KFilePlacesModel *mFilePlacesModel = nullptr;
    KUrlNavigator *mUrlNavigator = nullptr;
    SortedDirModel *mDirModel = nullptr;
    int mDocumentCountImages;
    int mDocumentCountVideos;
    KFileItemList *mSelectedMediaItems = nullptr;
    KActionCollection *mActionCollection = nullptr;
    FilterController *mFilterController = nullptr;
    QActionGroup *mSortAction = nullptr;
    KToggleAction *mSortDescendingAction = nullptr;
    KToggleAction *mShowHiddenFilesAction = nullptr;
    QActionGroup *mThumbnailDetailsActionGroup = nullptr;
    PreviewItemDelegate *mDelegate = nullptr;
    bool mFullScreen = false;
    QLabel *mFolderTitle = nullptr;
    QLabel *mEmptyState = nullptr;
    QLabel *mSizeCaption = nullptr;
    QToolButton *mSortButton = nullptr;
    QToolButton *mDetailsButton = nullptr;
    QScrollArea *mFilterScroll = nullptr;
    int mPreferredThumbnailWidth = 128;
    bool mResizingThumbnails = false;
    bool mLoading = false;
    QWidget *mHeader = nullptr;
    FullScreenSurface *mChrome = nullptr;
    BrowseGallerySurface *mGallery = nullptr;
    QWidget *mFullScreenNavigation = nullptr;
    FullScreenButton *mExitFullScreenButton = nullptr;
    QAction *mHistoryBack = nullptr;
    QAction *mHistoryForward = nullptr;

    void setupWidgets()
    {
        setupUi(q);
        // The panel has the viewer's outer inset plus padding for its controls.
        q->layout()->setContentsMargins(FloatingPanelPadding, FloatingPanelPadding, FloatingPanelPadding, FloatingPanelPadding);
        q->layout()->setSpacing(8);
        q->setAutoFillBackground(false);

        mStatusBarContainer->layout()->setContentsMargins(0, 6, 0, 0);
        mStatusBarContainer->layout()->setSpacing(8);
        mThumbnailSlider->setMinimumWidth(100);
        mThumbnailSlider->setMaximumWidth(180);
        mThumbnailSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        mThumbnailSlider->slider()->setAccessibleName(i18n("Thumbnail size"));
        for (auto button : mThumbnailSlider->findChildren<QToolButton *>()) {
            const QString iconName = button->icon().name();
            button->setProperty("browseIconName", iconName);
            button->setToolTip(iconName == QStringLiteral("zoom-in") ? i18n("Larger thumbnails") : i18n("Smaller thumbnails"));
            button->setAccessibleName(button->toolTip());
            button->setFocusPolicy(Qt::TabFocus);
        }
        mSizeCaption = new QLabel(i18n("Thumbnails"), mStatusBarContainer);
        mSizeCaption->setForegroundRole(QPalette::PlaceholderText);
        static_cast<QHBoxLayout *>(mStatusBarContainer->layout())->insertWidget(3, mSizeCaption);

        auto header = new QWidget(q); header->setObjectName(QStringLiteral("browseHeader"));
        mHeader = header;
        auto headerLayout = new QHBoxLayout(header); headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(6);
        mFolderTitle = new QLabel(header); mFolderTitle->setObjectName(QStringLiteral("browseFolderTitle"));
        QFont titleFont = mFolderTitle->font(); titleFont.setBold(true); titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
        mFolderTitle->setFont(titleFont); mFolderTitle->setTextFormat(Qt::PlainText);
        mFolderTitle->setMinimumWidth(0); mFolderTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        headerLayout->addWidget(mFolderTitle, 1);
        mSortButton = new QToolButton(header); mSortButton->setObjectName(QStringLiteral("browseSortButton"));
        mDetailsButton = new QToolButton(header); mDetailsButton->setObjectName(QStringLiteral("browseDetailsButton"));
        // Moving the existing button preserves its filter menu and shortcuts.
        mStatusBarContainer->layout()->removeWidget(mAddFilterButton);
        for (auto button : {mSortButton, mDetailsButton, static_cast<QToolButton *>(mAddFilterButton)}) {
            button->setPopupMode(QToolButton::InstantPopup); button->setAutoRaise(true);
            button->setMinimumHeight(34); button->setIconSize(QSize(18, 18));
            button->setFocusPolicy(Qt::StrongFocus);
            headerLayout->addWidget(button);
        }
        mAddFilterButton->setText(i18n("Filters")); mAddFilterButton->setToolTip(i18n("Filter images by name, date, rating, tags or dimensions"));
        mAddFilterButton->setAccessibleName(i18n("Add filter"));
        static_cast<QVBoxLayout *>(q->layout())->insertWidget(0, header);

        // mThumbnailView
        mThumbnailView->setModel(mDirModel);
        mThumbnailView->setFrameShape(QFrame::NoFrame);
        mThumbnailView->setAutoFillBackground(false);
        mThumbnailView->viewport()->setAutoFillBackground(false);
        mFilterFrame->setFrameShape(QFrame::NoFrame);

        mThumbnailView->setSpacing(5);
        mThumbnailView->setMouseTracking(true);
        mThumbnailView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mThumbnailView->viewport()->installEventFilter(q);
        mDelegate = new BrowsePreviewDelegate(mThumbnailView);
        mThumbnailView->setItemDelegate(mDelegate);
        mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mEmptyState = new QLabel(mThumbnailView->viewport());
        mEmptyState->setObjectName(QStringLiteral("browseEmptyState"));
        mEmptyState->setAlignment(Qt::AlignCenter); mEmptyState->setWordWrap(true);
        mEmptyState->setForegroundRole(QPalette::PlaceholderText);
        mEmptyState->setAttribute(Qt::WA_TransparentForMouseEvents);
        mEmptyState->hide();
        mFilterScroll = new QScrollArea(q); mFilterScroll->setObjectName(QStringLiteral("browseFilterScroll"));
        mFilterScroll->setFrameShape(QFrame::NoFrame); mFilterScroll->setWidgetResizable(true);
        mFilterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        mFilterScroll->setAutoFillBackground(false); mFilterScroll->viewport()->setAutoFillBackground(false);
        // Active filters sit below the navigation, above the thumbnail grid.
        q->layout()->removeWidget(mFilterFrame);
        mFilterScroll->setWidget(mFilterFrame); mFilterFrame->setAutoFillBackground(false);
        mFilterFrame->installEventFilter(q);
        static_cast<QVBoxLayout *>(q->layout())->insertWidget(2, mFilterScroll);
        mFilterScroll->hide();

        // mUrlNavigator (use stupid layouting code because KUrlNavigator ctor
        // can't be used directly from Designer)
        mFilePlacesModel = new KFilePlacesModel(q);
        mUrlNavigator = new BrowseUrlNavigator(mFilePlacesModel, QUrl(), mUrlNavigatorContainer);
        mUrlNavigatorContainer->setAutoFillBackground(false);
        mUrlNavigatorContainer->setBackgroundRole(QPalette::Mid);
        mUrlNavigatorContainer->setContentsMargins(4, 3, 4, 3);
        mUrlNavigator->setMinimumWidth(0);
        mUrlNavigator->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        mUrlNavigator->setAccessibleName(i18n("Folder location"));
        QObject::connect(mUrlNavigator, &KUrlNavigator::urlChanged, q, [this] {
            schedulePresentationUpdate();
            QTimer::singleShot(0, q, [this] { updateControlIcons(); });
        });
        auto layout = new QVBoxLayout(mUrlNavigatorContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(mUrlNavigator);
        QObject::connect(mUrlNavigator, SIGNAL(urlsDropped(QUrl, QDropEvent *)), q, SLOT(slotUrlsDropped(QUrl, QDropEvent *)));

        // Reuse the normal controls inside two floating surfaces in fullscreen.
        // The breadcrumb keeps its own row when the sidebar narrows the pane.
        mChrome = new FullScreenSurface(q);
        mChrome->setObjectName(QStringLiteral("fullscreenBrowseChrome"));
        mChrome->setFloating(false);
        mChrome->setHorizontalInset(FloatingPanelInset);
        mChrome->setFloatingParts({mHeader, mUrlNavigatorContainer});
        auto chromeLayout = new QVBoxLayout(mChrome);
        chromeLayout->setContentsMargins(0, 0, 0, 0); chromeLayout->setSpacing(8);
        q->layout()->removeWidget(mHeader);
        chromeLayout->addWidget(mHeader);
        q->layout()->removeItem(horizontalLayout_2);
        chromeLayout->addLayout(horizontalLayout_2);
        delete mFullScreenToolBar; mFullScreenToolBar = nullptr;
        delete mFullScreenToolBar2; mFullScreenToolBar2 = nullptr;

        mGallery = new BrowseGallerySurface(q);
        mGallery->setObjectName(QStringLiteral("fullscreenBrowseGallery"));
        auto galleryLayout = new QVBoxLayout(mGallery);
        galleryLayout->setContentsMargins(0, 0, 0, 0); galleryLayout->setSpacing(8);
        for (auto widget : {static_cast<QWidget *>(mFilterScroll), static_cast<QWidget *>(mThumbnailView), mStatusBarContainer}) {
            q->layout()->removeWidget(widget);
            galleryLayout->addWidget(widget, widget == mThumbnailView ? 1 : 0);
        }
        static_cast<QVBoxLayout *>(q->layout())->addWidget(mChrome);
        static_cast<QVBoxLayout *>(q->layout())->addWidget(mGallery, 1);

        QObject::connect(mThumbnailSlider, &ZoomSlider::valueChanged, q, [this](int value) {
            mPreferredThumbnailWidth = value; fitThumbnails();
        });
        QObject::connect(mThumbnailView, &ThumbnailView::thumbnailWidthChanged, q, [this](int value) {
            if (mResizingThumbnails) return;
            mPreferredThumbnailWidth = value;
            const QSignalBlocker blocker(mThumbnailSlider); mThumbnailSlider->setValue(value);
            fitThumbnails();
        });
        mDocumentCountLabel->setContentsMargins(4, 0, 4, 0);
        mDocumentCountLabel->setMinimumWidth(0);
        mDocumentCountLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        static_cast<QHBoxLayout *>(mStatusBarContainer->layout())->setStretchFactor(mDocumentCountLabel, 1);
    }

    void fitThumbnails()
    {
        if (mResizingThumbnails) return;
        mResizingThumbnails = true;
        // Preserve the user's preferred size while temporarily fitting one tile.
        const int availableWidth = qMax(32, mThumbnailView->viewport()->width() - 40);
        const int detailsHeight = qMax(0, mThumbnailView->gridSize().height() - mThumbnailView->thumbnailSize().height());
        const int availableHeight = qMax(32, mThumbnailView->viewport()->height() - detailsHeight - 2 * mThumbnailView->spacing());
        const int widthForHeight = qMax(32, qFloor(availableHeight * mThumbnailView->thumbnailAspectRatio()));
        mThumbnailView->setThumbnailWidth(qMin(mPreferredThumbnailWidth, qMin(availableWidth, widthForHeight)));
        mResizingThumbnails = false;
    }

    void schedulePresentationUpdate()
    {
        // Folder rows and viewport changes often arrive together. Lay out the
        // final state once instead of queuing one callback for every signal.
        if (!mPresentationTimer->isActive()) mPresentationTimer->start();
    }

    void updatePresentation()
    {
        if (!mUrlNavigator) return;
        QString title = mUrlNavigator->locationUrl().adjusted(QUrl::StripTrailingSlash).fileName();
        if (title.isEmpty()) title = mUrlNavigator->locationUrl().toDisplayString();
        if (title.isEmpty()) title = i18n("Browse images");
        mFolderTitle->setText(mFolderTitle->fontMetrics().elidedText(title, Qt::ElideMiddle, mFolderTitle->width()));
        mFolderTitle->setToolTip(mUrlNavigator->locationUrl().toDisplayString());
        mFolderTitle->setAccessibleName(title);
        if (mHistoryBack) {
            mHistoryBack->setEnabled(mUrlNavigator->historyIndex() + 1 < mUrlNavigator->historySize());
            mHistoryForward->setEnabled(mUrlNavigator->historyIndex() > 0);
        }
        const bool stacked = mFullScreen && q->width() < 760;
        horizontalLayout_2->setDirection(stacked ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
        if (mFullScreenNavigation) horizontalLayout_2->setAlignment(mFullScreenNavigation, Qt::AlignLeft);
        const bool compact = q->width() < (mFullScreen ? 640 : 560);
        for (auto button : {mSortButton, mDetailsButton, static_cast<QToolButton *>(mAddFilterButton)})
            button->setToolButtonStyle(compact ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
        mSizeCaption->setVisible(q->width() >= 460);
        mThumbnailSlider->setMaximumWidth(compact ? 120 : 180);
        const int filterBudget = mFullScreen ? (q->height() - mChrome->height() - 210) : q->height() / 3;
        mFilterScroll->setMaximumHeight(qBound(60, filterBudget, 170));
        mFilterScroll->setVisible(!mFilterFrame->isHidden());
        mEmptyState->setGeometry(mThumbnailView->viewport()->rect().adjusted(12, 12, -12, -12));
        mEmptyState->setText(mLoading ? i18n("Loading folder…")
            : !mFilterFrame->isHidden() ? i18n("No matching items\nTry changing or removing your filters.")
            : i18n("No images or folders to show\nChoose another folder using the location bar."));
        mEmptyState->setVisible(mDirModel->rowCount() == 0);
        fitThumbnails();
    }

    void updateControlIcons()
    {
        // Use theme assets as masks so symbolic icons work in both palettes.
        QList<QPair<QToolButton *, QString>> icons = {
            {mSortButton, QStringLiteral("view-sort")}, {mDetailsButton, QStringLiteral("view-list-details")},
            {mAddFilterButton, QStringLiteral("view-filter")}};
        for (auto button : mThumbnailSlider->findChildren<QToolButton *>())
            icons.append({button, button->property("browseIconName").toString()});
        // KUrlNavigator's places/home glyphs must also remain legible against
        // the fullscreen surface. Keep their original assets for palette changes.
        for (auto button : mUrlNavigator->findChildren<QAbstractButton *>()) {
            QIcon source = button->icon();
            if (source.isNull()) continue;
            if (!source.name().isEmpty() || !button->property("browseOriginalIcon").isValid())
                button->setProperty("browseOriginalIcon", source);
            else source = qvariant_cast<QIcon>(button->property("browseOriginalIcon"));
            QPixmap pix = source.pixmap(button->iconSize(), q->devicePixelRatioF());
            if (pix.isNull()) continue;
            QPainter painter(&pix); painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(pix.rect(), q->palette().color(QPalette::WindowText)); painter.end();
            button->setIcon(QIcon(pix));
        }
        for (const auto &item : icons) {
            QPixmap pix = QIcon::fromTheme(item.second, QIcon::fromTheme(QStringLiteral("configure"))).pixmap(QSize(18, 18), q->devicePixelRatioF());
            QPainter p(&pix); p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(pix.rect(), q->palette().color(QPalette::WindowText)); p.end();
            item.first->setIcon(QIcon(pix));
        }
    }

    QAction *thumbnailDetailAction(const QString &text, PreviewItemDelegate::ThumbnailDetail detail)
    {
        auto action = new QAction(q);
        action->setText(text);
        action->setCheckable(true);
        action->setChecked(GwenviewConfig::thumbnailDetails() & detail);
        action->setData(QVariant(detail));
        mThumbnailDetailsActionGroup->addAction(action);
        QObject::connect(action, SIGNAL(triggered(bool)), q, SLOT(updateThumbnailDetails()));
        return action;
    }

    void setupActions(KActionCollection *actionCollection)
    {
        auto view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);
        QAction *action = view->addAction("edit_location", q, &BrowseMainPage::editLocation);
        action->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
        actionCollection->setDefaultShortcut(action, Qt::Key_F6);

        auto sortActionMenu = view->add<KActionMenu>("sort_by");
        sortActionMenu->setText(i18nc("@action:inmenu", "Sort By"));
        sortActionMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-sort")));
        sortActionMenu->setPopupMode(QToolButton::InstantPopup);

        mSortAction = new QActionGroup(actionCollection);
        action = new QAction(i18nc("@addAction:inmenu", "Name"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Name));
        action = new QAction(i18nc("@addAction:inmenu", "Date"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Date));
        action = new QAction(i18nc("@addAction:inmenu", "Size"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Size));
        action = new QAction(i18nc("@addAction:inmenu", "Type"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Type));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        action = new QAction(i18nc("@addAction:inmenu", "Rating"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Rating));
#endif
        QObject::connect(mSortAction, SIGNAL(triggered(QAction *)), q, SLOT(updateSortOrder()));

        mSortDescendingAction = view->add<KToggleAction>(QStringLiteral("sort_desc"));
        mSortDescendingAction->setText(i18nc("@action:inmenu Sort", "Descending"));
        QObject::connect(mSortDescendingAction, SIGNAL(toggled(bool)), q, SLOT(updateSortOrder()));

        for (auto action : mSortAction->actions()) {
            sortActionMenu->addAction(action);
        }
        sortActionMenu->addSeparator();
        sortActionMenu->addAction(mSortDescendingAction);

        mThumbnailDetailsActionGroup = new QActionGroup(q);
        mThumbnailDetailsActionGroup->setExclusive(false);
        auto thumbnailDetailsAction = view->add<KActionMenu>(QStringLiteral("thumbnail_details"));
        thumbnailDetailsAction->setText(i18nc("@action:inmenu", "Thumbnail Details"));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Filename"), PreviewItemDelegate::FileNameDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Date"), PreviewItemDelegate::DateDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Image Size"), PreviewItemDelegate::ImageSizeDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "File Size"), PreviewItemDelegate::FileSizeDetail));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Rating"), PreviewItemDelegate::RatingDetail));
#endif
        mShowHiddenFilesAction = view->add<KToggleAction>(QStringLiteral("view_toggle_showhiddenfiles"));
        mShowHiddenFilesAction->setText(i18nc("@action:inmenu View", "Show Hidden Files"));
        mShowHiddenFilesAction->setIcon(QIcon::fromTheme(QStringLiteral("view-visible")));
        mShowHiddenFilesAction->setChecked(mDirModel->isShowHiddenFiles());
        mSortButton->setText(i18n("Sort")); mSortButton->setToolTip(i18n("Sort images"));
        mSortButton->setMenu(sortActionMenu->menu());
        mDetailsButton->setText(i18n("View")); mDetailsButton->setToolTip(i18n("Thumbnail details and hidden files"));
        auto detailsMenu = new QMenu(mDetailsButton);
        for (auto detail : mThumbnailDetailsActionGroup->actions()) detailsMenu->addAction(detail);
        detailsMenu->addSeparator(); detailsMenu->addAction(mShowHiddenFilesAction);
        mDetailsButton->setMenu(detailsMenu);

        actionCollection->setDefaultShortcuts(mShowHiddenFilesAction, KStandardShortcut::showHideHiddenFiles());
        QObject::connect(mShowHiddenFilesAction, SIGNAL(toggled(bool)), q, SLOT(slotToggleShowHiddenFiles()));

        auto file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
        action = file->addAction(QStringLiteral("add_folder_to_places"), q, &BrowseMainPage::addFolderToPlaces);
        action->setText(i18nc("@action:inmenu", "Add Folder to Places"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    }

    void setupFilterController()
    {
        auto menu = new QMenu(mAddFilterButton);
        mFilterController = new FilterController(mFilterFrame, mDirModel);
        const auto actionList = mFilterController->actionList();
        for (QAction *action : actionList) {
            menu->addAction(action);
        }
        mAddFilterButton->setMenu(menu);
        for (auto action : actionList) {
            q->addAction(action);
            QObject::connect(action, &QAction::triggered, q, [this] {
                schedulePresentationUpdate();
            });
        }
    }

    void setupFullScreenToolBar()
    {
        mFullScreenNavigation = new QWidget(mChrome);
        mFullScreenNavigation->setObjectName(QStringLiteral("fullscreenBrowseNavigation"));
        mFullScreenNavigation->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        auto row = new QHBoxLayout(mFullScreenNavigation);
        row->setContentsMargins(0, 0, 0, 0); row->setSpacing(3);
        mHistoryBack = new QAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18n("Back to previous folder"), q);
        mHistoryBack->setObjectName(QStringLiteral("browse_history_back"));
        mHistoryBack->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
        mHistoryForward = new QAction(QIcon::fromTheme(QStringLiteral("go-next")), i18n("Forward to next folder"), q);
        mHistoryForward->setObjectName(QStringLiteral("browse_history_forward"));
        mHistoryForward->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
        for (auto action : {mHistoryBack, mHistoryForward}) {
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut); q->addAction(action);
        }
        QObject::connect(mHistoryBack, &QAction::triggered, mUrlNavigator, &KUrlNavigator::goBack);
        QObject::connect(mHistoryForward, &QAction::triggered, mUrlNavigator, &KUrlNavigator::goForward);
        QObject::connect(mUrlNavigator, &KUrlNavigator::historyChanged, q, [this] { updatePresentation(); });
        FullScreenPill *pill = nullptr;
        for (auto action : {mActionCollection->action(QStringLiteral("toggle_sidebar")),
                            mActionCollection->action(QStringLiteral("view")), mHistoryBack, mHistoryForward,
                            mActionCollection->action(QStringLiteral("go_up"))}) {
            if (!pill || action == mHistoryBack) { pill = new FullScreenPill(mFullScreenNavigation); row->addWidget(pill); }
            auto button = new FullScreenButton(action, pill);
            button->setObjectName(QStringLiteral("fullscreenBrowser_") + action->objectName());
            pill->layout()->addWidget(button);
        }
        horizontalLayout_2->insertWidget(0, mFullScreenNavigation);
        mExitFullScreenButton = new FullScreenButton(mActionCollection->action(QStringLiteral("leave_fullscreen")), mHeader);
        mExitFullScreenButton->setObjectName(QStringLiteral("fullscreenBrowser_leave_fullscreen"));
        static_cast<QHBoxLayout *>(mHeader->layout())->addWidget(mExitFullScreenButton);
    }

    void updateSelectedMediaItems(const QItemSelection &selected, const QItemSelection &deselected)
    {
        for (auto index : selected.indexes()) {
            KFileItem item = mDirModel->itemForIndex(index);
            if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
                mSelectedMediaItems->append(item);
            }
        }
        for (auto index : deselected.indexes()) {
            KFileItem item = mDirModel->itemForIndex(index);
            mSelectedMediaItems->removeOne(item);
        }
    }

    void updateDocumentCountLabel()
    {
        if (mSelectedMediaItems->count() > 1) {
            KIO::filesize_t totalSize = 0;
            for (const auto &item : *mSelectedMediaItems) {
                totalSize += item.size();
            }
            const QString text = i18nc("@info:status %1 number of selected documents, %2 total number of documents, %3 total filesize of selected documents",
                                       "Selected %1 of %2 (%3)",
                                       mSelectedMediaItems->count(),
                                       mDocumentCountImages + mDocumentCountVideos,
                                       KIO::convertSize(totalSize));
            mDocumentCountLabel->setText(text);
            mDocumentCountLabel->setToolTip(text);
        } else {
            const QString imageText = i18ncp("@info:status Image files", "%1 image", "%1 images", mDocumentCountImages);
            const QString videoText = i18ncp("@info:status Video files", "%1 video", "%1 videos", mDocumentCountVideos);
            QString labelText;
            if (mDocumentCountImages > 0 && mDocumentCountVideos == 0) {
                labelText = imageText;
            } else if (mDocumentCountImages == 0 && mDocumentCountVideos > 0) {
                labelText = videoText;
            } else {
                labelText = i18nc("@info:status images, videos", "%1, %2", imageText, videoText);
            }
            mDocumentCountLabel->setText(labelText);
            mDocumentCountLabel->setToolTip(labelText);
        }
    }

    void documentCountsForIndexRange(const QModelIndex &parent, int start, int end, int &imageCountOut, int &videoCountOut)
    {
        imageCountOut = 0;
        videoCountOut = 0;
        for (int row = start; row <= end; ++row) {
            QModelIndex index = mDirModel->index(row, 0, parent);
            KFileItem item = mDirModel->itemForIndex(index);
            if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
                MimeTypeUtils::Kind kind = MimeTypeUtils::mimeTypeKind(item.mimetype());
                if (kind == MimeTypeUtils::KIND_RASTER_IMAGE || kind == MimeTypeUtils::KIND_SVG_IMAGE) {
                    imageCountOut++;
                } else if (kind == MimeTypeUtils::KIND_VIDEO) {
                    videoCountOut++;
                }
            }
        }
    }

    void updateContextBarActions()
    {
        PreviewItemDelegate::ContextBarActions actions;
        switch (GwenviewConfig::thumbnailActions()) {
        case ThumbnailActions::None:
            actions = PreviewItemDelegate::NoAction;
            break;
        case ThumbnailActions::ShowSelectionButtonOnly:
            actions = PreviewItemDelegate::SelectionAction;
            break;
        case ThumbnailActions::AllButtons:
        default:
            actions = PreviewItemDelegate::SelectionAction | PreviewItemDelegate::RotateAction;
            if (!mFullScreen) {
                actions |= PreviewItemDelegate::FullScreenAction;
            }
            break;
        }
        mDelegate->setContextBarActions(actions);
    }

    void applyPalette(bool fullScreenmode)
    {
        q->setPalette(mGvCore->palette(GvCore::NormalPalette));
        auto viewPalette = mGvCore->palette(fullScreenmode ? GvCore::FullScreenViewPalette : GvCore::NormalViewPalette);
        {
            QColor surface = q->palette().color(QPalette::Button);
            surface.setAlpha(255);
            viewPalette.setColor(QPalette::Base, surface);
            viewPalette.setColor(QPalette::Window, surface);
        }
        mThumbnailView->setPalette(viewPalette);
        mThumbnailView->viewport()->setAutoFillBackground(false);
        mUrlNavigatorContainer->setAutoFillBackground(false);
        const QColor edge = q->palette().color(QPalette::WindowText);
        mUrlNavigatorContainer->setStyleSheet(QStringLiteral(
            "QWidget#mUrlNavigatorContainer { border: 1px solid rgba(%1,%2,%3,22); border-radius: 10px; }")
            .arg(edge.red()).arg(edge.green()).arg(edge.blue()));
        updateControlIcons();
        q->update();
    }
};

BrowseMainPage::BrowseMainPage(QWidget *parent, KActionCollection *actionCollection, GvCore *gvCore)
    : QWidget(parent)
    , d(new BrowseMainPagePrivate)
{
    d->q = this;
    d->mGvCore = gvCore;
    d->mDirModel = gvCore->sortedDirModel();
    d->mDocumentCountImages = 0;
    d->mDocumentCountVideos = 0;
    d->mSelectedMediaItems = new KFileItemList;
    d->mActionCollection = actionCollection;
    d->mPresentationTimer = new QTimer(this);
    d->mPresentationTimer->setSingleShot(true);
    connect(d->mPresentationTimer, &QTimer::timeout, this, [this] { d->updatePresentation(); });
    d->setupWidgets();
    d->setupActions(actionCollection);
    d->setupFilterController();
    loadConfig();
    updateSortOrder();
    updateThumbnailDetails();

    // Set up connections for document count
    connect(d->mDirModel, &SortedDirModel::rowsInserted, this, &BrowseMainPage::slotDirModelRowsInserted);
    connect(d->mDirModel, &SortedDirModel::rowsAboutToBeRemoved, this, &BrowseMainPage::slotDirModelRowsAboutToBeRemoved);
    connect(d->mDirModel, &SortedDirModel::modelReset, this, &BrowseMainPage::slotDirModelReset);
    connect(thumbnailView(), &ThumbnailView::selectionChangedSignal, this, &BrowseMainPage::slotSelectionChanged);

    connect(qApp, &QApplication::paletteChanged, this, [this]() {
        d->applyPalette(d->mFullScreen);
    });

    auto refresh = [this] { d->schedulePresentationUpdate(); };
    connect(d->mDirModel, &SortedDirModel::rowsInserted, this, refresh);
    connect(d->mDirModel, &SortedDirModel::rowsRemoved, this, refresh);
    connect(d->mDirModel, &SortedDirModel::modelReset, this, refresh);
    connect(d->mDirModel->dirLister(), &KDirLister::started, this, [this] { d->mLoading = true; d->schedulePresentationUpdate(); });
    connect(d->mDirModel->dirLister(), qOverload<>(&KDirLister::completed), this, [this] { d->mLoading = false; d->schedulePresentationUpdate(); });
    installEventFilter(this);
    refresh();
}

BrowseMainPage::~BrowseMainPage()
{
    d->mSelectedMediaItems->clear();
    delete d->mSelectedMediaItems;
    delete d;
}

void BrowseMainPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (d->mFullScreen) {
        painter.fillRect(rect(), d->mGvCore->palette(GvCore::FullScreenViewPalette).brush(QPalette::Base));
        return;
    }
    paintFloatingPanel(painter, QRectF(rect()).adjusted(FloatingPanelInset, FloatingPanelInset, -FloatingPanelInset, -FloatingPanelInset), palette());
}

void BrowseMainPage::loadConfig()
{
    d->applyPalette(window()->isFullScreen());
    d->mUrlNavigator->setUrlEditable(GwenviewConfig::urlNavigatorIsEditable());
    d->mUrlNavigator->setShowFullPath(GwenviewConfig::urlNavigatorShowFullPath());

    d->mThumbnailSlider->setValue(GwenviewConfig::thumbnailSize());
    d->mThumbnailSlider->updateToolTip();
    // If GwenviewConfig::thumbnailSize() returns the current value of
    // mThumbnailSlider, it won't emit valueChanged() and the thumbnail view
    // won't be updated. That's why we do it ourself.
    d->mThumbnailView->setThumbnailAspectRatio(GwenviewConfig::thumbnailAspectRatio());
    d->mPreferredThumbnailWidth = GwenviewConfig::thumbnailSize();
    { const QSignalBlocker blocker(d->mThumbnailSlider); d->mThumbnailSlider->setValue(d->mPreferredThumbnailWidth); }
    d->fitThumbnails();

    const auto actionsList = d->mSortAction->actions();
    for (QAction *action : actionsList) {
        if (sortingFromSortAction(action) == GwenviewConfig::sorting()) {
            action->setChecked(true);
            break;
        }
    }
    d->mSortDescendingAction->setChecked(GwenviewConfig::sortDescending());
    d->mShowHiddenFilesAction->setChecked(GwenviewConfig::showHiddenFiles());

    d->updateContextBarActions();
}

void BrowseMainPage::saveConfig() const
{
    GwenviewConfig::setUrlNavigatorIsEditable(d->mUrlNavigator->isUrlEditable());
    GwenviewConfig::setUrlNavigatorShowFullPath(d->mUrlNavigator->showFullPath());
    GwenviewConfig::setThumbnailSize(d->mPreferredThumbnailWidth);
    GwenviewConfig::setSorting(sortingFromSortAction(d->mSortAction->checkedAction()));
    GwenviewConfig::setSortDescending(d->mSortDescendingAction->isChecked());
    GwenviewConfig::setThumbnailDetails(d->mDelegate->thumbnailDetails());
    GwenviewConfig::setShowHiddenFiles(d->mShowHiddenFilesAction->isChecked());
}

void BrowseMainPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d->schedulePresentationUpdate();
}

bool BrowseMainPage::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == d->mThumbnailView->viewport() && event->type() == QEvent::Resize)
        || (watched == d->mFilterFrame && (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent)))
        d->schedulePresentationUpdate();
    // Leave fullscreen when not viewing an image
    if (d->mFullScreen && event->type() == QEvent::ShortcutOverride) {
        const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            d->mActionCollection->action(QStringLiteral("leave_fullscreen"))->trigger();
            event->accept();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void BrowseMainPage::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::ForwardButton:
    case Qt::BackButton:
        return;
    default:
        QWidget::mousePressEvent(event);
    }
}

ThumbnailView *BrowseMainPage::thumbnailView() const
{
    return d->mThumbnailView;
}

KUrlNavigator *BrowseMainPage::urlNavigator() const
{
    return d->mUrlNavigator;
}

void BrowseMainPage::slotToggleShowHiddenFiles()
{
    d->mDirModel->setShowHiddenFiles(d->mShowHiddenFilesAction->isChecked());
}

void BrowseMainPage::reload()
{
    const QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : list) {
        d->mThumbnailView->reloadThumbnail(index);
    }
    d->mDirModel->reload();
}

void BrowseMainPage::editLocation()
{
    d->mUrlNavigator->setUrlEditable(true);
    d->mUrlNavigator->setFocus();
}

void BrowseMainPage::addFolderToPlaces()
{
    QUrl url = d->mUrlNavigator->locationUrl();
    QString text = url.adjusted(QUrl::StripTrailingSlash).fileName();
    if (text.isEmpty()) {
        text = url.toDisplayString();
    }
    d->mFilePlacesModel->addPlace(text, url);
}

void BrowseMainPage::slotDirModelRowsInserted(const QModelIndex &parent, int start, int end)
{
    int imageCount;
    int videoCount;
    d->documentCountsForIndexRange(parent, start, end, imageCount, videoCount);
    if (imageCount > 0 || videoCount > 0) {
        d->mDocumentCountImages += imageCount;
        d->mDocumentCountVideos += videoCount;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    int imageCount;
    int videoCount;
    d->documentCountsForIndexRange(parent, start, end, imageCount, videoCount);
    if (imageCount > 0 || videoCount > 0) {
        d->mDocumentCountImages -= imageCount;
        d->mDocumentCountVideos -= videoCount;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelReset()
{
    d->mDocumentCountImages = 0;
    d->mDocumentCountVideos = 0;
    d->mSelectedMediaItems->clear();
    d->updateDocumentCountLabel();
}

void BrowseMainPage::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    d->updateSelectedMediaItems(selected, deselected);
    d->updateDocumentCountLabel();
}

void BrowseMainPage::updateSortOrder()
{
    const QAction *action = d->mSortAction->checkedAction();
    GV_RETURN_IF_FAIL(action);

    const Qt::SortOrder order = d->mSortDescendingAction->isChecked() ? Qt::DescendingOrder : Qt::AscendingOrder;
    KDirModel::ModelColumns column = KDirModel::Name;
    int sortRole = Qt::DisplayRole;

    // Map Sorting::Enum to model columns and sorting roles
    switch (sortingFromSortAction(action)) {
    case Sorting::Name:
        column = KDirModel::Name;
        break;
    case Sorting::Size:
        column = KDirModel::Size;
        break;
    case Sorting::Date:
        column = KDirModel::ModifiedTime;
        break;
    case Sorting::Type:
        column = KDirModel::Type;
        break;
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    case Sorting::Rating:
        column = KDirModel::Name;
        sortRole = SemanticInfoDirModel::RatingRole;
        break;
#endif
    }

    d->mDirModel->setSortRole(sortRole);
    d->mDirModel->sort(column, order);
    d->mGvCore->setTrackFileManagerSorting(false);
}

void BrowseMainPage::updateThumbnailDetails()
{
    PreviewItemDelegate::ThumbnailDetails details = {};
    const auto actionList = d->mThumbnailDetailsActionGroup->actions();
    for (const QAction *action : actionList) {
        if (action->isChecked()) {
            details |= PreviewItemDelegate::ThumbnailDetail(action->data().toInt());
        }
    }
    d->mDelegate->setThumbnailDetails(details);
    d->fitThumbnails();
}

void BrowseMainPage::setFullScreenMode(bool fullScreen)
{
    d->mFullScreen = fullScreen;
    const int margin = fullScreen ? 0 : FloatingPanelPadding;
    layout()->setContentsMargins(margin, margin, margin, margin);
    layout()->setSpacing(fullScreen ? 0 : 8);
    d->applyPalette(fullScreen);
    d->mUrlNavigatorContainer->setContentsMargins(4, 3, 4, 3);
    d->updateContextBarActions();
    if (fullScreen && !d->mFullScreenNavigation) d->setupFullScreenToolBar();
    if (d->mFullScreenNavigation) {
        d->mFullScreenNavigation->setVisible(fullScreen);
        d->mExitFullScreenButton->setVisible(fullScreen);
    }
    d->mChrome->setFloating(fullScreen);
    d->mGallery->setFloating(fullScreen);
    d->mChrome->layout()->setContentsMargins(fullScreen ? QMargins(FloatingPanelPadding, 18, FloatingPanelPadding, 28) : QMargins());
    const int galleryPadding = fullScreen ? FloatingPanelPadding : 0;
    d->mGallery->layout()->setContentsMargins(galleryPadding, galleryPadding, galleryPadding, galleryPadding);
    d->mToggleSideBarButton->setVisible(!fullScreen);
    for (auto button : {d->mSortButton, d->mDetailsButton, static_cast<QToolButton *>(d->mAddFilterButton)})
        button->setMinimumHeight(fullScreen ? 38 : 34);
    d->updatePresentation();

}

void BrowseMainPage::setStatusBarVisible(bool visible)
{
    d->mStatusBarContainer->setVisible(visible);
}

void BrowseMainPage::slotUrlsDropped(const QUrl &destUrl, QDropEvent *event)
{
    const QList<QUrl> urlList = KUrlMimeData::urlsFromMimeData(event->mimeData());
    if (urlList.isEmpty()) {
        return;
    }
    event->acceptProposedAction();

    // We can't call FileOperations::showMenuForDroppedUrls() directly because
    // we need the slot to return so that the drop event is accepted. Otherwise
    // the drop cursor is still visible when the menu is shown.
    QMetaObject::invokeMethod(
        this,
        [this, urlList, destUrl]() {
            showMenuForDroppedUrls(urlList, destUrl);
        },
        Qt::QueuedConnection);
}

void BrowseMainPage::showMenuForDroppedUrls(const QList<QUrl> &urlList, const QUrl &destUrl)
{
    FileOperations::showMenuForDroppedUrls(d->mUrlNavigator, urlList, destUrl);
}

QToolButton *BrowseMainPage::toggleSideBarButton() const
{
    return d->mToggleSideBarButton;
}

} // namespace

#include "moc_browsemainpage.cpp"
