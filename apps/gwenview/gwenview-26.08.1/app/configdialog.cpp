// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "configdialog.h"

// Qt
#include <QFontDatabase>
#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <lib/sequoiadesign.h>
#include <QScreen>
#include <QTabWidget>
#include <QTabBar>

// KF
#include <KLocalizedString>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/invisiblebuttongroup.h>

namespace Gwenview
{
class SettingsScrollArea : public QScrollArea
{
public:
    QSize sizeHint() const override { return QSize(680, 480); }
    QSize minimumSizeHint() const override { return QSize(400, 240); }
};

template<class Ui>
QWidget *setupPage(Ui &ui)
{
    auto widget = new QWidget;
    ui.setupUi(widget);
    for (auto label : widget->findChildren<QLabel *>()) label->setWordWrap(true);
    auto scroll = new SettingsScrollArea;
    scroll->setObjectName(QStringLiteral("gwenviewSettingsScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // Weather places section headings outside its 22px cards, centered in a
    // bounded column. Keep the original KConfig controls and object names.
    if (auto column = qobject_cast<QVBoxLayout *>(widget->layout())) {
        for (auto card : widget->findChildren<QFrame *>()) {
            if (!card->property("gwenviewSettingsCard").toBool()) continue;
            for (auto title : card->findChildren<QLabel *>()) {
                if (!title->property("gwenviewSectionTitle").toBool()) continue;
                card->layout()->removeWidget(title);
                column->insertWidget(column->indexOf(card), title);
                break;
            }
        }
    }
    widget->setMaximumWidth(540);
    auto container = new QWidget;
    auto row = new QHBoxLayout(container); row->setContentsMargins(8, 8, 8, 8);
    row->addStretch(); row->addWidget(widget, 1); row->addStretch();
    scroll->setWidget(container);
    return scroll;
}

ConfigDialog::ConfigDialog(QWidget *parent)
    : KConfigDialog(parent, QStringLiteral("Settings"), GwenviewConfig::self())
{
    setFaceType(KPageDialog::Tabbed);
    setWindowTitle(i18nc("@title:window", "Gwenview Settings"));
    resize(QSize(630, 700).boundedTo(screen()->availableGeometry().size() * 0.9));

    // General
    QWidget *widget = setupPage(mGeneralConfigPage);

    mWrapNavigationBehaviorGroup = new InvisibleButtonGroup(widget);
    mWrapNavigationBehaviorGroup->setObjectName(QStringLiteral("kcfg_NavigationEndNotification"));
    mWrapNavigationBehaviorGroup->addButton(mGeneralConfigPage.neverShowWrapNoticeRadioButton, int(SlideShow::NavigationEndNotification::NeverWarn));
    mWrapNavigationBehaviorGroup->addButton(mGeneralConfigPage.wrapNoticeOnSlideshowRadioButton, int(SlideShow::NavigationEndNotification::WarnOnSlideshow));
    mWrapNavigationBehaviorGroup->addButton(mGeneralConfigPage.alwaysShowWrapNoticeRadioButton, int(SlideShow::NavigationEndNotification::AlwaysWarn));

    mFullScreenBackgroundGroup = new InvisibleButtonGroup(widget);
    mFullScreenBackgroundGroup->setObjectName(QStringLiteral("kcfg_FullScreenBackground"));
    mFullScreenBackgroundGroup->addButton(mGeneralConfigPage.fullscreenBackgroundBlackRadioButton, int(FullScreenBackground::Black));
    mFullScreenBackgroundGroup->addButton(mGeneralConfigPage.fullscreenBackgroundImageRadioButton, int(FullScreenBackground::Image));

    mThumbnailActionsGroup = new InvisibleButtonGroup(widget);
    mThumbnailActionsGroup->setObjectName(QStringLiteral("kcfg_ThumbnailActions"));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.allButtonsThumbnailActionsRadioButton, int(ThumbnailActions::AllButtons));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.selectionOnlyThumbnailActionsRadioButton, int(ThumbnailActions::ShowSelectionButtonOnly));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.noneThumbnailActionsRadioButton, int(ThumbnailActions::None));

