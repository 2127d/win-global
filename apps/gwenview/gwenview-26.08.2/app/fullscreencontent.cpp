#include <lib/sequoiadesign.h>
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
#include "fullscreencontent.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QLabel>
#include <QMenu>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>
#include <QPainter>
#include <QScreen>
#include "fullscreencontrols.h"
#include "fullscreenbackdrop.h"

// KF
#include <KActionCollection>
#include <KActionMenu>
#include <KIconLoader>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include <gvcore.h>
#include <lib/document/documentfactory.h>
#include <lib/eventwatcher.h>
#include <lib/fullscreenbar.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/slideshow.h>
#include <lib/stylesheetutils.h>
#include <lib/thumbnailview/thumbnailbarview.h>

namespace Gwenview
{
class FullScreenToolBar : public QWidget
{
public:
    explicit FullScreenToolBar(QWidget *parent = nullptr) : QWidget(parent), mLayout(new QHBoxLayout(this))
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mLayout->setContentsMargins(0, 0, 0, 0); mLayout->setSpacing(3);
    }
    void addAction(QAction *action)
    {
        if (!mPill) { mPill = new FullScreenPill(this); mLayout->addWidget(mPill); }
        mPill->layout()->addWidget(new FullScreenButton(action, mPill));
    }
    void addSeparator() { mPill = nullptr; }
private:
    FullScreenPill *mPill = nullptr;
    QHBoxLayout *mLayout;
};

class FullScreenThumbnailDelegate : public ThumbnailBarItemDelegate
{
public:
    explicit FullScreenThumbnailDelegate(ThumbnailBarView *view) : ThumbnailBarItemDelegate(view), mView(view) {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        const int side = qMax(32, mView->viewport()->height() - 4);
        return QSize(side, side);
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save(); painter->setRenderHint(QPainter::Antialiasing);
        const QRectF card = QRectF(option.rect).adjusted(2, 2, -2, -2);
        const bool selected = option.state & QStyle::State_Selected;
        const bool focused = option.state & QStyle::State_HasFocus;
        QColor accent = option.palette.color(QPalette::Highlight);
        QColor fill = Sequoia::alpha(selected ? accent : option.palette.color(QPalette::WindowText),
                                    selected ? 0.16 : option.state & QStyle::State_MouseOver ? 0.08 : 0);
        painter->setBrush(fill);
        painter->setPen(selected || focused ? QPen(accent, 1.5) : QPen(Qt::NoPen));
        painter->drawRoundedRect(card, Sequoia::CardRadius, Sequoia::CardRadius);
        const QPixmap image = mView->thumbnailForIndex(index);
        if (!image.isNull()) {
            const QRectF inside = card.adjusted(5, 5, -5, -5);
            const QSizeF size = image.deviceIndependentSize().scaled(inside.size(), Qt::KeepAspectRatio);
            const QRectF target(inside.center() - QPointF(size.width() / 2, size.height() / 2), size);
            QPainterPath clip; clip.addRoundedRect(target, Sequoia::ControlRadius, Sequoia::ControlRadius); painter->setClipPath(clip);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->drawPixmap(target, image, QRectF(image.rect()));
        }
        painter->restore();
    }
private:
    ThumbnailBarView *mView;
};

FullScreenContent::FullScreenContent(QObject *parent, GvCore *gvCore)
    : QObject(parent)
{
    mGvCore = gvCore;
    mViewPageVisible = false;
}

