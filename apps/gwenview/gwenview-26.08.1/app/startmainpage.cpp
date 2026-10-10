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
#include "startmainpage.h"
#include "gvcore.h"
#include <lib/mimetypeutils.h>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <KLocalizedString>

namespace Gwenview
{
StartMainPage::StartMainPage(QWidget *parent, GvCore *core)
    : QFrame(parent), mCore(core)
{
    setObjectName(QStringLiteral("imageWelcomePage"));
    setFrameShape(QFrame::NoFrame);
    setAcceptDrops(true);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(48, 48, 48, 48);
    layout->setSpacing(16);
    layout->addStretch();
    auto icon = new QLabel(this);
    icon->setPixmap(QIcon::fromTheme(QStringLiteral("image-x-generic")).pixmap(72, 72));
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);
    auto title = new QLabel(i18n("Open an image"), this);
    QFont font = title->font();
    font.setPointSizeF(font.pointSizeF() * 1.8);
    font.setBold(true);
    title->setFont(font);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    auto text = new QLabel(i18n("Drag and drop images here, or choose files to get started."), this);
    text->setAlignment(Qt::AlignCenter);
    text->setWordWrap(true);
    layout->addWidget(text);
    auto button = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add files…"), this);
    button->setObjectName(QStringLiteral("addImagesButton"));
    button->setMinimumSize(160, 42);
    layout->addWidget(button, 0, Qt::AlignHCenter);
    connect(button, &QPushButton::clicked, this, &StartMainPage::addFilesRequested);
    layout->addStretch();
    setFocusProxy(button);
    loadConfig();
    connect(qApp, &QApplication::paletteChanged, this, [this] { loadConfig(); });
}

void StartMainPage::applyPalette(const QPalette &palette)
{
    setPalette(palette);
    setAutoFillBackground(false);
    update();
}

void StartMainPage::paintEvent(QPaintEvent *)
{
    // Match the viewer's 8px inset, 16px corners, and raised theme surface.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor surface = palette().color(QPalette::Button);
    surface.setAlpha(255);
    painter.setPen(Qt::NoPen);
    painter.setBrush(surface);
    painter.drawRoundedRect(QRectF(rect()).adjusted(8, 8, -8, -8), 16, 16);
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
    if (!imageUrls(event->mimeData()).isEmpty()) event->acceptProposedAction();
}

void StartMainPage::dropEvent(QDropEvent *event)
{
    const auto urls = imageUrls(event->mimeData());
    if (urls.isEmpty()) return;
    event->acceptProposedAction();
    Q_EMIT filesSelected(urls);
}
} // namespace Gwenview
#include "moc_startmainpage.cpp"
