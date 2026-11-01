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
#include "floatingsurface.h"
#include "startmainpage.h"
#include "gvcore.h"
#include <lib/mimetypeutils.h>
#include <QApplication>
#include <QAction>
#include <QHideEvent>
#include <QLinearGradient>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <KLocalizedString>

namespace Gwenview
{
namespace {
class WelcomeOpenRow : public QWidget
{
public:
    WelcomeOpenRow(QPushButton *button, QLabel *shortcut, QWidget *parent)
        : QWidget(parent), mButton(button), mShortcut(shortcut)
    {
        button->setParent(this); shortcut->setParent(this);
        auto layout = new QHBoxLayout(this); layout->setContentsMargins(0, 0, 0, 0);
        layout->addStretch(); layout->addWidget(button); layout->addStretch();
    }
    QSize sizeHint() const override
    {
        const int shortcutWidth = mShortcut->fontMetrics().horizontalAdvance(mShortcut->property("fullShortcut").toString()) + 8;
        return QWidget::sizeHint().expandedTo(QSize(qMax(mButton->minimumWidth(), mButton->sizeHint().width()) + 2 * (shortcutWidth + 12), 44));
    }
    void placeShortcut()
    {
        const int left = mButton->geometry().right() + 13;
        const int available = qMax(0, width() - left);
        mShortcut->setFixedWidth(available);
        mShortcut->setFixedHeight(mShortcut->sizeHint().height());
        mShortcut->move(left, (height() - mShortcut->height()) / 2);
        const QString full = mShortcut->property("fullShortcut").toString();
        mShortcut->setText(mShortcut->fontMetrics().elidedText(full, Qt::ElideRight, available));
        mShortcut->setToolTip(full);
        mShortcut->setVisible(!full.isEmpty() && available > 0);
    }
protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event); layout()->activate(); placeShortcut();
    }
private:
    QPushButton *mButton;
    QLabel *mShortcut;
};

class ImageStackMark : public QWidget {
public:
    explicit ImageStackMark(QWidget *parent) : QWidget(parent) { setFixedSize(112, 96); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QColor accent = palette().color(QPalette::Active, QPalette::Highlight);
        QColor back = accent; back.setAlpha(28);
        QColor edge = accent; edge.setAlpha(65);
        painter.translate(56, 48);
        painter.save(); painter.rotate(-13);
        painter.setPen(QPen(edge, 1.5)); painter.setBrush(back);
        painter.drawRoundedRect(QRectF(-37, -27, 74, 54), 10, 10);
        painter.restore();
        const QRectF card(-32, -26, 76, 56);
        paintFloatingShadow(painter, card, 10, palette());
        QColor surface = palette().color(QPalette::Button); surface.setAlpha(255);
        painter.setPen(QPen(edge, 1.5)); painter.setBrush(surface);
        painter.drawRoundedRect(card, 10, 10);
        painter.setPen(Qt::NoPen); painter.setBrush(accent);
        painter.drawEllipse(QPointF(22, -10), 5, 5);
        QPainterPath hills; hills.moveTo(-23, 20); hills.lineTo(-6, -1);
        hills.quadTo(-3, -4, 0, 0); hills.lineTo(9, 10); hills.lineTo(15, 4);
        hills.quadTo(17, 2, 20, 5); hills.lineTo(34, 20); hills.closeSubpath();
        QColor mountain = accent; mountain.setAlpha(180); painter.fillPath(hills, mountain);
    }
};
}

StartMainPage::StartMainPage(QWidget *parent, GvCore *core)
    : QFrame(parent), mCore(core)
{
    setObjectName(QStringLiteral("imageWelcomePage"));
    setFrameShape(QFrame::NoFrame);
    setAcceptDrops(true);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(0);
    layout->addStretch(4);
    auto group = new QWidget(this);
    group->setObjectName(QStringLiteral("welcomeContent"));
    group->setMaximumWidth(520);
    auto content = new QVBoxLayout(group);
    content->setContentsMargins(12, 12, 12, 12);
    content->setSpacing(18);
    auto mark = new ImageStackMark(group);
    mark->setObjectName(QStringLiteral("welcomeImageStack"));
    content->addWidget(mark, 0, Qt::AlignHCenter);
    mTitle = new QLabel(i18n("Open your images"), group);
    mTitle->setObjectName(QStringLiteral("welcomeTitle"));
    QFont font = mTitle->font();
    font.setPointSizeF(font.pointSizeF() * 1.9);
    font.setBold(true);
    mTitle->setFont(font); mTitle->setAlignment(Qt::AlignCenter);
    mTitle->setWordWrap(true);
    content->addWidget(mTitle);
    mDescription = new QLabel(i18n("Drop images here, choose files, or pick up where you left off in Recent."), group);
    mDescription->setAlignment(Qt::AlignCenter); mDescription->setWordWrap(true);
    content->addWidget(mDescription);
    content->addSpacing(4);
    auto button = new QPushButton(i18n("Open images…"), group);
    button->setObjectName(QStringLiteral("addImagesButton"));
    button->setMinimumSize(184, 44);
    mShortcut = new QLabel(group);
    mShortcut->setObjectName(QStringLiteral("openImagesShortcut"));
    content->addWidget(new WelcomeOpenRow(button, mShortcut, group));
    auto browse = new QPushButton(i18n("Browse a folder…"), group);
    browse->setObjectName(QStringLiteral("browseFolderButton"));
    browse->setFlat(true); browse->setMinimumHeight(36);
    content->addWidget(browse, 0, Qt::AlignHCenter);
    layout->addWidget(group, 0, Qt::AlignHCenter);
    layout->addStretch(5);
    connect(button, &QPushButton::clicked, this, &StartMainPage::addFilesRequested);
    connect(browse, &QPushButton::clicked, this, &StartMainPage::browseFolderRequested);
    setFocusProxy(button);
    loadConfig();
    connect(qApp, &QApplication::paletteChanged, this, [this] { loadConfig(); });
}

