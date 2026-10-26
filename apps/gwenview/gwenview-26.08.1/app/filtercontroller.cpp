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
#include "filtercontroller.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QResizeEvent>
#include <QToolButton>

// KF
#include <KComboBox>
#include <KIconLoader>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include <chrono>
#include <lib/flowlayout.h>
#include <lib/paintutils.h>

using namespace std::chrono_literals;

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// KF

// Local
#endif

namespace Gwenview
{
NameFilterWidget::NameFilterWidget(SortedDirModel *model)
    : mFilter(new NameFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Name contains"), QVariant(NameFilter::Contains));
    mModeComboBox->addItem(i18n("Name does not contain"), QVariant(NameFilter::DoesNotContain));

    mLineEdit = new QLineEdit;
    mLineEdit->setPlaceholderText(i18n("Filter by filename…"));
    mLineEdit->setAccessibleName(i18n("Filter by filename"));

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mLineEdit);

    auto timer = new QTimer(this);
    timer->setInterval(350ms);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &NameFilterWidget::applyNameFilter);

    connect(mLineEdit, SIGNAL(textChanged(QString)), timer, SLOT(start()));

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(applyNameFilter()));

    QTimer::singleShot(0, mLineEdit, SLOT(setFocus()));
}

NameFilterWidget::~NameFilterWidget()
{
    delete mFilter;
}

void NameFilterWidget::applyNameFilter()
{
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(NameFilter::Mode(data.toInt()));
    mFilter->setText(mLineEdit->text());
}

DateFilterWidget::DateFilterWidget(SortedDirModel *model)
    : mFilter(new DateFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Date >="), DateFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Date ="), DateFilter::Equal);
    mModeComboBox->addItem(i18n("Date <="), DateFilter::LessOrEqual);

    mDateWidget = new DateWidget;

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mDateWidget);

    connect(mDateWidget, &DateWidget::dateChanged, this, &DateFilterWidget::applyDateFilter);
    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(applyDateFilter()));

    applyDateFilter();
}

DateFilterWidget::~DateFilterWidget()
{
    delete mFilter;
}

void DateFilterWidget::applyDateFilter()
{
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(DateFilter::Mode(data.toInt()));
    mFilter->setDate(mDateWidget->date());
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
RatingFilterWidget::RatingFilterWidget(SortedDirModel *model)
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Rating >="), RatingFilter::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Rating ="), RatingFilter::Equal);
    mModeComboBox->addItem(i18n("Rating <="), RatingFilter::LessOrEqual);

    mRatingWidget = new KRatingWidget;
    mRatingWidget->setHalfStepsEnabled(true);
    mRatingWidget->setMaxRating(10);
    auto updateStar = [this] {
        QPixmap star = QIcon::fromTheme(QStringLiteral("rating")).pixmap(18, 18);
        if (!star.isNull()) {
            QPainter painter(&star); painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(star.rect(), palette().color(QPalette::Active, QPalette::Highlight)); painter.end();
            mRatingWidget->setCustomPixmap(star);
        }
    };
    connect(qApp, &QApplication::paletteChanged, this, updateStar); updateStar();

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mRatingWidget);

    mFilter = new RatingFilter(model);

    QObject::connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateFilterMode()));

    QObject::connect(mRatingWidget, SIGNAL(ratingChanged(int)), SLOT(slotRatingChanged(int)));

    updateFilterMode();
}

RatingFilterWidget::~RatingFilterWidget()
{
    delete mFilter;
}

void RatingFilterWidget::slotRatingChanged(int value)
{
    mFilter->setRating(value);
}

void RatingFilterWidget::updateFilterMode()
{
    const QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    mFilter->setMode(RatingFilter::Mode(data.toInt()));
}

TagFilterWidget::TagFilterWidget(SortedDirModel *model)
    : mFilter(new TagFilter(model))
{
    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Tagged"), QVariant(true));
    mModeComboBox->addItem(i18n("Not Tagged"), QVariant(false));

    mTagComboBox = new QComboBox;

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    layout->addWidget(mTagComboBox);

    AbstractSemanticInfoBackEnd *backEnd = model->semanticInfoBackEnd();
    backEnd->refreshAllTags();
    TagModel *tagModel = TagModel::createAllTagsModel(this, backEnd);

    auto completer = new QCompleter(mTagComboBox);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModel(tagModel);
    mTagComboBox->setCompleter(completer);
    mTagComboBox->setInsertPolicy(QComboBox::NoInsert);
    mTagComboBox->setEditable(true);
    mTagComboBox->setModel(tagModel);
    mTagComboBox->setCurrentIndex(-1);

    connect(mTagComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()));

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()));

    QTimer::singleShot(0, mTagComboBox, SLOT(setFocus()));
}

