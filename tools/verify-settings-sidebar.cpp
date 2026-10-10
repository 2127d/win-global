// SPDX-License-Identifier: MIT
// Exercise the native KDE settings list used by Konsole's profile dialog.
// Build with -fPIC, Qt6Widgets/Qt6Test pkg-config flags, and KF6WidgetsAddons.
#include <QApplication>
#include <KPageDialog>
#include <KPageWidgetItem>
#include <QLabel>
#include <QListView>
#include <QScrollBar>
#include <QStyleFactory>
#include <QTest>
#include <cstdio>
#include <cstdlib>

static void require(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void settle()
{
    QTest::qWait(100);
}

static void verifyRows(QListView *view)
{
    settle();
    const auto viewport = view->viewport()->rect();
    require(view->viewport()->x() >= 12, "Sidebar lost its island inset");
    require(viewport.width() >= view->sizeHintForColumn(0), "Sidebar clips the delegate's required width");
    for (int row = 0; row < view->model()->rowCount(); ++row) {
        const auto rect = view->visualRect(view->model()->index(row, 0));
        require(rect.left() >= viewport.left() && rect.right() <= viewport.right(),
                "Selection row extends beyond the viewport");
    }
    require(view->horizontalScrollBar()->maximum() == 0, "Settings sidebar needs horizontal scrolling");
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    require(argc == 3, "Usage: verify-settings-sidebar PLUGIN_DIRECTORY PREVIEW.png");
    QCoreApplication::addLibraryPath(QString::fromLocal8Bit(argv[1]));
    auto style = QStyleFactory::create(QStringLiteral("SequoiaEleven"));
    require(style, "Could not load the staged style");
    app.setStyle(style);
    app.setFont(QFont(QStringLiteral("Adwaita Sans"), 10));
    KPageDialog dialog;
    dialog.setFaceType(KPageDialog::List);
    QList<KPageWidgetItem *> pages;
    const QStringList names{"General", "Tabs", "Appearance", "Scrolling", "Keyboard", "Mouse", "Advanced"};
    const QStringList icons{"utilities-terminal", "tab-duplicate", "preferences-desktop-theme",
                            "preferences-system-windows", "input-keyboard", "input-mouse", "configure"};
    for (int i = 0; i < names.size(); ++i) {
        auto page = dialog.addPage(new QLabel(names[i]), names[i]);
        page->setIcon(QIcon::fromTheme(icons[i]));
        pages.append(page);
    }
    dialog.resize(750, 700);
    dialog.show();
    settle();
    QListView *view = nullptr;
    for (auto candidate : dialog.findChildren<QListView *>()) {
        if (candidate->inherits("KDEPrivate::KPageListView")) view = candidate;
    }
    require(view, "Native KDE settings sidebar was not found");
    require(dialog.grab().save(QString::fromLocal8Bit(argv[2])), "Could not save preview");
    verifyRows(view);
    for (auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
        dialog.setLayoutDirection(direction);
        for (int height : {700, 300}) {
            dialog.resize(750, height);
            verifyRows(view);
            for (int row : {0, 2, 6}) {
                const auto index = view->model()->index(row, 0);
                view->scrollTo(index);
                settle();
                QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, view->visualRect(index).center());
                require(dialog.currentPage() == pages[row], "Sidebar click did not change the settings page");
            }
        }
    }
    auto font = view->font();
    font.setPointSize(14);
    view->setFont(font);
    verifyRows(view);
    pages[2]->setName(QStringLiteral("Longer appearance settings"));
    verifyRows(view);
    const auto originalWidth = view->property("_sequoia_page_list_width").toSize().width();
    require(originalWidth > 0, "Width adjustment was not exercised");
    app.setStyle(QStringLiteral("Fusion"));
    settle();
    require(!view->property("_sequoia_page_list_width").isValid(), "Style-owned width was not cleared");
    require(view->minimumWidth() == originalWidth && view->maximumWidth() == originalWidth,
            "Original sidebar width was not restored");
    fprintf(stderr, "PASS: native KDE settings rows, clicks, scrolling, RTL, font/model changes and style cleanup\n");
}
