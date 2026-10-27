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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "floatingsurface.h"
#include "sidebar.h"

// Qt
#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QTextLayout>
#include <QScrollBar>
#include <QTimer>
#include <QPointer>
#include <QVariantAnimation>
#include <QMenu>
#include <QScrollArea>
#include <QStackedLayout>
#include <QStyle>
#include <QStyleOptionTab>
#include <QToolButton>
#include <QVBoxLayout>

// KF
#include <KIconLoader>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/signalblocker.h>

namespace Gwenview
{
// Keep native QAction/menu behavior, with a wrapping, keyboard-accessible row.
class SideBarButton : public QToolButton
{
public:
    SideBarButton()
    {
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
    }

    QSize sizeHint() const override { return QSize(220, heightForWidth(220)); }
    QSize minimumSizeHint() const override { return QSize(64, 40); }
    int heightForWidth(int width) const override
    {
        return qMax(40, qCeil(textLayout(nullptr, qMax(16, width - 52))) + 20);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const auto group = isEnabled() ? QPalette::Active : QPalette::Disabled;
        const QColor ink = palette().color(group, QPalette::WindowText);
        QColor accent = palette().color(QPalette::Active, QPalette::Highlight);
        if (underMouse() || isDown() || isChecked() || hasFocus()) {
            QColor fill = accent; fill.setAlpha(isDown() ? 38 : 18);
            p.setBrush(fill);
            accent.setAlpha(hasFocus() ? 180 : 35);
            p.setPen(QPen(accent, hasFocus() ? 1.5 : 1));
            p.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 9, 9);
        }
        QPixmap pix = icon().pixmap(QSize(18, 18), devicePixelRatioF());
        if (!pix.isNull()) {
            QPainter tint(&pix); tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(pix.rect(), ink); tint.end();
            p.drawPixmap(QRect(10, (height() - 18) / 2, 18, 18), pix);
        }
        p.setPen(ink);
        p.save(); p.translate(38, (height() - textLayout(nullptr, qMax(16, width() - 52))) / 2);
        textLayout(&p, qMax(16, width() - 52)); p.restore();
        if (menu()) {
            p.setPen(QPen(ink, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const QPointF c(width() - 9, height() / 2.0);
            p.drawLine(c + QPointF(-2, -2), c + QPointF(1, 0));
            p.drawLine(c + QPointF(1, 0), c + QPointF(-2, 2));
        }
    }

private:
    qreal textLayout(QPainter *painter, int width) const
    {
        QString label;
        const QString source = text();
        for (int i = 0; i < source.size(); ++i) {
            if (source[i] == QLatin1Char('&')) {
                if (i + 1 < source.size() && source[i + 1] == QLatin1Char('&')) ++i;
                else continue;
            }
            label += source[i];
        }
        QTextLayout layout(label, font());
        QTextOption option; option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option);
        layout.beginLayout(); qreal y = 0;
        while (true) {
            auto line = layout.createLine(); if (!line.isValid()) break;
            line.setLineWidth(width); line.setPosition(QPointF(0, y)); y += line.height();
        }
        layout.endLayout();
        if (painter) layout.draw(painter, QPointF());
        return y;
    }
};

class SideBarEmptyMark : public QWidget
{
public:
    SideBarEmptyMark(bool information, QWidget *parent) : QWidget(parent), mInformation(information)
    { setFixedSize(64, 64); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QColor accent = palette().color(QPalette::Active, QPalette::Highlight);
        QColor fill = accent; fill.setAlpha(18);
        p.setPen(Qt::NoPen); p.setBrush(fill); p.drawRoundedRect(QRectF(1, 1, 62, 62), 18, 18);
        p.setBrush(Qt::NoBrush); p.setPen(QPen(accent, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (mInformation) {
            p.drawEllipse(QRectF(19, 19, 26, 26));
            p.drawPoint(QPointF(32, 25)); p.drawLine(QPointF(32, 31), QPointF(32, 39));
        } else {
            QPolygonF pencil; pencil << QPointF(21, 37) << QPointF(37, 21) << QPointF(43, 27)
                                     << QPointF(27, 43) << QPointF(20, 44) << QPointF(21, 37);
            p.drawPolyline(pencil); p.drawLine(QPointF(34, 24), QPointF(40, 30));
        }
    }
private:
    bool mInformation;
};

//- SideBarGroup ---------------------------------------------------------------
struct SideBarGroupPrivate {
    QFrame *mContainer = nullptr;
    QLabel *mTitleLabel = nullptr;
};

SideBarGroup::SideBarGroup(const QString &title)
    : QFrame()
    , d(new SideBarGroupPrivate)
{
    d->mContainer = nullptr;
    d->mTitleLabel = new QLabel(this);
    d->mTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font(d->mTitleLabel->font());
    font.setWeight(QFont::DemiBold);
    d->mTitleLabel->setFont(font);
    d->mTitleLabel->setText(title);
    d->mTitleLabel->setWordWrap(true);
    d->mTitleLabel->setVisible(!d->mTitleLabel->text().isEmpty());

    auto layout = new QVBoxLayout(this);
    layout->addWidget(d->mTitleLabel);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    clear();
}

void SideBarGroup::paintEvent(QPaintEvent *)
{
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QColor edge = palette().color(QPalette::WindowText); edge.setAlpha(18);
    QColor fill = palette().color(QPalette::WindowText); fill.setAlpha(4);
    p.setPen(QPen(edge, 1)); p.setBrush(fill);
    p.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), 12, 12);
}

SideBarGroup::~SideBarGroup()
{
    delete d;
}

void SideBarGroup::addWidget(QWidget *widget)
{
    widget->setParent(d->mContainer);
    d->mContainer->layout()->addWidget(widget);
}

void SideBarGroup::clear()
{
    if (d->mContainer) {
        layout()->removeWidget(d->mContainer);
        d->mContainer->hide();
        d->mContainer->deleteLater();
    }

    d->mContainer = new QFrame(this);
    auto containerLayout = new QVBoxLayout(d->mContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(2);

    layout()->addWidget(d->mContainer);
}

void SideBarGroup::addAction(QAction *action)
{
    int size = KIconLoader::global()->currentSize(KIconLoader::Small);
    QToolButton *button = new SideBarButton();
    button->setObjectName(QStringLiteral("sidebarAction_") + action->objectName());
    button->setAutoRaise(true);
    button->setDefaultAction(action);
    auto refresh = [button, action] {
        QString tip = action->toolTip();
        if (!action->shortcut().isEmpty()) tip += QStringLiteral(" (%1)").arg(action->shortcut().toString(QKeySequence::NativeText));
        button->setToolTip(tip);
        button->updateGeometry();
    };
    connect(action, &QAction::changed, button, refresh); refresh();
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIconSize(QSize(size, size));
    if (action->menu()) {
        button->setPopupMode(QToolButton::InstantPopup);
    }
    addWidget(button);
}

// QScrollArea otherwise uses minimum-size hints that can omit wrapped rows.
// Keep the full height-for-width of the page reachable through its scrollbar.
class SideBarScrollArea : public QScrollArea
{
public:
    explicit SideBarScrollArea(QWidget *parent) : QScrollArea(parent)
    {
        viewport()->installEventFilter(this);
    }
    void setPage(QWidget *page)
    {
        setWidget(page); page->installEventFilter(this); scheduleHeightUpdate();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if ((object == widget() && event->type() == QEvent::LayoutRequest)
            || (object == viewport() && event->type() == QEvent::Resize)) scheduleHeightUpdate();
        return QScrollArea::eventFilter(object, event);
    }
private:
    void scheduleHeightUpdate()
    {
        if (mPending) return;
        mPending = true;
        QTimer::singleShot(0, this, [this] {
            mPending = false;
            if (!widget() || !widget()->layout()) return;
            auto layout = widget()->layout();
            const int height = layout->hasHeightForWidth()
                ? layout->totalHeightForWidth(viewport()->width()) : layout->totalSizeHint().height();
            if (widget()->minimumHeight() != height) widget()->setMinimumHeight(qMax(0, height));
        });
    }
    bool mPending = false;
};

//- SideBarPage ----------------------------------------------------------------
struct SideBarPagePrivate {
    QIcon mIcon;
    QString mTitle;
    QVBoxLayout *mLayout = nullptr;
    QStackedLayout *mStack = nullptr;
    QWidget *mContent = nullptr;
    QWidget *mContentPage = nullptr;
    QWidget *mEmpty = nullptr;
};

SideBarPage::SideBarPage(const QIcon &icon, const QString &title)
    : QWidget()
    , d(new SideBarPagePrivate)
{
    d->mIcon = icon;
    d->mTitle = title;
    d->mStack = new QStackedLayout(this);
    d->mStack->setContentsMargins(0, 0, 0, 0);
    d->mContent = new QWidget(this);
    d->mContentPage = d->mContent;
    d->mLayout = new QVBoxLayout(d->mContent);
    d->mStack->addWidget(d->mContent);
    QMargins margins = d->mLayout->contentsMargins();
    margins.setRight(qMax(0, margins.right() - style()->pixelMetric(QStyle::PM_SplitterWidth)));
    d->mLayout->setContentsMargins(margins);
}

SideBarPage::~SideBarPage()
{
    delete d;
}

const QIcon &SideBarPage::icon() const
{
    return d->mIcon;
}

const QString &SideBarPage::title() const
{
    return d->mTitle;
}

void SideBarPage::addWidget(QWidget *widget)
{
    d->mLayout->addWidget(widget);
}

void SideBarPage::addStretch()
{
    d->mLayout->addStretch();
}

void SideBarPage::setContentMargins(const QMargins &margins)
{
    d->mLayout->setContentsMargins(margins);
}

void SideBarPage::setEmptyMessage(const QString &message)
{
    if (d->mEmpty) return;
    d->mLayout->setContentsMargins(8, 8, 8, 12);
    d->mLayout->setSpacing(14);
    auto makeHeading = [this](QWidget *parent) {
        auto heading = new QLabel(d->mTitle, parent);
        QFont font = heading->font(); font.setBold(true); heading->setFont(font);
        heading->setWordWrap(true);
        return heading;
    };
    d->mLayout->insertWidget(0, makeHeading(d->mContent));
    // Keep lengthy information and tool lists inside the page, above the footer.
    auto scroll = new SideBarScrollArea(this);
    scroll->setObjectName(objectName() + QStringLiteral("ScrollArea"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    d->mStack->removeWidget(d->mContent);
    scroll->setPage(d->mContent);
    d->mContent->setAutoFillBackground(false);
    d->mContentPage = scroll;
    d->mStack->addWidget(scroll);
    d->mEmpty = new QWidget(this);
    d->mEmpty->setObjectName(objectName() + QStringLiteral("EmptyState"));
    auto layout = new QVBoxLayout(d->mEmpty);
    layout->setContentsMargins(8, 8, 8, 16);
    layout->addWidget(makeHeading(d->mEmpty));
    layout->setSpacing(16);
    layout->addStretch(2);
    layout->addWidget(new SideBarEmptyMark(objectName() == QLatin1String("information"), d->mEmpty), 0, Qt::AlignHCenter);
    auto label = new QLabel(message, d->mEmpty);
    label->setObjectName(objectName() + QStringLiteral("EmptyMessage"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setContentsMargins(4, 0, 4, 0);
    layout->addWidget(label);
    layout->addStretch(3);
    d->mStack->addWidget(d->mEmpty);
}

void SideBarPage::setContextAvailable(bool available)
{
    if (d->mEmpty) d->mStack->setCurrentWidget(available ? d->mContentPage : d->mEmpty);
}

//- SideBarTabBar --------------------------------------------------------------
struct SideBarTabBarPrivate {
    SideBarTabBar::TabButtonStyle tabButtonStyle = SideBarTabBar::TabButtonTextUnderIcon;
};

SideBarTabBar::SideBarTabBar(QWidget *parent)
    : QTabBar(parent), d(new SideBarTabBarPrivate)
{
    setIconSize(QSize(20, 20));
    setElideMode(Qt::ElideNone);
    setMouseTracking(true);
}

SideBarTabBar::~SideBarTabBar() = default;
SideBarTabBar::TabButtonStyle SideBarTabBar::tabButtonStyle() const
{
    return width() < sizeHint(TabButtonTextUnderIcon).width() ? TabButtonIconOnly : TabButtonTextUnderIcon;
}

QSize SideBarTabBar::tabContentSize(int index, TabButtonStyle style, const QStyleOptionTab &opt) const
{
    if (index < 0 || index >= count()) return {};
    // KDE can add mnemonic markers after the tabs are created. Measure and
    // paint them with Qt's mnemonic handling, including escaped ampersands.
    const int labelWidth = style == TabButtonIconOnly ? 0
        : opt.fontMetrics.size(Qt::TextHideMnemonic, tabText(index)).width();
    return QSize(qMax(opt.iconSize.width(), labelWidth),
                 opt.iconSize.height() + opt.fontMetrics.height() + 5);
}

QSize SideBarTabBar::tabSizeHint(int index, TabButtonStyle style) const
{
    QStyleOptionTab opt; initStyleOption(&opt, index);
    int width = 0;
    for (int i = 0; i < count(); ++i) width = qMax(width, tabContentSize(i, style, opt).width());
    return QSize(width + 18, tabContentSize(index, style, opt).height() + 22);
}

QSize SideBarTabBar::sizeHint(TabButtonStyle style) const
{
    if (!count()) return {};
    const QSize tab = tabSizeHint(0, style);
    return QSize(tab.width() * count(), tab.height());
}
QSize SideBarTabBar::sizeHint() const { return sizeHint(TabButtonTextUnderIcon); }
QSize SideBarTabBar::minimumSizeHint() const { return sizeHint(TabButtonIconOnly); }
QSize SideBarTabBar::tabSizeHint(int index) const { return tabSizeHint(index, TabButtonTextUnderIcon); }
QSize SideBarTabBar::minimumTabSizeHint(int index) const { return tabSizeHint(index, TabButtonIconOnly); }
void SideBarTabBar::tabLayoutChange() { update(); }

void SideBarTabBar::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    for (int i = 0; i < count(); ++i) drawTab(i, painter);
}

void SideBarTabBar::drawTab(int index, QStylePainter &painter) const
{
    QStyleOptionTab opt; initStyleOption(&opt, index);
    const bool selected = opt.state & QStyle::State_Selected;
    const bool hovered = opt.state & QStyle::State_MouseOver;
    const QRectF capsule = QRectF(opt.rect).adjusted(4, 5, -4, -8);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    if (selected || hovered) {
        QColor accent = opt.palette.color(QPalette::Active, QPalette::Highlight);
        QColor fill = opt.palette.color(QPalette::Button);
        if (selected) paintFloatingShadow(painter, capsule, 12, opt.palette);
        painter.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), selected ? 55 : 20), 1));
        painter.setBrush(fill);
        painter.drawRoundedRect(capsule, 12, 12);
        accent.setAlpha(selected ? 24 : 12);
        painter.setPen(Qt::NoPen); painter.setBrush(accent);
        painter.drawRoundedRect(capsule, 12, 12);
    }
    if (hasFocus() && selected) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(opt.palette.color(QPalette::Highlight), 2));
        painter.drawRoundedRect(capsule.adjusted(1, 1, -1, -1), 11, 11);
    }
    const QColor ink = selected ? opt.palette.color(QPalette::Active, QPalette::Highlight)
                                : opt.palette.color(QPalette::WindowText);
    const bool showLabel = tabButtonStyle() != TabButtonIconOnly;
    const QRect target(opt.rect.center().x() - 10,
                       showLabel ? opt.rect.top() + 12 : qRound(capsule.center().y()) - 10, 20, 20);
    if (tabData(index).toString() == QLatin1String("recent")) {
        // A theme-independent history symbol, using the same palette as the other tabs.
        painter.save();
        painter.translate(target.topLeft());
        painter.setPen(QPen(ink, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawArc(QRectF(3, 3, 14, 14), 135 * 16, -315 * 16);
        painter.drawLine(QPointF(3, 2.5), QPointF(3, 6.5));
        painter.drawLine(QPointF(3, 6.5), QPointF(7, 6.5));
        painter.drawLine(QPointF(10, 6), QPointF(10, 10));
        painter.drawLine(QPointF(10, 10), QPointF(13, 12));
        painter.restore();
    } else {
        QPixmap icon = opt.icon.pixmap(opt.iconSize, devicePixelRatioF());
        if (!icon.isNull()) {
            QPainter tint(&icon); tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(icon.rect(), ink); tint.end();
            painter.drawPixmap(target, icon);
        }
    }
    if (showLabel) {
        QFont labelFont = font(); labelFont.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        painter.setFont(labelFont);
        painter.setPen(opt.palette.color(QPalette::WindowText));
        painter.drawText(opt.rect.adjusted(4, 36, -4, -10),
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextHideMnemonic, tabText(index));
    }
    painter.restore();
}

//- SideBar --------------------------------------------------------------------
class SideBarTabs : public QTabWidget
{
public:
    explicit SideBarTabs(QWidget *parent) : QTabWidget(parent)
    {
        setObjectName(QStringLiteral("sidebarTabs"));
        setTabBar(new SideBarTabBar(this));
        setDocumentMode(true);
        setAutoFillBackground(false);
        tabBar()->setDocumentMode(true);
        tabBar()->setUsesScrollButtons(false);
        tabBar()->setFocusPolicy(Qt::StrongFocus);
        tabBar()->setExpanding(true);
        tabBar()->setDrawBase(false);
        tabBar()->setAutoFillBackground(false);
        setTabPosition(QTabWidget::South);
        mSlide = new QVariantAnimation(this);
        mSlide->setObjectName(QStringLiteral("sidebarSlideAnimation"));
        mSlide->setDuration(140);
        mSlide->setEasingCurve(QEasingCurve::OutCubic);
        mSlide->setStartValue(0.0);
        mSlide->setEndValue(1.0);
        connect(mSlide, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            if (!mIncoming || !mOutgoing) return;
            const int shift = qRound(mPageGeometry.width() * value.toReal());
            mOutgoing->move(mPageGeometry.topLeft() - QPoint(mDirection * shift, 0));
            mIncoming->move(mPageGeometry.topLeft() + QPoint(mDirection * (mPageGeometry.width() - shift), 0));
        });
        connect(mSlide, &QVariantAnimation::finished, this, [this] { finishSlide(); });
        connect(this, &QTabWidget::currentChanged, this, [this](int index) { slideTo(index); });
    }
protected:
    void showEvent(QShowEvent *event) override
    {
        finishSlide();
        QTabWidget::showEvent(event);
        mLastPage = currentWidget();
    }
    void hideEvent(QHideEvent *event) override
    {
        finishSlide();
        QTabWidget::hideEvent(event);
    }
    void resizeEvent(QResizeEvent *event) override
    {
        finishSlide();
        QTabWidget::resizeEvent(event);
    }
    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::StyleChange || event->type() == QEvent::LayoutDirectionChange)
            finishSlide();
        QTabWidget::changeEvent(event);
    }