TagFilterWidget::~TagFilterWidget()
{
    delete mFilter;
}

void TagFilterWidget::updateTagSetFilter()
{
    QModelIndex index = mTagComboBox->model()->index(mTagComboBox->currentIndex(), 0);
    if (!index.isValid()) {
        qCWarning(GWENVIEW_APP_LOG) << "Invalid index";
        return;
    }
    SemanticInfoTag tag = index.data(TagModel::TagRole).toString();
    mFilter->setTag(tag);

    bool wantMatchingTag = mModeComboBox->itemData(mModeComboBox->currentIndex()).toBool();
    mFilter->setWantMatchingTag(wantMatchingTag);
}
#endif

ImageDimensionsWidget::ImageDimensionsWidget(SortedDirModel *model)
{
    mFilter = new ImageDimensions(model);

    mModeComboBox = new KComboBox;
    mModeComboBox->addItem(i18n("Dimensions >="), ImageDimensions::GreaterOrEqual);
    mModeComboBox->addItem(i18n("Dimensions ="), ImageDimensions::Equal);
    mModeComboBox->addItem(i18n("Dimensions <="), ImageDimensions::LessOrEqual);

    mLineEditHeight = new QLineEdit;
    mLineEditHeight->setValidator(new QIntValidator(ImageDimensions::minImageSizePx, ImageDimensions::maxImageSizePx, this));
    mLineEditWidth = new QLineEdit;
    mLineEditWidth->setValidator(new QIntValidator(ImageDimensions::minImageSizePx, ImageDimensions::maxImageSizePx, this));
    mLabelHeight = new QLabel;
    mLabelHeight->setText(i18n("Height"));
    mLabelWidth = new QLabel;
    mLabelWidth->setText(i18n("Width"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mModeComboBox);
    auto addDimension = [this, layout](QLabel *label, QLineEdit *edit) {
        auto pair = new QWidget(this);
        auto row = new QHBoxLayout(pair); row->setContentsMargins(0, 0, 0, 0); row->setSpacing(6);
        row->setSizeConstraint(QLayout::SetNoConstraint);
        edit->setMinimumWidth(0); edit->setPlaceholderText(i18n("Pixels")); edit->setAccessibleName(label->text());
        row->addWidget(label); row->addWidget(edit, 1);
        layout->addWidget(pair);
    };
    addDimension(mLabelWidth, mLineEditWidth);
    addDimension(mLabelHeight, mLineEditHeight);

    connect(mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(applyImageDimensions()));
    connect(mLineEditHeight, &QLineEdit::editingFinished, this, &ImageDimensionsWidget::applyImageDimensions);
    connect(mLineEditWidth, &QLineEdit::editingFinished, this, &ImageDimensionsWidget::applyImageDimensions);

    applyImageDimensions();
}

ImageDimensionsWidget::~ImageDimensionsWidget()
{
    delete mFilter;
}

void ImageDimensionsWidget::applyImageDimensions()
{
    QVariant data = mModeComboBox->itemData(mModeComboBox->currentIndex());
    uint32_t width = 0;
    uint32_t height = 0;

    width = mLineEditWidth->text().toUInt();
    height = mLineEditHeight->text().toUInt();

    mFilter->setMode(ImageDimensions::Mode(data.toInt()));
    mFilter->setSizes(width, height);
}

/**
 * A container for all filter widgets. It features a close button on the right.
 */
class FilterWidgetContainer : public QFrame
{
public:
    FilterWidgetContainer()
    {
        setObjectName(QStringLiteral("browseFilterCard"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    QSize minimumSizeHint() const override { return QSize(120, sizeHint().height()); }
    QSize sizeHint() const override { return QSize(300, layout() ? layout()->sizeHint().height() : 40); }

    void setFilterWidget(QWidget *widget)
    {
        mFilterWidget = widget;
        widget->setMinimumWidth(0);
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        widget->layout()->setSizeConstraint(QLayout::SetNoConstraint);
        for (auto field : widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
            field->setMinimumWidth(0);
            field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            if (auto line = qobject_cast<QLineEdit *>(field)) {
                line->setClearButtonEnabled(true);
                if (line->placeholderText().isEmpty()) line->setPlaceholderText(i18n("Value"));
            }
        }
        auto closeButton = new QToolButton;
        closeButton->setObjectName(QStringLiteral("removeBrowseFilter"));
        closeButton->setText(QStringLiteral("×"));
        closeButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        closeButton->setToolTip(i18n("Remove this filter"));
        closeButton->setAccessibleName(i18n("Remove this filter"));
        closeButton->setFocusPolicy(Qt::StrongFocus);
        closeButton->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
        closeButton->setAutoRaise(true);
        closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        int size = style()->pixelMetric(QStyle::PM_SmallIconSize);
        closeButton->setIconSize(QSize(size, size));
        connect(closeButton, &QAbstractButton::clicked, this, &QObject::deleteLater);
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);
        layout->setSizeConstraint(QLayout::SetNoConstraint);
        layout->addWidget(widget);
        layout->addWidget(closeButton, 0, Qt::AlignTop);
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QFrame::resizeEvent(event);
        auto row = qobject_cast<QBoxLayout *>(mFilterWidget->layout());
        if (!row) return;
        int wanted = 0;
        for (int i = 0; i < row->count(); ++i) {
            auto field = row->itemAt(i)->widget();
            if (field) wanted += field->sizeHint().width() + 6;
        }
        const auto direction = wanted > width() - 50 ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;
        row->setDirection(direction);
        row->setSpacing(6);
        row->invalidate(); layout()->invalidate();
        setMinimumHeight(layout()->sizeHint().height());
        updateGeometry();
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QPainterPath path = PaintUtils::roundedRectangle(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10);

        const QColor color = palette().color(QPalette::Active, QPalette::Highlight);
        painter.fillPath(path, PaintUtils::alphaAdjustedF(color, 0.05));
        painter.setPen(PaintUtils::alphaAdjustedF(color, 0.22));
        painter.drawPath(path);
    }
private:
    QWidget *mFilterWidget = nullptr;
};

FilterController::FilterController(QFrame *frame, SortedDirModel *dirModel)
    : QObject(frame)
{
    q = this;
    mFrame = frame;
    mDirModel = dirModel;
    mFilterWidgetCount = 0;

    mFrame->hide();
    auto layout = new QVBoxLayout(mFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->setAlignment(Qt::AlignTop);

    addAction(i18nc("@action:inmenu", "Filter by Name"), SLOT(addFilterByName()), QKeySequence(Qt::CTRL | Qt::Key_I));

    addAction(i18nc("@action:inmenu", "Filter by Date"), SLOT(addFilterByDate()), QKeySequence());

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    addAction(i18nc("@action:inmenu", "Filter by Rating"), SLOT(addFilterByRating()), QKeySequence());

    addAction(i18nc("@action:inmenu", "Filter by Tag"), SLOT(addFilterByTag()), QKeySequence());
#endif
    addAction(i18nc("@action:inmenu", "Filter by Dimensions"), SLOT(addFilterByImageDimensions()), QKeySequence());
}

QList<QAction *> FilterController::actionList() const
{
    return mActionList;
}

void FilterController::addFilterByName()
{
    addFilter(new NameFilterWidget(mDirModel));
}

void FilterController::addFilterByDate()
{
    addFilter(new DateFilterWidget(mDirModel));
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
void FilterController::addFilterByRating()
{
    addFilter(new RatingFilterWidget(mDirModel));
}

void FilterController::addFilterByTag()
{
    addFilter(new TagFilterWidget(mDirModel));
}
#endif

void FilterController::addFilterByImageDimensions()
{
    addFilter(new ImageDimensionsWidget(mDirModel));
}

void FilterController::slotFilterWidgetClosed()
{
    mFilterWidgetCount--;
    if (mFilterWidgetCount == 0) {
        mFrame->hide();
    }
}

void FilterController::addAction(const QString &text, const char *slot, const QKeySequence &shortcut)
{
    auto action = new QAction(text, q);
    action->setShortcut(shortcut);
    QObject::connect(action, SIGNAL(triggered()), q, slot);
    mActionList << action;
}

void FilterController::addFilter(QWidget *widget)
{
    if (mFrame->isHidden()) {
        mFrame->show();
    }
    auto container = new FilterWidgetContainer;
    container->setFilterWidget(widget);
    mFrame->layout()->addWidget(container);

    mFilterWidgetCount++;
    QObject::connect(container, &QObject::destroyed, q, &FilterController::slotFilterWidgetClosed);
}

} // namespace

#include "moc_filtercontroller.cpp"
