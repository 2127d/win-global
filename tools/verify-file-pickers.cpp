// SPDX-License-Identifier: MIT
#include <QApplication>
#include <KDirOperator>
#include <KFileFilterCombo>
#include <KFilePlacesView>
#include <KFilePlacesModel>
#include <KUrlComboBox>
#include <QAbstractItemView>
#include <QAction>
#include <QHeaderView>
#include <QFrame>
#include <QDir>
#include <QMenu>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStyleFactory>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdio>

static void require(bool ok, const char *message)
{
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

static void ordinaryWindow(const QString &output)
{
    // These are the same native controls the chooser and Dolphin both use.
    // Compare this capture byte-for-byte against the previously built style.
    QMainWindow window;
    auto central = new QWidget;
    auto layout = new QVBoxLayout(central);
    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon::fromTheme("go-previous"), "Back");
    toolbar->addAction(QIcon::fromTheme("go-next"), "Forward");
    window.addToolBar(toolbar);
    layout->addWidget(new QLabel("Ordinary file explorer controls"));
    layout->addWidget(new QLineEdit("/home/example/Documents"));
    auto combo = new QComboBox;
    combo->addItems({"Name", "Size", "Date"});
    layout->addWidget(combo);
    layout->addWidget(new QPushButton("Open"));
    window.setCentralWidget(central);
    window.resize(600, 300);
    window.show();
    window.setFocus();
    QTest::qWait(250);
    require(!layout->property("_sequoia_picker_margins").isValid(), "Explorer layout was tagged as a picker");
    require(window.grab().save(output + "/ordinary-window.png"), "Cannot save ordinary window");
    QDialog settings;
    auto settingsLayout = new QVBoxLayout(&settings);
    settingsLayout->addWidget(new QPushButton("Open"));
    settings.show();
    QTest::qWait(100);
    require(!settingsLayout->property("_sequoia_picker_margins").isValid(), "Unrelated dialog was tagged as a picker");
    require(settings.grab().save(output + "/ordinary-dialog.png"), "Cannot save ordinary dialog");
}

