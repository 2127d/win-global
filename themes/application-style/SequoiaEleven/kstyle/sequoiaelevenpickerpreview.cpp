// SPDX-License-Identifier: GPL-2.0-or-later
#include "sequoiaelevenpickerpreview.h"
#include "../../../../desktop/glass-material/glassrim.h"

#include <KDirOperator>
#include <KFileWidget>
#include <KPreviewWidgetBase>
#include <KIO/PreviewJob>
#include <QAbstractItemView>
#include <QAction>
#include <QFileInfo>
#include <QFormLayout>
#include <QImageReader>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QScrollArea>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

namespace SequoiaEleven::FilePicker {
namespace {
class RoundedThumbnail final : public QWidget
{
public:
    void setPixmap(const QPixmap &pixmap) { m_pixmap = pixmap; update(); }
    void clear() { m_pixmap = QPixmap(); update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_pixmap.isNull()) return;
        const QSizeF size = m_pixmap.size() / m_pixmap.devicePixelRatio();
        QRectF image(QPointF(), size);
        image.moveCenter(QRectF(rect()).center());
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath clip;
        clip.addRoundedRect(image, 12, 12);
        painter.setClipPath(clip);
        painter.drawPixmap(image, m_pixmap, QRectF(m_pixmap.rect()));
    }
private:
    QPixmap m_pixmap;
};

class PreviewCard final : public QWidget
{
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF surface = QRectF(rect()).adjusted(.5, .5, -.5, -.5);
        QColor fill = palette().color(QPalette::Button);
        QColor border = palette().color(QPalette::WindowText);
        border.setAlphaF(.12);
#ifdef SEQUOIA_GLASS_TRIAL
        fill.setAlphaF(.54);
#else
        fill.setAlpha(255);
#endif
        painter.setBrush(fill);
        painter.setPen(QPen(border, 1));
        painter.drawRoundedRect(surface, 14, 14);
#ifdef SEQUOIA_GLASS_TRIAL
        SequoiaGlass::paintRim(&painter, surface, 14, SequoiaGlass::Settings::instance()->intensity(), isActiveWindow());
#endif
    }
};

class SelectionPreview final : public KPreviewWidgetBase
{
public:
    explicit SelectionPreview(KDirOperator *op) : KPreviewWidgetBase(op), m_operator(op)
    {
        setObjectName(QStringLiteral("sequoiaPickerPreview"));
        setMinimumWidth(230);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        auto outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        auto scroll = new QScrollArea(this);
        scroll->setFrameShape(QFrame::StyledPanel);
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outer->addWidget(scroll);
        auto content = new QWidget;
        auto layout = new QVBoxLayout(content);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(12);
        m_image = new RoundedThumbnail;
        m_image->setObjectName(QStringLiteral("sequoiaPickerThumbnail"));
        m_image->setFixedHeight(170);
        m_image->setMinimumWidth(0);
        m_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        layout->addWidget(m_image);
        auto file = section(layout, tr("File"));
        m_fileHeading = qobject_cast<QLabel *>(file->itemAt(0, QFormLayout::SpanningRole)->widget());
        m_name = field(file, tr("Name"));
        m_type = field(file, tr("Type"));
        m_size = field(file, tr("Size"));
        auto image = section(layout, tr("Image"));
        m_imageSection = image->parentWidget();
        m_dimensions = field(image, tr("Dimensions"));
        auto location = section(layout, tr("Location"));
        m_location = field(location, tr("Folder"));
        auto dates = section(layout, tr("Dates"));
        m_modified = field(dates, tr("Modified"));
        m_created = field(dates, tr("Created"));
        layout->addStretch();
        scroll->setWidget(content);
        setSupportedMimeTypes({QStringLiteral("*/*"), QStringLiteral("inode/directory")});
        connect(op, &KDirOperator::fileHighlighted, this, [this] { schedule(); });
        connect(op, &KDirOperator::urlEntered, this, [this] { reset(); schedule(); });
        connect(op, &KDirOperator::viewChanged, this, [this](QAbstractItemView *view) { watchSelection(view); });
        connect(op->action(KDirOperator::ShowPreviewPanel), &QAction::toggled, this, [this] { schedule(); });
        watchSelection(op->view());
        hide();
    }
    ~SelectionPreview() override { cancelJob(); }
    QSize sizeHint() const override { return QSize(270, 480); }