void StartMainPage::setOpenAction(QAction *action)
{
    auto refresh = [this, action] {
        const QString shortcut = action->shortcut().toString(QKeySequence::NativeText);
        mShortcut->setProperty("fullShortcut", shortcut);
        mShortcut->parentWidget()->updateGeometry();
        static_cast<WelcomeOpenRow *>(mShortcut->parentWidget())->placeShortcut();
        mShortcut->setAccessibleName(i18n("Open images shortcut: %1", shortcut));
    };
    connect(action, &QAction::changed, this, refresh);
    refresh();
}

void StartMainPage::setDropActive(bool active)
{
    if (mDropActive == active) return;
    mDropActive = active;
    setProperty("dropActive", active);
    mTitle->setText(active ? i18n("Drop to open") : i18n("Open your images"));
    mDescription->setText(active ? i18n("Release to open your images.")
        : i18n("Drop images here, choose files, or pick up where you left off in Recent."));
    update();
}

void StartMainPage::applyPalette(const QPalette &palette)
{
    setPalette(palette);
    setAutoFillBackground(false);
    QPalette muted = palette;
    QColor ink = palette.color(QPalette::WindowText); ink.setAlpha(180);
    muted.setColor(QPalette::WindowText, ink);
    mDescription->setPalette(muted); mShortcut->setPalette(muted);
    const QColor accent = palette.color(QPalette::Active, QPalette::Highlight);
    const QColor text = palette.color(QPalette::Active, QPalette::HighlightedText);
    findChild<QPushButton *>(QStringLiteral("addImagesButton"))->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1; border-radius: 12px; padding: 4px 16px; font-weight: 600; }"
        "QPushButton:hover { background: %3; } QPushButton:pressed { background: %4; }"
        "QPushButton:focus { border: 2px solid %2; }")
        .arg(accent.name(), text.name(), accent.lighter(108).name(), accent.darker(110).name()));
    update();
}

void StartMainPage::paintEvent(QPaintEvent *)
{
    // Match the viewer's inset, rounded corners, and raised theme surface.
    QPainter painter(this);
    paintFloatingPanel(painter, QRectF(rect()).adjusted(FloatingPanelInset, FloatingPanelInset, -FloatingPanelInset, -FloatingPanelInset), palette());
    if (mDropActive) {
        QColor accent = palette().color(QPalette::Active, QPalette::Highlight);
        QColor tint = accent; tint.setAlpha(16);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(accent, 2.5)); painter.setBrush(tint);
        painter.drawRoundedRect(QRectF(rect()).adjusted(FloatingPanelInset + 2, FloatingPanelInset + 2, -FloatingPanelInset - 2, -FloatingPanelInset - 2), Sequoia::SurfaceRadius - 1, Sequoia::SurfaceRadius - 1);
    }
}

void StartMainPage::loadConfig()
{
    applyPalette(mCore->palette(GvCore::NormalViewPalette));
}

static QList<QUrl> imageUrls(const QMimeData *mime)
{
    QList<QUrl> urls;
    for (const QUrl &url : mime->urls()) {
        const auto kind = MimeTypeUtils::urlKind(url);
        if ((kind == MimeTypeUtils::KIND_RASTER_IMAGE || kind == MimeTypeUtils::KIND_SVG_IMAGE) && !urls.contains(url))
            urls.append(url);
    }
    return urls;
}

void StartMainPage::dragEnterEvent(QDragEnterEvent *event)
{
    setDropActive(!imageUrls(event->mimeData()).isEmpty());
    if (mDropActive) event->acceptProposedAction();
    else event->ignore();
}

void StartMainPage::dragMoveEvent(QDragMoveEvent *event)
{
    if (mDropActive) event->acceptProposedAction();
}

void StartMainPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    setDropActive(false); event->accept();
}

void StartMainPage::hideEvent(QHideEvent *event)
{
    setDropActive(false); QFrame::hideEvent(event);
}

void StartMainPage::dropEvent(QDropEvent *event)
{
    const auto urls = imageUrls(event->mimeData());
    setDropActive(false);
    if (urls.isEmpty()) return;
    event->acceptProposedAction();
    Q_EMIT filesSelected(urls);
}
} // namespace Gwenview
#include "moc_startmainpage.cpp"