void FullScreenContent::init(KActionCollection *actionCollection, QWidget *autoHideParentWidget, SlideShow *slideShow)
{
    mSlideShow = slideShow;
    mActionCollection = actionCollection;
    connect(actionCollection->action(QStringLiteral("view")), &QAction::toggled, this, &FullScreenContent::slotViewModeActionToggled);

    // mAutoHideContainer
    mAutoHideContainer = new FullScreenBar(autoHideParentWidget);
    mAutoHideContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto layout = new QVBoxLayout(mAutoHideContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    EventWatcher::install(autoHideParentWidget, QEvent::Resize, this, SLOT(adjustSize()));

    // mContent
    mContent = new FullScreenSurface;
    static_cast<FullScreenSurface *>(mContent)->setFloating(false);
    mContent->setObjectName(QStringLiteral("fullscreenSurface"));
    mContent->setMaximumWidth(1200);
    mContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mContent->setAutoFillBackground(false);
    EventWatcher::install(mContent, QEvent::Show, this, SLOT(updateCurrentUrlWidgets()));
    layout->addWidget(mContent, 0, Qt::AlignHCenter);
    if (auto viewer = autoHideParentWidget->findChild<QGraphicsView *>(QStringLiteral("roundedImageViewer")))
        new FullScreenBackdrop(mContent, viewer, mAutoHideContainer);

    createOptionsAction();

    mToolBar = new FullScreenToolBar(mContent);
    mToolBar->setObjectName(QStringLiteral("fullscreenNavigation"));
    for (const char *name : {"toggle_sidebar", "browse", "go_previous", "toggle_slideshow", "go_next"}) {
        if (qstrcmp(name, "go_previous") == 0) mToolBar->addSeparator();
        mToolBar->addAction(actionCollection->action(QString::fromLatin1(name)));
    }

    mImageToolBar = new FullScreenToolBar(mContent);
    mImageToolBar->setObjectName(QStringLiteral("fullscreenImageControls"));
    for (const char *name : {"view_zoom_out", "view_zoom_in", "view_zoom_to_fit", "view_actual_size", "rotate_left", "rotate_right"}) {
        if (qstrcmp(name, "rotate_left") == 0) mImageToolBar->addSeparator();
        mImageToolBar->addAction(actionCollection->action(QString::fromLatin1(name)));
    }

    mInformationLabel = new QLabel;
    mInformationLabel->setObjectName(QStringLiteral("fullscreenImageInformation"));
    mInformationLabel->setTextFormat(Qt::PlainText);
    mInformationLabel->setMinimumWidth(0);
    mInformationLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto font = mInformationLabel->font(); font.setBold(true); mInformationLabel->setFont(font);
    mDocumentCountLabel = new QLabel;
    mDocumentCountLabel->setObjectName(QStringLiteral("fullscreenImageCount"));
    mDocumentCountLabel->setForegroundRole(QPalette::PlaceholderText);

    mKeepVisibleAction = new QAction(QIcon::fromTheme(QStringLiteral("window-pin")), i18n("Keep controls visible"), this);
    mKeepVisibleAction->setObjectName(QStringLiteral("keep_controls_visible"));
    mKeepVisibleAction->setCheckable(true);
    connect(mKeepVisibleAction, &QAction::toggled, this, [this](bool checked) {
        mAutoHideContainer->setAutoHidingEnabled(!checked && !mImageMetaInfoDialog);
    });
    mRightToolBar = new FullScreenToolBar(mContent);
    mRightToolBar->addAction(mKeepVisibleAction);
    mRightToolBar->addAction(mOptionsAction);
    mRightToolBar->addAction(mActionCollection->action(QStringLiteral("leave_fullscreen")));

    mInformationContainer = new QWidget;
    auto header = new QHBoxLayout(mInformationContainer);
    header->setContentsMargins(0, 0, 0, 0); header->setSpacing(8);
    auto informationPill = new FullScreenPill(mInformationContainer);
    informationPill->setObjectName(QStringLiteral("fullscreenInformationPill"));
    auto informationLayout = static_cast<QHBoxLayout *>(informationPill->layout());
    informationLayout->setSpacing(12);
    informationLayout->addWidget(mInformationLabel, 1);
    informationLayout->addWidget(mDocumentCountLabel);
    header->addWidget(informationPill, 1);
    header->addWidget(mRightToolBar);

    mThumbnailBar = new ThumbnailBarView(mContent);
    mThumbnailSurface = new FullScreenSurface(mContent);
    mThumbnailSurface->setObjectName(QStringLiteral("fullscreenPreviewSurface"));
    static_cast<FullScreenSurface *>(mThumbnailSurface)->setHorizontalInset(6);
    auto previewLayout = new QVBoxLayout(mThumbnailSurface);
    previewLayout->setContentsMargins(14, 16, 14, 24);
    previewLayout->addWidget(mThumbnailBar);
    mThumbnailBar->setObjectName(QStringLiteral("fullscreenThumbnails"));
    mThumbnailBar->setThumbnailScaleMode(ThumbnailView::ScaleToSquare);
    mThumbnailBar->setItemDelegate(new FullScreenThumbnailDelegate(mThumbnailBar));
    mThumbnailBar->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mThumbnailBar->setFrameShape(QFrame::NoFrame);
    mThumbnailBar->setAccessibleName(i18n("Full screen images"));
    mMinimumThumbnailBarHeight = 104;
    connect(mThumbnailBar, &ThumbnailBarView::rowsInsertedSignal, this, &FullScreenContent::updateDocumentCountLabel);
    connect(mThumbnailBar, &ThumbnailBarView::rowsRemovedSignal, this, &FullScreenContent::updateDocumentCountLabel);
    connect(mThumbnailBar, &ThumbnailBarView::indexActivated, this, &FullScreenContent::updateDocumentCountLabel);

    updateLayout();

    updateContainerAppearance();
}

ThumbnailBarView *FullScreenContent::thumbnailBar() const
{
    return mThumbnailBar;
}

void FullScreenContent::setCurrentUrl(const QUrl &url)
{
    if (mCurrentDocument) disconnect(mCurrentDocument.data(), &Document::metaInfoUpdated, this, &FullScreenContent::updateCurrentUrlWidgets);
    if (url.isEmpty()) {
        mCurrentDocument = Document::Ptr();
    } else {
        mCurrentDocument = DocumentFactory::instance()->load(url);
        connect(mCurrentDocument.data(), &Document::metaInfoUpdated, this, &FullScreenContent::updateCurrentUrlWidgets);
    }
    updateCurrentUrlWidgets();

    // Give the thumbnail view time to update its "current index"
    QTimer::singleShot(0, this, &FullScreenContent::updateDocumentCountLabel);
}

void FullScreenContent::updateInformationLabel()
{
    if (!mCurrentDocument) {
        mInformationLabel->clear(); mInformationLabel->setToolTip(QString());
        return;
    }

    if (!mInformationLabel->isVisible()) {
        return;
    }

    ImageMetaInfoModel *model = mCurrentDocument->metaInfo();

    QStringList valueList;
    const QStringList fullScreenPreferredMetaInfoKeyList = GwenviewConfig::fullScreenPreferredMetaInfoKeyList();
    for (const QString &key : fullScreenPreferredMetaInfoKeyList) {
        const QString value = model->getValueForKey(key);
        if (!value.isEmpty()) {
            valueList << value;
        }
    }
    const QString text = valueList.join(i18nc("@item:intext fullscreen meta info separator", ", "));

    mInformationLabel->setText(mInformationLabel->fontMetrics().elidedText(text, Qt::ElideMiddle, mInformationLabel->width()));
    mInformationLabel->setToolTip(text);
    mInformationLabel->setAccessibleName(text);
}

void FullScreenContent::updateCurrentUrlWidgets()
{
    updateInformationLabel();
    updateMetaInfoDialog();
}

void FullScreenContent::showImageMetaInfoDialog()
{
    if (!mImageMetaInfoDialog) {
        mImageMetaInfoDialog = new ImageMetaInfoDialog(mInformationLabel);
        // Do not let the fullscreen theme propagate to this dialog for now,
        // it's already quite complicated to create a theme
        mImageMetaInfoDialog->setStyle(QApplication::style());
        mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(mImageMetaInfoDialog.data(),
                &ImageMetaInfoDialog::preferredMetaInfoKeyListChanged,
                this,
                &FullScreenContent::slotPreferredMetaInfoKeyListChanged);
        connect(mImageMetaInfoDialog.data(), &QObject::destroyed, this, &FullScreenContent::slotImageMetaInfoDialogClosed);
    }
    if (mCurrentDocument) {
        mImageMetaInfoDialog->setMetaInfo(mCurrentDocument->metaInfo(), GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
    }
    mAutoHideContainer->setAutoHidingEnabled(false);
    mImageMetaInfoDialog->show();
}

void FullScreenContent::slotPreferredMetaInfoKeyListChanged(const QStringList &list)
{
    GwenviewConfig::setFullScreenPreferredMetaInfoKeyList(list);
    GwenviewConfig::self()->save();
    updateInformationLabel();
}

void FullScreenContent::updateMetaInfoDialog()
{
    if (!mImageMetaInfoDialog) {
        return;
    }
    ImageMetaInfoModel *model = mCurrentDocument ? mCurrentDocument->metaInfo() : nullptr;
    mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
}

static QString formatSlideShowIntervalText(int value)
{
    return i18ncp("Slideshow interval in seconds", "%1 sec", "%1 secs", value);
}

void FullScreenContent::updateLayout()
{
    if (!mContent->layout()) {
        auto layout = new QVBoxLayout(mContent);
        layout->setContentsMargins(8, 0, 8, 0); layout->setSpacing(0);
        layout->addWidget(mInformationContainer);
        mControlLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        mControlLayout->setSpacing(6);
        mControlLayout->addWidget(mToolBar, 0, Qt::AlignLeft);
        mControlLayout->addStretch();
        mControlLayout->addWidget(mImageToolBar, 0, Qt::AlignRight);
        layout->addLayout(mControlLayout);
        layout->addWidget(mThumbnailSurface);
    }
    const int width = qMin(1200, mAutoHideContainer->parentWidget()->width());
    const bool compact = width < mToolBar->sizeHint().width() + mImageToolBar->sizeHint().width() + 22;
    mControlLayout->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    mControlLayout->setAlignment(mImageToolBar, compact ? Qt::AlignLeft : Qt::AlignRight);
    const int availableHeight = mAutoHideContainer->parentWidget()->height();
    const int preferredHeight = GwenviewConfig::fullScreenBarHeight() > 0 ? GwenviewConfig::fullScreenBarHeight() : 120;
    const int height = qBound(mMinimumThumbnailBarHeight, preferredHeight,
                              qMax(mMinimumThumbnailBarHeight, qMin(180, availableHeight / 4)));
    mThumbnailBar->setFixedHeight(height);
    mThumbnailSurface->setVisible(GwenviewConfig::showFullScreenThumbnails());
    mThumbnailBar->setVisible(GwenviewConfig::showFullScreenThumbnails());
    mContent->setFixedWidth(width);
    mContent->layout()->activate();
    mContent->setFixedHeight(mContent->sizeHint().height());
    mAutoHideContainer->setFixedSize(mAutoHideContainer->parentWidget()->width(), mContent->height());
    mAutoHideContainer->layout()->activate();
    updateInformationLabel();
}

void FullScreenContent::updateContainerAppearance()
{
    if (!mFullScreenMode || !mViewPageVisible) {
        mAutoHideContainer->setActivated(false);
        return;
    }
    setupThumbnailBarStyleSheet();
    updateLayout();
    mAutoHideContainer->setActivated(true);
}

void FullScreenContent::adjustSize()
{
    if (mFullScreenMode && mViewPageVisible) updateLayout();
}

void FullScreenContent::createOptionsAction()
{
    // We do not use a KActionMenu because:
    //
    // - It causes the button to show a small down arrow on its right,
    // which makes it wider
    //
    // - We can't control where the menu shows: in no-thumbnail-mode, the
    // menu should not be aligned to the right edge of the screen because
    // if the mode is changed to thumbnail-mode, we want the option button
    // to remain visible
    mOptionsAction = new QAction(this);
    mOptionsAction->setObjectName(QStringLiteral("options"));
    mOptionsAction->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    mOptionsAction->setText(i18n("Configure"));
    mOptionsAction->setToolTip(i18nc("@info:tooltip", "Configure full screen mode"));
    QObject::connect(mOptionsAction, &QAction::triggered, this, &FullScreenContent::showOptionsMenu);
}

void FullScreenContent::updateSlideShowIntervalLabel()
{
    Q_ASSERT(mConfigWidget);
    const int value = mConfigWidget->mSlideShowIntervalSlider->value();
    const QString text = formatSlideShowIntervalText(value);
    mConfigWidget->mSlideShowIntervalLabel->setText(text);
}

void FullScreenContent::setFullScreenBarHeight(int value)
{
    GwenviewConfig::setFullScreenBarHeight(value);
    updateLayout();
}

void FullScreenContent::showOptionsMenu()
{
    if (mConfigWidget) return;

    mConfigWidget = new FullScreenConfigWidget;
    FullScreenConfigWidget *widget = mConfigWidget;

    // Put widget in a menu
    QMenu menu(mContent);
    menu.setObjectName(QStringLiteral("fullscreenOptionsMenu"));
    auto action = new QWidgetAction(&menu);
    action->setDefaultWidget(widget);
    menu.addAction(action);

    // Slideshow checkboxes
    widget->mSlideShowLoopCheckBox->setChecked(mSlideShow->loopAction()->isChecked());
    connect(widget->mSlideShowLoopCheckBox, &QAbstractButton::toggled, mSlideShow->loopAction(), &QAction::trigger);

    widget->mSlideShowRandomCheckBox->setChecked(mSlideShow->randomAction()->isChecked());
    connect(widget->mSlideShowRandomCheckBox, &QAbstractButton::toggled, mSlideShow->randomAction(), &QAction::trigger);

    // Interval slider
    widget->mConfigureDisplayedInformationButton->setText(i18n("Choose image details…"));
    widget->mHeightSlider->setMinimumWidth(0);
    widget->mSlideShowIntervalSlider->setValue(int(GwenviewConfig::interval()));
    connect(widget->mSlideShowIntervalSlider, &QAbstractSlider::valueChanged, mSlideShow, &SlideShow::setInterval);
    connect(widget->mSlideShowIntervalSlider, &QAbstractSlider::valueChanged, this, &FullScreenContent::updateSlideShowIntervalLabel);

    // Interval label
    QString text = formatSlideShowIntervalText(88);
    int width = widget->mSlideShowIntervalLabel->fontMetrics().boundingRect(text).width();
    widget->mSlideShowIntervalLabel->setFixedWidth(width);
    updateSlideShowIntervalLabel();

    // Image information
    connect(widget->mConfigureDisplayedInformationButton, &QAbstractButton::clicked, this, &FullScreenContent::showImageMetaInfoDialog);

    // Thumbnails
    widget->mThumbnailGroupBox->setVisible(mViewPageVisible);
    if (mViewPageVisible) {
        widget->mShowThumbnailsCheckBox->setChecked(GwenviewConfig::showFullScreenThumbnails());
        widget->mHeightSlider->setMinimum(mMinimumThumbnailBarHeight);
        widget->mHeightSlider->setValue(mThumbnailBar->height());
        connect(widget->mShowThumbnailsCheckBox, &QAbstractButton::toggled, this, &FullScreenContent::slotShowThumbnailsToggled);
        connect(widget->mHeightSlider, &QAbstractSlider::valueChanged, this, &FullScreenContent::setFullScreenBarHeight);
    }

    // Show menu below its button
    QPoint pos;
    QWidget *button = nullptr;
    for (QObject *object : mOptionsAction->associatedObjects()) {
        auto candidate = qobject_cast<QToolButton *>(object);
        if (candidate && candidate->isVisible()) { button = candidate; break; }
    }
    if (!button) button = mContent;
    if (QApplication::isRightToLeft()) {
        pos = button->mapToGlobal(button->rect().bottomLeft());
    } else {
        pos = button->mapToGlobal(button->rect().bottomRight());
        pos.rx() -= menu.sizeHint().width();
    }
    if (auto screen = button->screen()) {
        const QRect area = screen->availableGeometry();
        pos.setX(qBound(area.left(), pos.x(), qMax(area.left(), area.right() - menu.sizeHint().width())));
        pos.setY(qBound(area.top(), pos.y(), qMax(area.top(), area.bottom() - menu.sizeHint().height())));
    }
    menu.exec(pos);
}

void FullScreenContent::setFullScreenMode(bool fullScreenMode)
{
    mFullScreenMode = fullScreenMode;
    updateContainerAppearance();
    setupThumbnailBarStyleSheet();
}

void FullScreenContent::setDistractionFreeMode(bool distractionFreeMode)
{
    mAutoHideContainer->setEdgeTriggerEnabled(!distractionFreeMode);
}

void FullScreenContent::slotImageMetaInfoDialogClosed()
{
    mAutoHideContainer->setAutoHidingEnabled(!mKeepVisibleAction->isChecked());
}

void FullScreenContent::slotShowThumbnailsToggled(bool value)
{
    GwenviewConfig::setShowFullScreenThumbnails(value);
    GwenviewConfig::self()->save();
    mThumbnailSurface->setVisible(value);
    mThumbnailBar->setVisible(value);
    updateLayout();

}

void FullScreenContent::slotViewModeActionToggled(bool value)
{
    mViewPageVisible = value;
    updateContainerAppearance();
}

void FullScreenContent::setupThumbnailBarStyleSheet()
{
    const QPalette pal = mGvCore->palette(GvCore::NormalPalette);
    mContent->setPalette(pal);
    QPalette countPalette = pal;
    QColor secondary = pal.color(QPalette::WindowText); secondary.setAlpha(200);
    countPalette.setColor(QPalette::PlaceholderText, secondary);
    mDocumentCountLabel->setPalette(countPalette);
    QColor accent = mGvCore->palette(GvCore::NormalPalette).color(QPalette::Highlight);
    QColor selected = Sequoia::alpha(accent, 0.16);
    QColor hover = Sequoia::alpha(pal.color(QPalette::WindowText), 0.08);
    mThumbnailBar->setStyleSheet(QStringLiteral(
        "QListView { background: transparent; border: none; }"
        "QListView::item { border: 1px solid transparent; border-radius: 22px; }"
        "QListView::item:selected { background: %1; border-color: %2; }"
        "QListView::item:hover:!selected { background: %3; }"
        "QListView::item:focus { border-color: %2; }")
        .arg(StyleSheetUtils::rgba(selected), StyleSheetUtils::rgba(accent), StyleSheetUtils::rgba(hover)));
    mThumbnailBar->setAutoFillBackground(false);
    mThumbnailBar->viewport()->setAutoFillBackground(false);
}

void FullScreenContent::updateDocumentCountLabel()
{
    const int current = mThumbnailBar->currentIndex().row() + 1;
    const int total = mThumbnailBar->model()->rowCount();
    const QString text = i18nc("@info:status %1 current document index, %2 total documents", "%1 of %2", current, total);
    mDocumentCountLabel->setText(text);
}

} // namespace

#include "moc_fullscreencontent.cpp"
