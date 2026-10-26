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
/**
 * A button which always leave room for an icon, even if there is none, so that
 * all button texts are correctly aligned.
 */
class SideBarButton : public QToolButton
{
protected:
    void paintEvent(QPaintEvent *event) override
    {
        forceIcon();
        QToolButton::paintEvent(event);
    }

    QSize sizeHint() const override
    {
        const_cast<SideBarButton *>(this)->forceIcon();
        return QToolButton::sizeHint();
    }

private:
    void forceIcon()
    {
        if (!icon().isNull()) {
            return;
        }

        // Assign an empty icon to the button if there is no icon associated
        // with the action so that all button texts are correctly aligned.
        QSize wantedSize = iconSize();
        if (mEmptyIcon.isNull() || mEmptyIcon.actualSize(wantedSize) != wantedSize) {
            QPixmap pix(wantedSize);
            pix.fill(Qt::transparent);
            mEmptyIcon.addPixmap(pix);
        }
        setIcon(mEmptyIcon);
    }

    QIcon mEmptyIcon;
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
    font.setPointSizeF(font.pointSizeF() + 1);
    d->mTitleLabel->setFont(font);
    d->mTitleLabel->setText(title);
    d->mTitleLabel->setVisible(!d->mTitleLabel->text().isEmpty());

    auto layout = new QVBoxLayout(this);
    layout->addWidget(d->mTitleLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    clear();
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
        d->mContainer->deleteLater();
    }

    d->mContainer = new QFrame(this);
    auto containerLayout = new QVBoxLayout(d->mContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    layout()->addWidget(d->mContainer);
}

void SideBarGroup::addAction(QAction *action)
{
    int size = KIconLoader::global()->currentSize(KIconLoader::Small);
    QToolButton *button = new SideBarButton();
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    button->setAutoRaise(true);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIconSize(QSize(size, size));
    if (action->menu()) {
        button->setPopupMode(QToolButton::InstantPopup);
    }
    addWidget(button);
}

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
    // Keep lengthy information and tool lists inside the page, above the footer.
    auto scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);
    d->mStack->removeWidget(d->mContent);
    scroll->setWidget(d->mContent);
    d->mContent->setAutoFillBackground(false);
    d->mContentPage = scroll;
    d->mStack->addWidget(scroll);
    d->mEmpty = new QWidget(this);
    d->mEmpty->setObjectName(objectName() + QStringLiteral("EmptyState"));
    auto layout = new QVBoxLayout(d->mEmpty);
    layout->setContentsMargins(16, 28, 16, 28);
    layout->setSpacing(16);
    layout->addStretch(2);
    auto label = new QLabel(message, d->mEmpty);
    label->setObjectName(objectName() + QStringLiteral("EmptyMessage"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
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
    }
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
    layout->setContentsMargins(16, 16, 16, 16);
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
    paintFloatingPanel(painter, QRectF(rect()).adjusted(8, 8, -8, -8), palette());
}

QSize SideBar::sizeHint() const
{
    return QSize(qMax(280, d->tabs->tabBar()->sizeHint().width() + 32), 200);
}

void SideBar::addPage(SideBarPage *page)
{
    // Prevent emitting currentChanged() while populating pages
    SignalBlocker blocker(d->tabs);
    const int index = d->tabs->addTab(page, page->icon(), page->title());
    d->tabs->setTabToolTip(index, page->title());
    d->tabs->tabBar()->setTabData(index, page->objectName());
    setMinimumWidth(d->tabs->tabBar()->minimumSizeHint().width() + 32);
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