    mGeneralConfigPageItem = addPage(widget, i18n("General"));
    mGeneralConfigPageItem->setIcon(QIcon::fromTheme(QStringLiteral("gwenview")));
    connect(mGeneralConfigPage.kcfg_JPEGQuality, &QAbstractSlider::valueChanged, this, [=](int value) {
        mGeneralConfigPage.jpegQualitySpinner->setValue(value);
    });
    connect(mGeneralConfigPage.jpegQualitySpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
        mGeneralConfigPage.kcfg_JPEGQuality->setValue(value);
    });
    mGeneralConfigPage.jpegQualitySpinner->setValue(mGeneralConfigPage.kcfg_JPEGQuality->value());
    mGeneralConfigPage.lossyImageFormatHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));

    mGeneralConfigPage.kcfg_AutoplayVideos->setEnabled(mGeneralConfigPage.kcfg_ListVideos->isChecked());
    connect(mGeneralConfigPage.kcfg_ListVideos, &QCheckBox::stateChanged, this, [=](int state) {
        mGeneralConfigPage.kcfg_AutoplayVideos->setEnabled(state == Qt::Checked);
    });

    // Image View
    widget = setupPage(mImageViewConfigPage);

    mAlphaBackgroundModeGroup = new InvisibleButtonGroup(widget);
    mAlphaBackgroundModeGroup->setObjectName(QStringLiteral("kcfg_AlphaBackgroundMode"));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.surroundingRadioButton, int(AbstractImageView::AlphaBackgroundNone));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.checkBoardRadioButton, int(AbstractImageView::AlphaBackgroundCheckBoard));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.solidColorRadioButton, int(AbstractImageView::AlphaBackgroundSolid));

    mWheelBehaviorGroup = new InvisibleButtonGroup(widget);
    mWheelBehaviorGroup->setObjectName(QStringLiteral("kcfg_MouseWheelBehavior"));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelScrollRadioButton, int(MouseWheelBehavior::Scroll));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelBrowseRadioButton, int(MouseWheelBehavior::Browse));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelZoomRadioButton, int(MouseWheelBehavior::Zoom));

    mAnimationMethodGroup = new InvisibleButtonGroup(widget);
    mAnimationMethodGroup->setObjectName(QStringLiteral("kcfg_AnimationMethod"));
#ifdef QT_NO_OPENGL
    mImageViewConfigPage.glAnimationRadioButton->setEnabled(false);
    mAnimationMethodGroup->addButton(mImageViewConfigPage.glAnimationRadioButton, int(DocumentView::NoAnimation));
#else
    mAnimationMethodGroup->addButton(mImageViewConfigPage.glAnimationRadioButton, int(DocumentView::GLAnimation));
#endif
    mAnimationMethodGroup->addButton(mImageViewConfigPage.softwareAnimationRadioButton, int(DocumentView::SoftwareAnimation));
    mAnimationMethodGroup->addButton(mImageViewConfigPage.noAnimationRadioButton, int(DocumentView::NoAnimation));

    mZoomModeGroup = new InvisibleButtonGroup(widget);
    mZoomModeGroup->setObjectName(QStringLiteral("kcfg_ZoomMode"));
    mZoomModeGroup->addButton(mImageViewConfigPage.autofitZoomModeRadioButton, int(ZoomMode::Autofit));
    mZoomModeGroup->addButton(mImageViewConfigPage.keepSameZoomModeRadioButton, int(ZoomMode::KeepSame));
    mZoomModeGroup->addButton(mImageViewConfigPage.individualZoomModeRadioButton, int(ZoomMode::Individual));

    mThumbnailBarOrientationGroup = new InvisibleButtonGroup(widget);
    mThumbnailBarOrientationGroup->setObjectName(QStringLiteral("kcfg_ThumbnailBarOrientation"));
    mThumbnailBarOrientationGroup->addButton(mImageViewConfigPage.horizontalRadioButton, int(Qt::Horizontal));
    mThumbnailBarOrientationGroup->addButton(mImageViewConfigPage.verticalRadioButton, int(Qt::Vertical));

    mImageViewConfigPageItem = addPage(widget, i18n("Image View"));
    mImageViewConfigPageItem->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display-color")));

    // Advanced
    widget = setupPage(mAdvancedConfigPage);

    mRenderingIntentGroup = new InvisibleButtonGroup(widget);
    mRenderingIntentGroup->setObjectName(QStringLiteral("kcfg_RenderingIntent"));
    mRenderingIntentGroup->addButton(mAdvancedConfigPage.relativeRenderingIntentRadioButton, int(RenderingIntent::Relative));
    mRenderingIntentGroup->addButton(mAdvancedConfigPage.perceptualRenderingIntentRadioButton, int(RenderingIntent::Perceptual));

    mAdvancedConfigPageItem = addPage(widget, i18n("Advanced"));
    mAdvancedConfigPageItem->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    mAdvancedConfigPage.cacheHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.perceptualHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.relativeHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.colorProfileHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    // KConfigDialog adds another scroll area around each page and gives it a
    // MinimumExpanding policy. Let our inner scroll area own overflow instead
    // of making the preferred 680px page width a hard window minimum.
    for (auto scroll : findChildren<QScrollArea *>()) {
        if (scroll->objectName() != QLatin1String("gwenviewSettingsScroll")) {
            scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            scroll->setMinimumSize(0, 0);
        }
    }
    for (auto button : findChildren<QPushButton *>()) button->setAutoDefault(false);
    button(QDialogButtonBox::Ok)->setDefault(true);
}

void ConfigDialog::showEvent(QShowEvent *event)
{
    KConfigDialog::showEvent(event);
    if (auto tabs = findChild<QTabWidget *>()) tabs->tabBar()->setFocus(Qt::OtherFocusReason);
}

void ConfigDialog::setCurrentPage(int page)
{
    switch (page) {
    case 1:
        return KPageDialog::setCurrentPage(mImageViewConfigPageItem);
    case 2:
        return KPageDialog::setCurrentPage(mAdvancedConfigPageItem);
    default:
        return KPageDialog::setCurrentPage(mGeneralConfigPageItem);
    }
}

} // namespace

#include "moc_configdialog.cpp"