private:
    void finishSlide()
    {
        if (!mSlide) return;
        mSlide->stop();
        if (mOutgoing) {
            mOutgoing->setAttribute(Qt::WA_TransparentForMouseEvents, mOutgoingMouseTransparent);
            if (mOutgoing != currentWidget()) mOutgoing->hide();
            mOutgoing->move(mPageGeometry.topLeft());
        }
        if (mIncoming) mIncoming->move(mPageGeometry.topLeft());
        mOutgoing.clear();
        mIncoming.clear();
    }
    void slideTo(int index)
    {
        // Retarget immediately on rapid clicks; never queue transitions.
        finishSlide();
        QWidget *previous = mLastPage;
        QWidget *next = widget(index);
        mLastPage = next;
        if (!previous || !next || previous == next || !isVisible()
            || style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) == 0) return;
        mPageGeometry = next->geometry();
        if (mPageGeometry.isEmpty()) return;
        mDirection = index > indexOf(previous) ? 1 : -1;
        if (isRightToLeft()) mDirection = -mDirection;
        mOutgoing = previous;
        mIncoming = next;
        mOutgoingMouseTransparent = previous->testAttribute(Qt::WA_TransparentForMouseEvents);
        previous->setAttribute(Qt::WA_TransparentForMouseEvents);
        previous->setGeometry(mPageGeometry);
        previous->show();
        next->move(mPageGeometry.topLeft() + QPoint(mDirection * mPageGeometry.width(), 0));
        next->raise();
        // Live pages remain clipped by their stack; the footer never moves.
        mSlide->start();
    }
    QVariantAnimation *mSlide = nullptr;
    QPointer<QWidget> mLastPage;
    QPointer<QWidget> mOutgoing;
    QPointer<QWidget> mIncoming;
    QRect mPageGeometry;
    int mDirection = 1;
    bool mOutgoingMouseTransparent = false;
};