static void verifyDetails(const QString &fixture, const QString &output)
{
    QImage image(1600, 900, QImage::Format_RGB32);
    image.fill(QColor("#86b5ce"));
    require(image.save(fixture + "/landscape.png"), "Cannot prepare preview image");
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setDirectory(fixture);
    dialog.show();
    QTest::qWait(300);
    QWidget *surface = nullptr;
    for (auto widget : QApplication::topLevelWidgets())
        if (widget->isVisible() && widget->inherits("KDEPlatformFileDialog")) surface = widget;
    require(surface, "Missing native detail chooser");
    surface->resize(1100, 740);
    auto op = surface->findChild<KDirOperator *>();
    require(op, "Missing native directory operator");
    auto preview = surface->findChild<QWidget *>("sequoiaPickerPreview");
    require(preview, "Missing categorized preview");
    op->action(KDirOperator::ShowPreviewPanel)->setChecked(true);
    op->view()->clearSelection();
    QTest::qWait(150);
    require(!preview->isVisible(), "Empty selection leaves a preview panel");
    const QUrl selected = QUrl::fromLocalFile(fixture + "/landscape.png");
    op->setCurrentItem(selected);
    QTest::qWait(900);
    require(preview->isVisible() && preview->property("selectedUrl").toUrl() == selected, "Selected file preview did not appear");
    QStringList labels;
    for (auto label : preview->findChildren<QLabel *>()) if (label->isVisible()) labels.append(label->text());
    for (const QString &text : {QString("File"), QString("Image"), QString("Location"), QString("Dates"), QString("1600 × 900 px")})
        require(labels.contains(text), qPrintable("Missing metadata: " + text));
    for (auto header : op->findChildren<QHeaderView *>()) {
        if (header->orientation() != Qt::Horizontal || !header->isVisible()) continue;
        require(!header->viewport()->mask().contains(QPoint(0, 0)), "File header top-left corner is square");
        require(!header->viewport()->mask().contains(QPoint(header->viewport()->width()-1, 0)), "File header top-right corner is square");
        require(header->viewport()->mask().contains(header->viewport()->rect().center()), "Header mask hides its text");
    }
    for (auto frame : surface->findChildren<QFrame *>())
        if (frame->property("_sequoia_picker_separator").isValid()) require(!frame->isVisible(), "Breadcrumb divider is still visible");
    require(surface->grab().save(output + "/categorized-preview.png"), "Cannot capture enhanced preview");
    for (int row = 0; row < op->view()->model()->rowCount(); ++row) {
        const auto index = op->view()->model()->index(row, 0);
        if (index.data().toString() != "example.txt") continue;
        QTest::mouseMove(op->view()->viewport(), op->view()->visualRect(index).center());
        QTest::qWait(200);
        require(preview->property("selectedUrl").toUrl() == selected, "Hover replaced the selected preview");
    }
    const int fileWidth = op->view()->width();
    op->view()->clearSelection(); QTest::qWait(100);
    require(!preview->isVisible() && op->view()->width() > fileWidth, "Clearing selection does not reclaim preview space");
    op->setCurrentItem(QUrl::fromLocalFile(fixture + "/Documents")); QTest::qWait(100);
    require(!preview->isVisible(), "Folder preview visible in a file picker");
    op->setCurrentItems({selected, QUrl::fromLocalFile(fixture + "/example.txt")}); QTest::qWait(100);
    require(!preview->isVisible(), "Multiple selection leaves stale single-file details");
    op->setCurrentItem(selected); QTest::qWait(100);
    op->action(KDirOperator::ShowPreviewPanel)->setChecked(false); QTest::qWait(100);
    require(!preview->isVisible(), "Preview toggle ignored");
    op->action(KDirOperator::ShowPreviewPanel)->setChecked(true); QTest::qWait(100);
    require(preview->isVisible(), "Preview toggle cannot reopen selected item");
    const QString deep = fixture + "/A long breadcrumb parent folder/Another long breadcrumb parent folder/Current folder";
    require(QDir().mkpath(deep), "Cannot create breadcrumb fixture");
    op->setUrl(QUrl::fromLocalFile(deep), true);
    surface->resize(720, 520);
    QTest::qWait(200);
    QWidget *arrow = nullptr;
    for (auto widget : surface->findChildren<QWidget *>())
        if (widget->inherits("KDEPrivate::KUrlNavigatorDropDownButton") && widget->isVisible()) arrow = widget;
    require(arrow && arrow->width() == 36, "Breadcrumb arrow did not move inward");
    QTest::mouseMove(arrow, arrow->rect().center()); QTest::qWait(100);
    require(arrow->parentWidget()->grab().save(output + "/breadcrumb-hover.png"), "Cannot capture arrow hover");
    bool menuOpened = false;
    QTimer::singleShot(100, [&menuOpened] {
        auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        menuOpened = menu != nullptr;
        if (menu) QTest::keyClick(menu, Qt::Key_Escape);
    });
    QTest::mouseClick(arrow, Qt::LeftButton); QTest::qWait(150);
    require(menuOpened, "Breadcrumb arrow no longer opens its menu");
    QAbstractButton *edit = nullptr;
    QAbstractButton *crumb = nullptr;
    for (auto button : surface->findChildren<QAbstractButton *>()) {
        if (!button->isVisible()) continue;
        if (button->inherits("KDEPrivate::KUrlNavigatorToggleButton")) edit = button;
        if (button->inherits("KDEPrivate::KUrlNavigatorButton")) crumb = button;
    }
    require(edit && crumb, "Missing breadcrumb controls");
    const QPoint editPoint(edit->width() - 24, edit->height() / 2);
    QTest::mouseMove(edit, editPoint); QTest::qWait(100);
    require(edit->parentWidget()->grab().save(output + "/breadcrumb-edit-hover.png"), "Cannot capture edit hover");
    QTest::mouseClick(edit, Qt::LeftButton, Qt::NoModifier, editPoint); QTest::qWait(100);
    require(edit->isChecked(), "Edit button no longer opens path editing");
    auto history = edit->parentWidget()->findChild<KUrlComboBox *>();
    require(history && history->isVisible(), "Missing breadcrumb editor history");
    history->showPopup(); QTest::qWait(150);
    auto historyPopup = history->view()->window();
    require(historyPopup->isVisible(), "Breadcrumb history no longer opens");
    require(historyPopup->property("_sequoia_picker_filter_popup").toBool(), "Breadcrumb history was not styled");
    require(historyPopup->grab().save(output + "/breadcrumb-edit-dropdown.png"), "Cannot capture breadcrumb history");
    QTest::keyClick(history->view(), Qt::Key_Escape); QTest::qWait(100);

    QTest::mouseClick(edit, Qt::LeftButton, Qt::NoModifier, editPoint); QTest::qWait(100);
    require(!edit->isChecked(), "Edit button no longer returns to breadcrumbs");
    // Native breadcrumbs can be replaced when changing editing mode.
    for (auto button : surface->findChildren<QAbstractButton *>())
        if (button->inherits("KDEPrivate::KUrlNavigatorButton") && button->isVisible()) crumb = button;
    QTest::mouseMove(crumb, crumb->rect().center()); QTest::qWait(100);
    require(crumb->parentWidget()->grab().save(output + "/breadcrumb-folder-hover.png"), "Cannot capture folder hover");
    auto filter = surface->findChild<KFileFilterCombo *>();
    require(filter, "Missing native file-type control");
    filter->setFilters({KFileFilter("Image files", {"*.png"}, {}), KFileFilter("Documents and text files", {"*.txt"}, {})});
    QTest::qWait(100);
    require(filter && filter->count() >= 2, "Missing file-type filters");
    filter->setCurrentIndex(0);
    filter->showPopup(); QTest::qWait(150);
    auto popup = filter->view()->window();
    require(popup->isVisible() && popup->width() < filter->width(), "File-type menu still fills its field width");
    require(popup->width() >= filter->fontMetrics().horizontalAdvance("Documents and text files") + 16,
            "File-type menu clips its longest item");
    const int multipleWidth = popup->width();
    require(popup->grab().save(output + "/filetype-dropdown.png"), "Cannot capture file-type menu");
    QTest::keyClick(filter->view(), Qt::Key_End);
    QTest::keyClick(filter->view(), Qt::Key_Return); QTest::qWait(100);
    require(filter->currentIndex() == filter->count() - 1 && !popup->isVisible(), "File-type keyboard selection failed");
    filter->setFilters({KFileFilter("Image files", {"*.png"}, {})}); QTest::qWait(100);
    filter->showPopup(); QTest::qWait(150);
    require(popup->width() < multipleWidth && popup->height() < 80, "Single-item menu does not shrink to its contents");
    const QRect row = filter->view()->visualRect(filter->model()->index(0, 0));
    require(filter->view()->viewport()->rect().contains(row), "Dropdown clips the last item's rounded corners");
    require(popup->grab().toImage().pixelColor(0, 0).alpha() == 0, "Dropdown outer corner is not transparent");
    require(popup->grab().save(output + "/filetype-single-dropdown.png"), "Cannot capture single file-type menu");
    QTest::keyClick(filter->view(), Qt::Key_Escape); QTest::qWait(100);
    auto places = surface->findChild<KFilePlacesView *>();
    require(places && places->itemDelegate()->inherits("KFilePlacesViewDelegate"), "Missing pill sidebar navigation");
    auto placesModel = qobject_cast<KFilePlacesModel *>(places->model());
    const QUrl target = QUrl::fromLocalFile(fixture + "/Documents");
    placesModel->addPlace("Navigation test", target, "folder");
    QTest::qWait(400);
    const auto place = placesModel->closestItem(target);
    places->scrollTo(place); QTest::qWait(100);
    auto placeRect = places->visualRect(place);
    QTest::mouseClick(places->viewport(), Qt::LeftButton, Qt::NoModifier, placeRect.center()); QTest::qWait(250);
    require(op->url().adjusted(QUrl::StripTrailingSlash) == target, "Sidebar navigation no longer opens its folder");
    require(places->grab().save(output + "/sidebar-navigation.png"), "Cannot capture sidebar navigation");
    const QString styleName = qApp->style()->objectName();
    qApp->setStyle("Fusion"); QTest::qWait(150);
    require(!surface->findChild<QWidget *>("sequoiaPickerPreview"), "Custom preview survived style removal");
    qApp->setStyle(QStyleFactory::create(styleName)); QTest::qWait(150);
    require(surface->findChild<QWidget *>("sequoiaPickerPreview"), "Preview not restored when Sequoia is selected again");
    dialog.reject();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    require(argc == 5, "Usage: verify-file-pickers STYLE FIXTURE OUTPUT baseline|verify");
    app.setFont(QFont("Adwaita Sans", 10));
    auto style = QStyleFactory::create(QString::fromLocal8Bit(argv[1]));
    require(style, "Cannot load requested style");
    app.setStyle(style);
    const QString fixture = QString::fromLocal8Bit(argv[2]);
    const QString output = QString::fromLocal8Bit(argv[3]);
    ordinaryWindow(output);
    if (QString::fromLocal8Bit(argv[4]) == "baseline") return 0;

    for (bool native : {true, false}) {
        for (const QString mode : {"open", "multiple", "folder", "save", "cancel"}) {
            QFileDialog dialog;
            dialog.setOption(QFileDialog::DontUseNativeDialog, !native);
            dialog.setDirectory(fixture);
            dialog.setNameFilter("Text documents (*.txt)");
            dialog.setFileMode(mode == "folder" ? QFileDialog::Directory
                               : mode == "multiple" ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);
            if (mode == "folder") dialog.setOption(QFileDialog::ShowDirsOnly);
            if (mode == "save") { dialog.setAcceptMode(QFileDialog::AcceptSave); dialog.setFileMode(QFileDialog::AnyFile); }
            const QString selected = fixture + (mode == "folder" ? "/Documents" : mode == "save" ? "/new.txt" : "/example.txt");
            dialog.selectFile(selected);
            dialog.show();
            QTest::qWait(350);
            QWidget *surface = &dialog;
            if (native) {
                surface = nullptr;
                for (auto candidate : app.topLevelWidgets())
                    if (candidate->isVisible() && candidate->inherits("KDEPlatformFileDialog")) surface = candidate;
                require(surface, "KDE native file chooser was not used");
            }
            surface->resize(1000, 680);
            QTest::qWait(100);
            if (native && mode == "folder") {
                auto op = surface->findChild<KDirOperator *>();
                op->action(KDirOperator::ShowPreviewPanel)->setChecked(true);
                QTest::qWait(100);
                auto preview = surface->findChild<QWidget *>("sequoiaPickerPreview");
                require(preview && preview->isVisible(), "Selected folder has no preview in a folder picker");
                require(preview->property("selectedUrl").toUrl() == QUrl::fromLocalFile(selected), "Folder preview has the wrong selection");
            }
            require(surface->layout()->contentsMargins() == QMargins(12, 12, 12, 12), "Chooser needs 12px outer padding");
            QPushButton *accept = nullptr;
            QPushButton *cancel = nullptr;
            for (auto button : surface->findChildren<QPushButton *>()) {
                if (!button->isVisible()) continue;
                const auto text = button->text().remove('&');
                if (text == "Open" || text == "Save" || text == "Choose" || text == "Select Folder") accept = button;
                if (text == "Cancel") cancel = button;
            }
            require(accept && cancel, "Could not find native accept/cancel actions");
            require(accept->height() >= 42 && cancel->height() >= 42, "Actions are smaller than library controls");
            require(accept->width() < 250 && cancel->width() < 250, "Action buttons do not fit content");
            if (mode == "multiple") {
                QLineEdit *name = nullptr;
                for (auto edit : surface->findChildren<QLineEdit *>())
                    if (edit->isVisible() && edit->text() == "example.txt") name = edit;
                require(name, "Cannot find multi-file name entry");
                name->setFocus();
                name->selectAll();
                QTest::keyClicks(name, "\"example.txt\" \"second.txt\"");
                QTest::qWait(100);
            }
            if (mode == "open" || mode == "folder") {
                const auto capture = surface->grab();
                if (mode == "open" && QString::fromLocal8Bit(argv[1]) == "SequoiaElevenGlass") {
                    for (auto edit : surface->findChildren<QLineEdit *>()) {
                        if (!edit->isVisible() || edit->text() != "example.txt") continue;
                        const QPoint sample = edit->mapTo(surface, QPoint(edit->width() / 2, edit->height() - 3));
                        require(capture.toImage().pixelColor(sample).alpha() < 255,
                                "Opaque editor rectangle covers the glass field");
                    }
                }
                require(capture.save(output + (native ? "/native-" : "/qt-") + mode + ".png"), "Cannot capture chooser");
                surface->resize(720, 520);
                QTest::qWait(100);
                require(surface->rect().contains(accept->mapTo(surface, accept->rect().center())), "Action outside compact chooser");
                require(surface->grab().save(output + (native ? "/native-" : "/qt-") + mode + "-compact.png"), "Cannot capture compact chooser");
            }
            if (mode == "cancel") {
                QTest::mouseClick(cancel, Qt::LeftButton);
                QTest::qWait(100);
                require(dialog.result() == QDialog::Rejected && !surface->isVisible(), "Cancel failed");
            } else {
                require(accept->isEnabled(), "Accept is disabled for valid selection");
                QTest::mouseClick(accept, Qt::LeftButton);
                QTest::qWait(200);
                require(dialog.result() == QDialog::Accepted, "Chooser failed to accept selection");
                auto files = dialog.selectedFiles();
                files.sort();
                const QStringList expected = mode == "multiple"
                    ? QStringList{fixture + "/example.txt", fixture + "/second.txt"} : QStringList{selected};
                require(files == expected, "Chooser returned a different path");
            }
        }
    }
    verifyDetails(fixture, output);
    // Switching styles restores the dialog layout instead of leaving edits.
    QFileDialog cleanup;
    cleanup.setOption(QFileDialog::DontUseNativeDialog);
    cleanup.show();
    QTest::qWait(100);
    const auto original = cleanup.layout()->property("_sequoia_picker_margins").value<QMargins>();
    app.setStyle("Fusion");
    QTest::qWait(100);
    require(!cleanup.layout()->property("_sequoia_picker_margins").isValid(), "Picker state survived a style switch");
    require(cleanup.layout()->contentsMargins() == original, "Original dialog margins were not restored");
    fprintf(stderr, "PASS: native/Qt open, multi-file, folder, save, cancel, compact sizing and style cleanup\n");
}