    // KDE also calls these for hover. Always resolve the actual selection so a
    // passing pointer cannot replace it or resurrect an empty preview panel.
    void showPreview(const QUrl &) override { schedule(); }
    void clearPreview() override { schedule(); }

    void refresh()
    {
        if (!m_operator) return;
        const auto items = m_operator->selectedItems();
        const auto mode = m_operator->mode();
        const bool files = mode.testFlag(KFile::File) || mode.testFlag(KFile::Files);
        const bool folders = mode.testFlag(KFile::Directory);
        if (!m_operator->action(KDirOperator::ShowPreviewPanel)->isChecked() || items.size() != 1
            || (items.first().isDir() ? !folders : !files)) {
            reset();
            return;
        }
        const auto item = items.first();
        if (item.url().isLocalFile() && !QFileInfo::exists(item.url().toLocalFile())) { reset(); return; }
        if (m_url == item.url()) { show(); return; }
        cancelJob();
        m_url = item.url();
        setProperty("selectedUrl", m_url);
        m_fileHeading->setText(item.isDir() ? tr("Folder") : tr("File"));
        m_name->setText(item.name());
        m_type->setText(item.isDir() ? tr("Folder") : item.mimeComment());
        m_size->setText(item.isDir() ? tr("—") : locale().formattedDataSize(item.size()));
        m_location->setText(item.url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toDisplayString(QUrl::PreferLocalFile));
        m_modified->setText(date(item.time(KFileItem::ModificationTime)));
        m_created->setText(date(item.time(KFileItem::CreationTime)));
        QSize dimensions;
        if (item.isLocalFile() && item.mimetype().startsWith(QLatin1String("image/"))) {
            QImageReader reader(item.localPath());
            dimensions = reader.size();
        }
        m_imageSection->setVisible(dimensions.isValid());
        m_dimensions->setText(tr("%1 × %2 px").arg(dimensions.width()).arg(dimensions.height()));
        m_thumbnail = QIcon::fromTheme(item.iconName()).pixmap(96, 96);
        updateImage();
        show();
        if (!m_sized) {
            if (auto splitter = qobject_cast<QSplitter *>(parentWidget()); splitter && splitter->count() == 2 && splitter->width() > 560) {
                splitter->setSizes({splitter->width() - 280, 280});
                m_sized = true;
            }
        }
        if (!item.isDir()) {
            const QUrl requested = m_url;
            m_job = KIO::filePreview(KFileItemList{item}, QSize(480, 340));
            m_job->setScaleType(KIO::PreviewJob::Scaled);
            connect(m_job, &KIO::PreviewJob::gotPreview, this, [this, requested](const KFileItem &, const QPixmap &pixmap) {
                if (requested != m_url) return;
                m_thumbnail = pixmap;
                updateImage();
            });
        }
    }
protected:
    void showEvent(QShowEvent *event) override { KPreviewWidgetBase::showEvent(event); schedule(); }
    void resizeEvent(QResizeEvent *event) override { KPreviewWidgetBase::resizeEvent(event); updateImage(); }
private:
    static QFormLayout *section(QVBoxLayout *layout, const QString &title)
    {
        auto group = new PreviewCard;
        group->setObjectName(QStringLiteral("sequoiaPickerCategoryCard"));
        auto rows = new QFormLayout(group);
        rows->setContentsMargins(12, 12, 12, 12);
        rows->setHorizontalSpacing(12);
        rows->setVerticalSpacing(6);
        rows->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        rows->setRowWrapPolicy(QFormLayout::WrapLongRows);
        auto heading = new QLabel(title);
        auto font = heading->font(); font.setBold(true); heading->setFont(font);
        rows->addRow(heading);
        layout->addWidget(group);
        return rows;
    }
    static QLabel *field(QFormLayout *form, const QString &title)
    {
        auto value = new QLabel;
        value->setTextFormat(Qt::PlainText);
        value->setWordWrap(true);
        value->setMinimumWidth(0);
        value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto label = new QLabel(title);
        label->setForegroundRole(QPalette::PlaceholderText);
        form->addRow(label, value);
        return value;
    }
    QString date(const QDateTime &date) const { return date.isValid() ? locale().toString(date, QLocale::ShortFormat) : tr("Unavailable"); }
    void schedule()
    {
        if (m_pending) return;
        m_pending = true;
        QTimer::singleShot(0, this, [this] { m_pending = false; refresh(); });
    }
    void watchSelection(QAbstractItemView *view)
    {
        disconnect(m_selectionConnection);
        if (view && view->selectionModel())
            m_selectionConnection = connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { schedule(); });
        schedule();
    }
    void cancelJob() { if (m_job) { m_job->kill(); m_job = nullptr; } }
    void reset() { cancelJob(); m_url = QUrl(); setProperty("selectedUrl", m_url); m_thumbnail = QPixmap(); m_image->clear(); hide(); }
    void updateImage()
    {
        if (!m_thumbnail.isNull()) {
            const int height = qBound(80, m_image->width() * m_thumbnail.height() / qMax(1, m_thumbnail.width()), 170);
            m_image->setFixedHeight(height);
            m_image->setPixmap(m_thumbnail.scaled(qMax(1, m_image->width()), height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    QPointer<KDirOperator> m_operator;
    QPointer<KIO::PreviewJob> m_job;
    QMetaObject::Connection m_selectionConnection;
    QUrl m_url;
    QPixmap m_thumbnail;
    bool m_pending = false;
    bool m_sized = false;
    RoundedThumbnail *m_image;
    QLabel *m_fileHeading, *m_name, *m_type, *m_size, *m_dimensions, *m_location, *m_modified, *m_created;
    QWidget *m_imageSection;
};
}

void installPreview(QWidget *dialog)
{
    for (auto file : dialog->findChildren<KFileWidget *>()) {
        auto op = file->dirOperator();
        if (!op || op->property("_sequoia_preview_installed").toBool()) continue;
        // Respect application-supplied preview providers; replace KDE's default.
        bool custom = false;
        for (auto preview : op->findChildren<KPreviewWidgetBase *>())
            if (!preview->inherits("KFileMetaPreview") && !preview->inherits("KImageFilePreview")) custom = true;
        if (custom) continue;
        op->setProperty("_sequoia_preview_installed", true);
        auto action = op->action(KDirOperator::ShowPreviewPanel);
        op->setProperty("_sequoia_preview_enabled", action->isEnabled());
        const bool checked = action->isChecked();
        QList<QUrl> selected;
        for (const auto &item : op->selectedItems()) selected.append(item.url());
        auto preview = new SelectionPreview(op);
        op->setPreviewWidget(preview);
        action->setChecked(checked);
        if (!selected.isEmpty()) op->setCurrentItems(selected);
        preview->refresh();
    }
}

void restorePreview(QWidget *dialog)
{
    for (auto op : dialog->findChildren<KDirOperator *>()) {
        if (!op->property("_sequoia_preview_installed").toBool()) continue;
        const bool checked = op->action(KDirOperator::ShowPreviewPanel)->isChecked();
        op->setProperty("_sequoia_preview_installed", QVariant());
        op->setPreviewWidget(nullptr);
        op->action(KDirOperator::ShowPreviewPanel)->setEnabled(op->property("_sequoia_preview_enabled").toBool());
        op->setProperty("_sequoia_preview_enabled", QVariant());
        op->action(KDirOperator::ShowPreviewPanel)->setChecked(checked);
    }
}
}