struct SideBarPrivate {
    SideBarTabs *tabs = nullptr;
};

SideBar::SideBar(QWidget *parent)
    : QWidget(parent)
    , d(new SideBarPrivate)
{
    setObjectName(QStringLiteral("sidebarIsland"));
    setAutoFillBackground(false);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(FloatingPanelPadding, FloatingPanelPadding, FloatingPanelPadding, FloatingPanelPadding);
    layout->setSpacing(0);
    d->tabs = new SideBarTabs(this);
    layout->addWidget(d->tabs);
    connect(d->tabs, &QTabWidget::currentChanged, this, [this](int index) {
        GwenviewConfig::setSideBarPage(currentPage());
        Q_EMIT currentChanged(index);
    });
}

SideBar::~SideBar()
{
    delete d;
}

void SideBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    paintFloatingPanel(painter, QRectF(rect()).adjusted(FloatingPanelInset, FloatingPanelInset, -FloatingPanelInset, -FloatingPanelInset), palette());
}

QSize SideBar::sizeHint() const
{
    return QSize(qMax(280, d->tabs->tabBar()->sizeHint().width() + 2 * FloatingPanelPadding), 200);
}

void SideBar::addPage(SideBarPage *page)
{
    // Prevent emitting currentChanged() while populating pages
    SignalBlocker blocker(d->tabs);
    const int index = d->tabs->addTab(page, page->icon(), page->title());
    d->tabs->setTabToolTip(index, page->title());
    d->tabs->tabBar()->setTabData(index, page->objectName());
    setMinimumWidth(d->tabs->tabBar()->minimumSizeHint().width() + 2 * FloatingPanelPadding);
}

void SideBar::setContextAvailable(bool available)
{
    for (int i = 0; i < d->tabs->count(); ++i)
        static_cast<SideBarPage *>(d->tabs->widget(i))->setContextAvailable(available);
}

QString SideBar::currentPage() const
{
    return d->tabs->currentWidget()->objectName();
}

void SideBar::setCurrentPage(const QString &name)
{
    for (int index = 0; index < d->tabs->count(); ++index) {
        if (d->tabs->widget(index)->objectName() == name) {
            d->tabs->setCurrentIndex(index);
        }
    }
}

void SideBar::loadConfig()
{
    const auto page = GwenviewConfig::sideBarPage();
    setCurrentPage(page == QLatin1String("folders") ? QStringLiteral("recent") : page);
}

} // namespace

#include "moc_sidebar.cpp"
