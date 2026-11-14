// SPDX-License-Identifier: MIT
#include <QApplication>
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
