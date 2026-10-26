// SPDX-License-Identifier: MIT
// Test-only startup hook: exercises the real GUI with isolated configuration.
#include <QBitArray>
#include <QApplication>
#include <QAbstractItemView>
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QStyleOptionComboBox>
#include <QLineEdit>
#include <QListView>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <algorithm>
#include <QLabel>
#include <QTextEdit>
#include <QClipboard>
#include <QLayout>
#include <QMimeData>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QSplitter>
#include <QToolButton>
#include <QToolBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QTest>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <cstdio>
#include <cstdlib>
#include <sys/xattr.h>
static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void verifyToggleProximity(QWidget *window, const QString &output)
{
    auto viewer = window->findChild<QGraphicsView *>("roundedImageViewer");
    require(viewer, "viewer for toolbar proximity checks");
    QApplication::setActiveWindow(window);
    auto moveTo = [&](const QPoint &point) {
        // The offscreen platform does not deliver cursor-warp mouse moves.
        // Send the same viewport event the real pointer supplies instead.
        const QPoint local = viewer->viewport()->mapFrom(viewer, point);
        QMouseEvent move(QEvent::MouseMove, local, viewer->mapToGlobal(point),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(viewer->viewport(), &move);
    };
    for (const char *name : {"mainToolbarToggle", "imageToolbarToggle"}) {
        auto toggle = window->findChild<QToolButton *>(name);
        require(toggle && !toggle->isChecked(), "proximity starts with collapsed toolbar");
        auto island = toggle->parentWidget();
        auto shadow = window->findChild<QWidget *>(QString::fromLatin1(name) == "mainToolbarToggle" ? "mainToolbarShadow" : "imageToolbarShadow");
        require(shadow && shadow->testAttribute(Qt::WA_TransparentForMouseEvents) && shadow->focusPolicy() == Qt::NoFocus,
                "floating shadow never takes pointer input or keyboard focus");
        auto opacity = [&] { return toggle->property("revealOpacity").toReal(); };
        toggle->clearFocus();
        moveTo(viewer->rect().center()); QTest::qWait(250);
        require(opacity() == 0 && island->testAttribute(Qt::WA_TransparentForMouseEvents), "collapsed toggle invisible and passes image input through");
        require(!shadow->isVisible(), "hidden toggle leaves no shadow behind");
        const QPoint nearby = toggle->mapTo(viewer, toggle->rect().center()) - QPoint(60, 0);
        moveTo(nearby); QTest::qWait(60);
        require(opacity() > 0 && opacity() < 1, "nearby cursor smoothly fades toggle in");
        QTest::qWait(200);
        require(opacity() == 1 && !island->testAttribute(Qt::WA_TransparentForMouseEvents), "revealed toggle usable before pointer reaches button");
        require(shadow->isVisible() && shadow->geometry().contains(island->geometry()), "revealed toggle has room for its floating shadow");
        QImage shadowImage(shadow->size(), QImage::Format_ARGB32_Premultiplied);
        shadowImage.fill(Qt::transparent);
        shadow->render(&shadowImage, QPoint(), QRegion(), QWidget::DrawChildren);
        const QPoint below = shadow->mapFromGlobal(toggle->mapToGlobal(QPoint(toggle->width() / 2, toggle->height() + 4)));
        require(qAlpha(shadowImage.pixel(below)) > 0, "soft shadow is visibly painted below the circular toggle");
        require(qAlpha(shadowImage.pixel(0, 0)) == 0, "shadow halo has clear outer corners");
        require(shadowImage.save(output + '/' + shadow->objectName() + ".png"), "shadow render capture");
        moveTo(viewer->rect().center()); QTest::qWait(60);
        require(opacity() > 0 && opacity() < 1, "departing cursor smoothly fades toggle out");
        moveTo(nearby); QTest::qWait(220);
        require(opacity() == 1, "fade reversal returns to full opacity");
        QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(250);
        moveTo(viewer->rect().center()); QTest::qWait(220);
        require(toggle->isChecked() && opacity() == 1, "expanded toolbar remains visible away from cursor");
        QTest::mouseClick(toggle, Qt::LeftButton);
        toggle->clearFocus();
        moveTo(viewer->rect().center()); QTest::qWait(500);
        require(!toggle->isChecked() && opacity() == 0, "collapsing away from pointer hides toggle again");
        toggle->setFocus(Qt::TabFocusReason); QTest::qWait(220);
        require(toggle->hasFocus(), "collapsed toggle accepts keyboard focus");
        require(opacity() == 1, "keyboard focus reveals collapsed toggle");
        toggle->clearFocus(); QTest::qWait(220);
        require(opacity() == 0, "leaving keyboard focus restores auto-hide");
    }
    require(window->grab().save(output + "/viewer-toggles-hidden.png"), "hidden toggle screenshot");
    fprintf(stderr, "PASS: both toggles hide, reveal nearby, fade/reverse, pass through input, stay visible expanded, and support keyboard focus\n");
}
static QAction *findAction(QWidget *window, const char *name)
{
    auto widgets = window->findChildren<QWidget *>();
    widgets.prepend(window);
    for (auto widget : widgets)
        for (auto action : widget->actions())
            if (action->objectName() == name) return action;
    require(false, name);
    return nullptr;
}

static void verifyBarModes(QWidget *window, const QString &output)
{
    auto toolbar = window->findChild<QToolBar *>("mainToolBar");
    auto top = window->findChild<QWidget *>("floatingMainToolbar");
    auto bottom = window->findChild<QWidget *>("floatingImageToolbar");
    auto status = window->findChild<QWidget *>("imageToolbarControls");
    auto flyout = window->findChild<QScrollArea *>("imageToolbarFlyout");
    auto viewer = window->findChild<QWidget *>("roundedImageViewer");
    auto showStatus = findAction(window, "options_show_statusbar");
    require(toolbar && top && bottom && status && flyout && viewer, "native and floating bars exist");
    require(toolbar->isVisible() && toolbar->toggleViewAction()->isChecked() && showStatus->isChecked(), "native bars enabled by default");
    require(status->isVisible() && !top->isVisible() && !bottom->isVisible(), "default native bars hide both custom pills");
    require(status->styleSheet().isEmpty() && toolbar->styleSheet().isEmpty(), "native bars have no custom pill stylesheet");
    require(status->width() > viewer->width() / 2, "native status bar spans image layout");
    require(status->mapTo(window, QPoint()).y() >= viewer->mapTo(window, QPoint(0, viewer->height())).y(), "native status bar reserves space below image");
    require(window->grab().save(output + "/viewer-native-bars.png"), "native bars capture");
    for (int repeat = 0; repeat < 2; ++repeat) {
        toolbar->toggleViewAction()->trigger(); QTest::qWait(250);
        require(!toolbar->isVisible() && toolbar->isEnabled() && top->isVisible() && status->isVisible() && !bottom->isVisible(), "unchecked toolbar changes only top bar to pill");
        showStatus->trigger(); QTest::qWait(250);
        require(!showStatus->isChecked() && bottom->isVisible() && flyout->widget() == status, "unchecked status bar moves native controls into bottom pill");
        require(!status->styleSheet().isEmpty(), "floating status controls receive pill styling");
        toolbar->toggleViewAction()->trigger(); QTest::qWait(250);
        require(toolbar->isVisible() && !top->isVisible() && bottom->isVisible(), "checked toolbar restores native top bar independently");
        showStatus->trigger(); QTest::qWait(250);
        require(status->isVisible() && !bottom->isVisible() && flyout->widget() == nullptr && status->styleSheet().isEmpty(), "checking status restores native controls and removes glass styling");
        const QSize nativeSize = status->size();
        window->resize(window->width() + 80, window->height()); QTest::qWait(250);
        require(status->width() > nativeSize.width(), "restored status bar expands with window");
        window->resize(window->width() - 80, window->height()); QTest::qWait(250);
    }
    auto fullscreen = findAction(window, "fullscreen");
    fullscreen->trigger(); QTest::qWait(250);
    require(!toolbar->isVisible() && !top->isVisible(), "fullscreen hides standard and floating main toolbar");
    fullscreen->trigger(); QTest::qWait(250);
    require(toolbar->isVisible() && status->isVisible() && !top->isVisible() && !bottom->isVisible(), "fullscreen exit restores native bars");
    auto spotlight = findAction(window, "view_toggle_spotlightmode");
    spotlight->trigger(); QTest::qWait(250);
    spotlight->trigger(); QTest::qWait(250);
    require(toolbar->isVisible() && status->isVisible() && !top->isVisible() && !bottom->isVisible(), "spotlight exit restores native bars");
    toolbar->toggleViewAction()->trigger();
    showStatus->trigger();
    QTest::qWait(300);
    fprintf(stderr, "PASS: native defaults, independent checkbox combinations, repeated style restoration, resizing, fullscreen and spotlight restoration\n");
}

static void verifyMainToolbar(QWidget *window, const QUrl &image, const QString &output)
{
    auto docked = window->findChild<QToolBar *>("mainToolBar");
    auto controls = window->findChild<QToolBar *>("mainToolbarControls");
    auto island = window->findChild<QWidget *>("floatingMainToolbar");
    auto toggle = window->findChild<QToolButton *>("mainToolbarToggle");
    auto flyout = window->findChild<QScrollArea *>("mainToolbarFlyout");
    auto viewer = window->findChild<QWidget *>("roundedImageViewer");
    require(docked && controls && island && toggle && flyout && viewer, "main toolbar island widgets");
    for (auto action : controls->actions())
        require(action->isSeparator() || !action->text().isEmpty() || !action->icon().isNull(), "no empty spacer buttons in island");
    verifyBarModes(window, output);
    require(!docked->isVisible() && docked->isEnabled(), "unchecked toolbar hidden but native actions remain enabled");
    require(docked->toggleViewAction()->isEnabled(), "native toolbar checkbox remains usable in image view");
    require(toggle->isVisible() && !toggle->isChecked() && !flyout->isVisible(), "main toolbar starts collapsed");
    auto button = [&](const char *name) {
        for (auto action : controls->actions()) if (action->objectName() == name) {
            require(docked->actions().contains(action), "copy uses original toolbar actions");
            auto result = qobject_cast<QToolButton *>(controls->widgetForAction(action));
            require(result, "main toolbar action has a button");
            return result;
        }
        require(false, name); return static_cast<QToolButton *>(nullptr);
    };
    const QRect before = viewer->geometry();
    const int anchor = island->geometry().right();
    QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(300);
    require(flyout->isVisible() && island->width() > toggle->width(), "main toolbar expands");
    fprintf(stderr, "MAIN TOOLBAR: y=%d right=%d anchor=%d viewer before=%d,%d %dx%d after=%d,%d %dx%d controls=%dx%d\n",
            island->y(), island->geometry().right(), anchor, before.x(), before.y(), before.width(), before.height(),
            viewer->x(), viewer->y(), viewer->width(), viewer->height(), controls->width(), controls->height());
    require(island->y() == 16 && island->geometry().right() == anchor && viewer->geometry() == before,
            "main toolbar expands left at top without resizing image");
    require(button("go_next")->isEnabled(), "floating navigation remains enabled");
    int navigated = 0;
    auto connection = QObject::connect(button("go_next")->defaultAction(), &QAction::triggered, window, [&] { ++navigated; });
    QTest::mouseClick(button("go_next"), Qt::LeftButton); QTest::qWait(400);
    QObject::disconnect(connection);
    require(navigated == 1, "floating next button triggers native action once");
    bool menuOpened = false;
    QTimer::singleShot(150, window, [&] {
        for (auto widget : QApplication::topLevelWidgets()) {
            if (auto menu = qobject_cast<QMenu *>(widget); menu && menu->isVisible()) {
                menuOpened = true;
                menu->close();
            }
        }
    });
    QTest::mouseClick(button("hamburger_menu"), Qt::LeftButton); QTest::qWait(250);
    require(menuOpened, "floating hamburger opens native menu");
    QAction custom(QStringLiteral("Toolbar probe"), window);
    docked->insertAction(button("go_next")->defaultAction(), &custom);
    QTest::qWait(200);
    require(controls->actions().contains(&custom), "floating copy follows toolbar customization");
    custom.setEnabled(false);
    require(!controls->widgetForAction(&custom)->isEnabled(), "floating copy tracks action enabled state");
    docked->removeAction(&custom); QTest::qWait(200);
    require(!controls->actions().contains(&custom), "floating copy removes customized action");
    require(window->grab().save(output + "/viewer-main-toolbar-expanded.png"), "main toolbar screenshot");
    QTest::mouseClick(button("browse"), Qt::LeftButton); QTest::qWait(350);
    require(!docked->isVisible() && !island->isVisible(), "Browse respects unchecked toolbar and hides image pill");
    docked->toggleViewAction()->trigger(); QTest::qWait(250);
    require(docked->isVisible() && docked->isEnabled(), "Browse shows native toolbar when checked");
    require(window->grab().save(output + "/browse-rounded.png"), "rounded Browse capture");
    require(docked->toggleViewAction()->isEnabled(), "Browse keeps toolbar visibility control");
    docked->toggleViewAction()->trigger(); QTest::qWait(250);
    require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{image})), "return to image view");
    QTest::qWait(500);
    require(!docked->isVisible() && docked->isEnabled() && toggle->isVisible() && !toggle->isChecked(), "return to viewer collapses main island");
    QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(300);
    QTest::mouseClick(button("go_start_page"), Qt::LeftButton); QTest::qWait(300);
    require(!docked->isVisible() && docked->isEnabled() && !island->isVisible(), "Start respects unchecked toolbar and hides pill");
    require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{image})), "reopen image after Start");
    QTest::qWait(500);
    QAction *spotlight = nullptr;
    auto widgets = window->findChildren<QWidget *>(); widgets.prepend(window);
    for (auto widget : widgets)
        for (auto action : widget->actions())
            if (action->objectName() == "view_toggle_spotlightmode") spotlight = action;
    require(spotlight, "native spotlight action");
    spotlight->trigger(); QTest::qWait(250);
    require(!docked->isVisible() && toggle->isVisible(), "spotlight retains main island");
    spotlight->trigger(); QTest::qWait(250);
    require(!docked->isVisible() && toggle->isVisible(), "leaving spotlight keeps image toolbar floating");
    fprintf(stderr, "PASS: floating main toolbar actions, animation, native visibility preference across Browse/Start\n");
}
static void verifyWelcome(QWidget *window, const QString &output, const QString &mode)
{
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    auto welcome = window->findChild<QWidget *>("imageWelcomePage");
    auto title = window->findChild<QLabel *>("welcomeTitle");
    auto shortcut = window->findChild<QLabel *>("openImagesShortcut");
    auto button = window->findChild<QPushButton *>("addImagesButton");
    auto browse = window->findChild<QPushButton *>("browseFolderButton");
    auto tabs = window->findChild<QTabWidget *>("sidebarTabs");
    auto recent = window->findChild<QAbstractItemView *>("recentImagesView");
    auto count = window->findChild<QLabel *>("recentImagesCount");
    auto more = window->findChild<QToolButton *>("recentImagesMenuButton");
    require(welcome && title && shortcut && button && browse && tabs && recent && count && more, "welcome and sidebar controls exist");
    require(title->text() == "Open your images" && button->text() == "Open images…", "clear welcome labels");
    require(window->findChild<QWidget *>("welcomeImageStack"), "vector image-stack mark exists");
    auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "welcome capture"); };
    auto grid = window->findChild<QAction *>("recentGridView");
    auto list = window->findChild<QAction *>("recentListView");
    auto remove = window->findChild<QAction *>("removeRecentImage");
    auto clear = window->findChild<QAction *>("clearRecentImages");
    require(grid && list && remove && clear, "shared recent menu commands exist");
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    if (mode == "welcome-restart") {
        require(recent->property("listMode").toBool() && list->isChecked(), "recent list preference survives restart");
        require(count->text().toInt() == recent->model()->rowCount() && count->text().toInt() > 0, "recent count survives restart");
        clear->trigger(); QTest::qWait(250);
        require(recent->model()->rowCount() == 0 && count->text() == "0", "clear history updates live count");
        require(QFileInfo::exists(fixtures + "/one/picture.png") && QFileInfo::exists(fixtures + "/two/picture.png"), "clear history preserves source images");
        capture("welcome-cleared");
        fprintf(stderr, "PASS: persisted list preference, clear history, live count, source files preserved\n");
        return;
    }
    if (mode == "welcome-cleared") {
        require(recent->model()->rowCount() == 0 && count->text() == "0", "cleared history persists after restart");
        require(window->findChild<QLabel *>("recentImagesEmpty")->isVisible(), "cleared recents has an explanatory empty state");
        fprintf(stderr, "PASS: history remains cleared after restart\n");
        return;
    }
    require(count->text() == "6", "recent count reflects six fixtures");
    require(tabs->tabText(0) == "Recent" && tabs->tabText(1) == "Information" && tabs->tabText(2) == "Tools", "footer labels are discoverable");
    auto sidebarIsland = window->findChild<QWidget *>("sidebarIsland");
    auto splitter = qobject_cast<QSplitter *>(sidebarIsland->parentWidget());
    require(splitter, "sidebar splitter exists");
    splitter->setSizes({340, window->width() - 340}); QTest::qWait(200);
    const int tabWidth = tabs->tabBar()->tabRect(0).width();
    for (int i = 0; i < 3; ++i) {
        require(qAbs(tabs->tabBar()->tabRect(i).width() - tabWidth) <= 1, "footer tabs have equal widths");
        require(tabs->tabBar()->tabRect(i).width() >= tabs->tabBar()->fontMetrics().horizontalAdvance(tabs->tabText(i)) + 8, "footer labels fit without truncation");
    }
    auto bar = tabs->tabBar();
    require(bar->property("tabButtonStyle").toInt() == 3, "wide footer shows labels");
    const QImage plainFooter = bar->grab().toImage();
    tabs->setTabText(0, "&Recent"); tabs->setTabText(2, "&Tools"); QTest::qWait(100);
    require(bar->grab().toImage() == plainFooter, "mnemonic ampersands do not appear or change label spacing");
    const QIcon recentIcon = tabs->tabIcon(0);
    tabs->setTabIcon(0, QIcon()); QTest::qWait(100);
    require(bar->grab().toImage() == plainFooter, "Recent history icon is independent of missing theme assets");
    const auto savedSizes = splitter->sizes();
    splitter->setSizes({190, window->width() - 190}); QTest::qWait(200);
    require(bar->property("tabButtonStyle").toInt() == 0, "narrow sidebar switches to icons only");
    require(bar->tabRect(2).right() < bar->width(), "all three narrow tabs remain visible");
    require(tabs->tabToolTip(0) == "Recent" && tabs->tabToolTip(2) == "Tools", "icon-only tabs retain descriptive tooltips");
    capture("welcome-footer-narrow");
    QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->tabRect(2).center());
    require(tabs->currentIndex() == 2, "narrow Tools tab remains clickable");
    splitter->setSizes(savedSizes); tabs->setCurrentIndex(0); QTest::qWait(200);
    require(bar->property("tabButtonStyle").toInt() == 3, "widening restores footer labels");
    tabs->setTabText(0, "Recent"); tabs->setTabText(2, "Tools"); tabs->setTabIcon(0, recentIcon);
    fprintf(stderr, "PASS: mnemonic-free footer, theme-independent Recent icon, responsive labels and narrow tab navigation\n");
    auto infoEmpty = window->findChild<QWidget *>("informationEmptyState");
    auto toolsEmpty = window->findChild<QWidget *>("operationsEmptyState");
    require(infoEmpty && toolsEmpty, "sidebar context empty states exist");
    tabs->setCurrentIndex(1); QTest::qWait(120);
    require(infoEmpty->isVisible(), "Information explains its purpose before opening an image"); capture("welcome-information");
    tabs->setCurrentIndex(2); QTest::qWait(120);
    require(toolsEmpty->isVisible(), "Tools explains its purpose before opening an image"); capture("welcome-tools");
    tabs->tabBar()->setFocus(Qt::TabFocusReason);
    QTest::keyClick(tabs->tabBar(), Qt::Key_Left); QTest::qWait(100);
    require(tabs->currentIndex() == 1, "footer supports keyboard navigation");
    tabs->setCurrentIndex(0); QTest::qWait(150); capture("welcome-grid");
    auto open = findAction(window, "file_open");
    require(shortcut->text() == open->shortcut().toString(QKeySequence::NativeText), "welcome shortcut matches native Open action");
    auto verifyWelcomeCenter = [&] {
        const int center = welcome->mapToGlobal(welcome->rect().center()).x();
        require(qAbs(button->mapToGlobal(button->rect().center()).x() - center) <= 1, "Open images button is centered independently of shortcut");
        require(qAbs(browse->mapToGlobal(browse->rect().center()).x() - center) <= 1, "Browse folder action shares welcome center");
    };
    verifyWelcomeCenter();
    const auto originalShortcut = open->shortcut();
    open->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O)); QTest::qWait(150);
    require(shortcut->text() == open->shortcut().toString(QKeySequence::NativeText), "displayed shortcut follows customization");
    verifyWelcomeCenter();
    open->setShortcut(originalShortcut);
    QTest::mouseClick(button, Qt::LeftButton); QTest::qWait(250);
    auto picker = window->findChild<QFileDialog *>();
    require(picker && picker->isVisible() && picker->fileMode() == QFileDialog::ExistingFiles, "Open images opens native multi-file picker");
    picker->reject(); QTest::qWait(200);
    QMimeData invalidMime; invalidMime.setText("not an image");
    QDragEnterEvent invalid(QPoint(60, 60), Qt::CopyAction, &invalidMime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(welcome, &invalid);
    require(!invalid.isAccepted() && !welcome->property("dropActive").toBool(), "invalid drags never highlight image drop target");
    const QByteArray tagText(90, 'W');
    const QByteArray taggedFile = QFile::encodeName(fixtures + "/one/picture.png");
    require(setxattr(taggedFile.constData(), "user.xdg.tags", tagText.constData(), tagText.size(), 0) == 0, "long tag fixture metadata");
    QMimeData mime; mime.setUrls({QUrl::fromLocalFile(fixtures + "/one/picture.png")});
    QDragEnterEvent enter(QPoint(60, 60), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(welcome, &enter); QTest::qWait(100);
    require(enter.isAccepted() && welcome->property("dropActive").toBool() && title->text() == "Drop to open", "valid image drag highlights the panel and explains the action");
    capture("welcome-drop-active");
    QDragLeaveEvent leave; QApplication::sendEvent(welcome, &leave);
    require(!welcome->property("dropActive").toBool() && title->text() == "Open your images", "drag leave resets welcome state");
    QTest::mouseClick(browse, Qt::LeftButton); QTest::qWait(250);
    auto folder = window->findChild<QFileDialog *>("welcomeFolderDialog");
    require(folder && folder->isVisible() && folder->fileMode() == QFileDialog::Directory, "Browse opens a directory picker");
    folder->setDirectory(fixtures + "/one"); QTest::qWait(250);
    require(folder->selectedUrls().value(0) == QUrl::fromLocalFile(fixtures + "/one"), "folder picker accepts the selected directory");
    require(QMetaObject::invokeMethod(folder, "accept"), "accept folder picker"); QTest::qWait(800);
    require(!welcome->isVisible() && findAction(window, "browse")->isChecked(), "selected folder opens native Browse mode");
    findAction(window, "go_start_page")->trigger(); QTest::qWait(250);
    require(welcome->isVisible(), "return from folder to welcome");
    QDragEnterEvent enterAgain(QPoint(60, 60), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(welcome, &enterAgain);
    QDropEvent drop(QPointF(60, 60), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(welcome, &drop); QTest::qWait(700);
    require(drop.isAccepted() && !welcome->isVisible() && !welcome->property("dropActive").toBool(), "image drop opens native viewer and clears highlight");
    tabs->setCurrentIndex(1); QTest::qWait(150);
    require(!infoEmpty->isVisible(), "image information replaces empty state");
    tabs->setCurrentIndex(2); QTest::qWait(150);
    require(!toolsEmpty->isVisible(), "native tools replace empty state");
    auto toolsScroll = window->findChild<QScrollArea *>("operationsScrollArea");
    auto infoScroll = window->findChild<QScrollArea *>("informationScrollArea");
    require(toolsScroll && infoScroll, "both context pages have one outer scroll area");
    const auto contextSize = window->size();
    window->resize(1100, 650); splitter->setSizes({340, 760}); QTest::qWait(250);
    capture("context-tools-wide");
    auto rotateButton = window->findChild<QToolButton *>("sidebarAction_rotate_right");
    require(rotateButton && rotateButton->isVisible(), "native image tools appear in polished rows");
    require(rotateButton->focusPolicy() == Qt::StrongFocus, "tool rows accept keyboard focus");
    auto rotateAction = rotateButton->defaultAction();
    const QString originalToolText = rotateAction->text();
    rotateAction->setText("Rotate the selected image clockwise");
    splitter->setSizes({190, 910}); QTest::qWait(250);
    require(sidebarIsland->width() <= 205, "Tools respects a narrow sidebar");
    require(toolsScroll->horizontalScrollBar()->maximum() == 0, "Tools needs no horizontal scrolling");
    require(rotateButton->height() >= rotateButton->heightForWidth(rotateButton->width()), "long tool labels wrap without vertical clipping");
    require(rotateButton->height() > 40, "long tool action uses multiple lines at narrow widths");
    rotateButton->setFocus(); toolsScroll->ensureWidgetVisible(rotateButton); QTest::qWait(150);
    capture("context-tools-narrow");
    bool rotated = false;
    auto rotationConnection = QObject::connect(rotateAction, &QAction::triggered, window, [&] { rotated = true; });
    QTest::keyClick(rotateButton, Qt::Key_Space); QTest::qWait(200);
    require(rotated, "wrapped tool row triggers the native action by keyboard");
    QObject::disconnect(rotationConnection);
    findAction(window, "edit_undo")->trigger(); QTest::qWait(200);
    // Action rows are rebuilt after the document changes.
    rotateAction->setText(originalToolText);
    const QRect footerRect = bar->geometry();
    require(toolsScroll->verticalScrollBar()->maximum() > 0, "long Tools page scrolls vertically");
    toolsScroll->verticalScrollBar()->setValue(toolsScroll->verticalScrollBar()->maximum()); QTest::qWait(100);
    require(bar->geometry() == footerRect, "Tools scrolling keeps footer anchored");
    capture("context-tools-narrow-bottom");
    tabs->setCurrentIndex(1); QTest::qWait(200);
    require(sidebarIsland->width() <= 205, "Information respects a narrow sidebar");
    require(infoScroll->horizontalScrollBar()->maximum() == 0, "Information needs no horizontal scrolling");
    auto values = tabs->widget(1)->findChildren<QTextEdit *>("metadataValue");
    require(!values.isEmpty(), "Information shows image metadata");
    auto value = values.last(); const QString originalValue = value->toPlainText();
    value->setPlainText(QString(90, 'W') + ".png");
    value->parentWidget()->updateGeometry();
    splitter->setSizes({192, 908}); QTest::qWait(200);
    require(value->heightForWidth(value->width()) > value->fontMetrics().height() * 2, "long metadata values wrap at narrow widths");
    require(value->height() >= value->heightForWidth(value->width()), "metadata value height contains its wrapped text");
    value->selectAll(); value->copy();
    require(QApplication::clipboard()->text() == value->toPlainText(), "wrapped metadata copies its exact original text");
    value->moveCursor(QTextCursor::Start);
    capture("context-information-narrow");
    value->setPlainText(originalValue); value->parentWidget()->updateGeometry();
    splitter->setSizes({190, 910}); QTest::qWait(200);
    auto tags = tabs->widget(1)->findChildren<QLabel *>("sidebarTag");
    const int narrowTagWidth = tags.isEmpty() ? 0 : tags.first()->width();
    for (auto tag : tags) {
        require(tag->width() <= tag->parentWidget()->width(), "long tags fit the narrow panel");
        require(!tag->toolTip().isEmpty(), "elided tags retain their full labels in tooltips");
    }
    capture("context-information-narrow-normal");
    if (auto description = tabs->widget(1)->findChild<QTextEdit *>("mDescriptionTextEdit")) {
        require(!tags.isEmpty(), "long tag fixture appears in the native semantic panel");
        infoScroll->verticalScrollBar()->setValue(infoScroll->verticalScrollBar()->maximum()); QTest::qWait(150);
        require(description->width() <= infoScroll->viewport()->width(), "description editor fits narrow Information panel");
        require(description->mapTo(infoScroll->viewport(), QPoint()).y() >= 0
                && description->mapTo(infoScroll->viewport(), description->rect().bottomRight()).y() < infoScroll->viewport()->height(),
                "entire description editor is reachable by scrolling");
        capture("context-information-narrow-bottom");
    }
    splitter->setSizes({340, 760}); infoScroll->verticalScrollBar()->setValue(0); QTest::qWait(200);
    if (!tags.isEmpty()) require(tags.first()->width() > narrowTagWidth, "wider Information panel reveals more of long tags");
    capture("context-information-wide");
    auto details = window->findChild<QLabel *>("showMoreImageDetails");
    require(details && details->focusPolicy() == Qt::StrongFocus, "full image details link is keyboard accessible");
    QMetaObject::invokeMethod(details, "linkActivated", Q_ARG(QString, "#")); QTest::qWait(200);
    QDialog *detailsDialog = nullptr;
    for (auto dialog : window->findChildren<QDialog *>()) if (dialog->isVisible()) detailsDialog = dialog;
    require(detailsDialog, "full metadata dialog remains available"); detailsDialog->close();
    window->resize(contextSize); splitter->setSizes(savedSizes); QTest::qWait(200);
    fprintf(stderr, "PASS: centered welcome actions, wrapped metadata and tools, narrow panels, native tool keyboard activation and details dialog\n");

    findAction(window, "go_start_page")->trigger(); QTest::qWait(250);
    tabs->setCurrentIndex(1); require(infoEmpty->isVisible(), "welcome never shows stale image information");
    tabs->setCurrentIndex(0);
    bool menuOpened = false;
    QTimer::singleShot(120, window, [&] {
        menuOpened = more->menu()->isVisible(); capture("welcome-recents-menu");
        list->trigger(); more->menu()->close();
    });
    QTest::mouseClick(more, Qt::LeftButton); QTest::qWait(200);
    require(menuOpened && recent->property("listMode").toBool(), "overflow menu changes to filename view");
    const QRect first = recent->visualRect(recent->model()->index(0, 0));
    const QRect second = recent->visualRect(recent->model()->index(1, 0));
    require(first.width() > first.height() && second.top() > first.top(), "filename view lays out full-width rows");
    recent->setCurrentIndex(recent->model()->index(0, 0)); recent->setFocus();
    QTest::keyClick(recent, Qt::Key_Down);
    require(recent->currentIndex().row() == 1, "recent filename view supports keyboard selection");
    capture("welcome-list");
    const QSize originalSize = window->size();
    window->resize(900, 520); QTest::qWait(250);
    const QRect footer = tabs->tabBar()->geometry();
    require(recent->verticalScrollBar()->maximum() > 0, "recent filename view scrolls in a compact window");
    recent->verticalScrollBar()->setValue(recent->verticalScrollBar()->maximum()); QTest::qWait(100);
    require(tabs->tabBar()->geometry() == footer && tabs->tabBar()->isVisible(), "sidebar footer stays anchored while recent content scrolls");
    capture("welcome-compact");
    window->resize(originalSize); QTest::qWait(200);
    const QPersistentModelIndex selection(recent->currentIndex());
    grid->trigger(); QTest::qWait(150);
    require(!recent->property("listMode").toBool() && recent->currentIndex() == selection, "grid/list switching preserves selection");
    list->trigger(); QTest::qWait(150);
    const int before = recent->model()->rowCount();
    const QString removedPath = recent->currentIndex().data(Qt::ToolTipRole).toString();
    more->menu()->popup(more->mapToGlobal(QPoint(0, more->height()))); QTest::qWait(100);
    remove->trigger(); more->menu()->close(); QTest::qWait(250);
    require(recent->model()->rowCount() == before - 1 && count->text().toInt() == before - 1, "remove selected updates history and header count");
    require(QFileInfo::exists(removedPath), "remove recent leaves that exact image file intact");
    capture("welcome-final");
    fprintf(stderr, "PASS: welcome identity, configured shortcut, file/folder opening, drag highlight/reset, labeled keyboard footer, contextual empty states, recent menu, count, grid/list switching and removal\n");
}

static void verifyBrowser(QWidget *window, const QString &output, const QString &mode)
{
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    auto gallery = QUrl::fromLocalFile(fixtures + "/Gallery");
    require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, gallery)), "open gallery in native browser");
    QTest::qWait(700);
    auto page = window->findChild<QWidget *>("BrowseMainPage");
    auto view = window->findChild<QListView *>("mThumbnailView");
    auto title = window->findChild<QLabel *>("browseFolderTitle");
    auto sort = window->findChild<QToolButton *>("browseSortButton");
    auto details = window->findChild<QToolButton *>("browseDetailsButton");
    auto filters = window->findChild<QToolButton *>("mAddFilterButton");
    auto filterScroll = window->findChild<QScrollArea *>("browseFilterScroll");
    auto sliderContainer = window->findChild<QWidget *>("mThumbnailSlider");
    auto slider = sliderContainer ? sliderContainer->findChild<QSlider *>() : nullptr;
    auto empty = window->findChild<QLabel *>("browseEmptyState");
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto splitter = qobject_cast<QSplitter *>(sidebar->parentWidget());
    require(page && view && title && sort && details && filters && filterScroll && slider && empty && splitter, "browser presentation controls exist");
    auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "browser screenshot"); };
    if (mode == "browser-restart") {
        require(slider->value() == 400, "preferred thumbnail size survives close from narrow window");
        fprintf(stderr, "PASS: browser preferred thumbnail size survives restart\n"); return;
    }
    require(page->isVisible() && title->text() == "Gallery", "folder identity follows navigation");
    require(view->model()->rowCount() == 13, "gallery shows images and its folder");
    require(!empty->isVisible(), "populated folder hides empty state");
    slider->setValue(150); QTest::qWait(300);
    view->setCurrentIndex(view->model()->index(1, 0)); view->setFocus();
    QTest::keyClick(view, Qt::Key_Right); QTest::qWait(100);
    require(view->currentIndex().row() == 2, "thumbnail keyboard navigation retained");
    view->selectionModel()->select(view->model()->index(3, 0), QItemSelectionModel::Select); QTest::qWait(100);
    auto count = window->findChild<QLabel *>("mDocumentCountLabel");
    require(count && count->text().contains("Selected"), "selection count remains accurate");
    capture("browser-wide");
    bool menuShown = false;
    QTimer::singleShot(100, window, [&] { menuShown = sort->menu()->isVisible(); sort->menu()->close(); });
    QTest::mouseClick(sort, Qt::LeftButton); require(menuShown, "Sort button opens native sorting menu");
    findAction(window, "sort_desc")->setChecked(true); QTest::qWait(150);
    require(view->model()->index(1, 0).data().toString() == "scene-11.png", "native descending sort applies");
    findAction(window, "sort_desc")->setChecked(false);
    findAction(window, "view_toggle_showhiddenfiles")->setChecked(true); QTest::qWait(250);
    require(view->model()->rowCount() == 14, "View menu retains hidden-file action");
    findAction(window, "view_toggle_showhiddenfiles")->setChecked(false); QTest::qWait(150);
    auto detailAction = details->menu()->actions().at(1); detailAction->trigger(); QTest::qWait(100);
    require(detailAction->isChecked(), "thumbnail details toggle works"); detailAction->trigger();
    filters->menu()->actions().first()->trigger(); QTest::qWait(250);
    auto nameField = filterScroll->findChild<QLineEdit *>();
    require(nameField && filterScroll->isVisible(), "name filter appears beneath navigation");
    nameField->setText("scene-01"); QTest::qWait(600);
    require(view->model()->rowCount() == 1, "filename filtering works");
    nameField->setText("no-matches-expected"); QTest::qWait(600);
    require(empty->isVisible() && empty->text().contains("No matching"), "filtered empty state explains recovery"); capture("browser-no-results");
    nameField->clear(); QTest::qWait(500);
    filters->menu()->actions().last()->trigger(); QTest::qWait(200);
    const auto filterActions = filters->menu()->actions();
    for (int i = 1; i < filterActions.size() - 1; ++i) filterActions[i]->trigger();
    QTest::qWait(250);
    window->resize(720, 500); splitter->setSizes({190, 530}); QTest::qWait(350);
    require(window->width() <= 720 && window->height() <= 500, "browser permits a small window");
    require(sort->toolButtonStyle() == Qt::ToolButtonIconOnly && details->toolButtonStyle() == Qt::ToolButtonIconOnly, "small browser uses compact header actions");
    for (auto control : {static_cast<QWidget *>(sort), static_cast<QWidget *>(details), static_cast<QWidget *>(filters), sliderContainer}) {
        const QRect rect(control->mapTo(page, QPoint()), control->size());
        require(page->rect().contains(rect), "browser controls remain inside small page");
    }
    require(filterScroll->horizontalScrollBar()->maximum() == 0, "filter cards fit without horizontal scrolling");
    require(view->viewport()->height() >= 100, "multiple filters preserve thumbnail browsing space");
    filterScroll->verticalScrollBar()->setValue(filterScroll->verticalScrollBar()->maximum());
    capture("browser-small-filters");
    while (auto close = filterScroll->findChild<QToolButton *>("removeBrowseFilter")) {
        close->click(); QTest::qWait(100);
    }
    QTest::qWait(300);
    require(!filterScroll->isVisible() && view->model()->rowCount() == 13, "removing all filters restores full gallery and space");
    window->resize(640, 480); splitter->setSizes({190, 450}); QTest::qWait(250);
    require(window->width() <= 640, "browser also fits a 640px window");
    slider->setValue(400); QTest::qWait(250);
    require(view->gridSize().width() <= view->viewport()->width(), "large thumbnails fit small viewport");
    require(view->gridSize().height() <= view->viewport()->height(), "one full thumbnail and its caption fit the small viewport height");
    const int smallGrid = view->gridSize().width(); capture("browser-small");
    window->resize(1200, 760); splitter->setSizes({240, 960}); QTest::qWait(300);
    require(view->gridSize().width() > smallGrid && slider->value() == 400, "widening restores preferred thumbnail size");
    require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/Empty folder"))), "navigate to empty folder");
    QTest::qWait(500);
    require(empty->isVisible() && empty->text().contains("No images"), "empty folder explains navigation"); capture("browser-empty-folder");
    require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, gallery)), "navigate back to gallery"); QTest::qWait(350);
    findAction(window, "edit_location")->trigger(); QTest::qWait(100);
    auto location = page->findChild<QWidget *>("mUrlNavigatorContainer");
    require(location && !location->findChildren<QLineEdit *>().isEmpty(), "F6 location editing is retained");
    view->setCurrentIndex(view->model()->index(1, 0)); view->setFocus(); QTest::qWait(100);
    auto full = findAction(window, "fullscreen");
    full->trigger();
    require(window->isFullScreen(), "browser requests fullscreen");
    QTest::qWait(300);
    auto mainToolbar = window->findChild<QToolBar *>("mainToolBar");
    require(full->isChecked() && !mainToolbar->isVisible() && page->layout()->contentsMargins().left() == 0,
            "browser fullscreen presentation remains available");
    capture("browser-fullscreen");
    // The offscreen platform can revoke window state without a window manager.
    // Exercise the same native exit action used by Escape.
    findAction(window, "leave_fullscreen")->trigger(); QTest::qWait(200);
    require(!full->isChecked() && mainToolbar->isVisible() && page->layout()->contentsMargins().left() > 0,
            "leaving fullscreen restores browser presentation");
    auto showStatus = findAction(window, "options_show_statusbar");
    if (showStatus->isChecked()) showStatus->trigger(); QTest::qWait(100);
    require(!sliderContainer->isVisible() && filters->isVisible(), "status preference hides footer without losing filters");
    if (!showStatus->isChecked()) showStatus->trigger();
    view->setCurrentIndex(view->model()->index(1, 0));
    QTest::keyClick(view, Qt::Key_Return); QTest::qWait(400);
    require(!page->isVisible(), "browser activation opens native image viewer");
    findAction(window, "browse")->trigger(); QTest::qWait(200);
    window->resize(720, 500); splitter->setSizes({190, 530}); QTest::qWait(150);
    fprintf(stderr, "PASS: browser header, sorting, hidden files, details, selection, adaptive filters, empty states, narrow layout, thumbnail fitting, fullscreen and activation\n");
}

static void verifyFullscreen(QWidget *window, const QString &output)
{
    // Isolated sessions have no desktop tags worker. Dismiss only its startup
    // error so it cannot hold keyboard focus during input/auto-hide checks.
    if (auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
        for (auto label : dialog->findChildren<QLabel *>()) {
            if (label->text().contains("Unable to create KIO worker") && label->text().contains("protocol 'tags'")) {
                dialog->reject(); break;
            }
        }
    }
    QApplication::setActiveWindow(window);

    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    const auto file = QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png");
    require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{file})), "open image for fullscreen");
    QTest::qWait(650);
    auto full = findAction(window, "fullscreen");
    full->trigger(); QTest::qWait(650);
    auto bar = window->findChild<QWidget *>("fullScreenBar");
    auto surface = window->findChild<QWidget *>("fullscreenSurface");
    auto thumbs = window->findChild<QListView *>("fullscreenThumbnails");
    auto info = window->findChild<QLabel *>("fullscreenImageInformation");
    auto count = window->findChild<QLabel *>("fullscreenImageCount");
    auto viewer = window->findChild<QGraphicsView *>("roundedImageViewer");
    auto button = [&](const char *name) {
        auto b = bar->findChild<QToolButton *>(QStringLiteral("fullscreen_") + QString::fromLatin1(name));
        require(b && !b->icon().isNull() && !b->toolTip().isEmpty(), "fullscreen button has an icon and tooltip"); return b;
    };
    require(bar && bar->isVisible() && surface && thumbs && info && count && viewer, "fullscreen floating panel exists");
    auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "fullscreen screenshot"); };
    auto pin = button("keep_controls_visible"); pin->click(); QTest::qWait(200);
    require(pin->isChecked() && bar->y() == 0, "pin keeps fullscreen controls revealed");
    for (const char *name : {"go_previous", "toggle_slideshow", "go_next", "rotate_left", "rotate_right", "view_zoom_in", "view_zoom_out", "view_zoom_to_fit", "view_actual_size", "leave_fullscreen", "options", "browse", "toggle_sidebar"}) button(name);
    require(info->toolTip().contains("scene-00") && count->text().contains("of"), "image information and count are shown");
    capture("fullscreen-wide");
    button("go_next")->click(); QTest::qWait(450);
    require(info->toolTip().contains("scene-01"), "next button opens the next image");
    button("go_previous")->click(); QTest::qWait(450);
    require(info->toolTip().contains("scene-00"), "previous button opens the previous image");
    auto zoom = window->findChild<QWidget *>("imageViewControls");
    if (!zoom) zoom = window->findChild<QWidget *>("imageToolbarFlyout");
    auto slider = zoom ? zoom->findChild<QSlider *>() : nullptr;
    require(slider, "native image zoom slider exists");
    const int before = slider->value();
    button("view_zoom_in")->click(); QTest::qWait(150);
    require(slider->value() > before, "fullscreen zoom in changes actual image scale");
    button("view_zoom_out")->click(); QTest::qWait(150);
    require(slider->value() < before + 1, "fullscreen zoom out changes actual image scale");
    button("view_actual_size")->click(); QTest::qWait(100);
    require(findAction(window, "view_actual_size")->isChecked(), "actual size control follows native state");
    button("view_zoom_to_fit")->click(); QTest::qWait(100);
    require(findAction(window, "view_zoom_to_fit")->isChecked(), "fit control follows native state");
    button("rotate_right")->click(); QTest::qWait(250);
    require(findAction(window, "file_save")->isEnabled(), "fullscreen rotation edits image");
    findAction(window, "edit_undo")->trigger(); QTest::qWait(150);
    button("toggle_slideshow")->click(); QTest::qWait(150);
    require(findAction(window, "toggle_slideshow")->text().contains("Pause"), "slideshow starts from fullscreen control");
    button("toggle_slideshow")->click(); QTest::qWait(150);
    require(!findAction(window, "toggle_slideshow")->text().contains("Pause"), "slideshow pauses from same control");

    bool menuWorked = false;
    QTimer::singleShot(100, window, [&] {
        auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        require(menu && menu->objectName() == "fullscreenOptionsMenu", "fullscreen options menu opens");
        auto show = menu->findChild<QCheckBox *>("mShowThumbnailsCheckBox");
        auto height = menu->findChild<QSlider *>("mHeightSlider");
        require(show && height, "native thumbnail settings remain available");
        if (!show->isChecked()) show->click();
        height->setValue(130); QTest::qWait(100);
        require(thumbs->isVisible() && thumbs->height() <= 160, "thumbnail height responds and remains bounded");
        require(menu->grab().save(output + "/fullscreen-options.png"), "fullscreen options screenshot");
        show->click(); QTest::qWait(100);
        require(!thumbs->isVisible() && bar->height() < 180, "thumbnails can be hidden without losing controls");
        capture("fullscreen-no-thumbnails");
        show->click(); menuWorked = true; menu->close();
    });
    button("options")->click(); require(menuWorked, "options menu completed"); QTest::qWait(150);
    // Compact layout follows the image pane, including when the sidebar is open.
    window->resize(640, 480); QTest::qWait(200);
    require(window->width() <= 640 && bar->width() <= bar->parentWidget()->width(), "fullscreen panel fits narrow image pane");
    for (auto b : bar->findChildren<QToolButton *>()) {
        if (b->objectName().startsWith("fullscreen_"))
            require(surface->rect().contains(QRect(b->mapTo(surface, QPoint()), b->size())), "fullscreen controls remain inside compact surface");
    }
    capture("fullscreen-compact");
    require(thumbs->width() >= surface->width() - 60, "thumbnails span the floating panel width");
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    if (!sidebar->isVisible()) button("toggle_sidebar")->click();
    auto split = qobject_cast<QSplitter *>(sidebar->parentWidget());
    split->setSizes({190, 450}); QTest::qWait(250);
    require(bar->width() < 560, "fullscreen controls adapt to narrow pane beside sidebar");
    auto navigation = bar->findChild<QWidget *>("fullscreenNavigation");
    auto imageControls = bar->findChild<QWidget *>("fullscreenImageControls");
    require(imageControls->y() > navigation->y(), "narrow fullscreen controls stack into two groups");
    for (auto b : bar->findChildren<QToolButton *>()) {
        if (b->objectName().startsWith("fullscreen_"))
            require(b->isVisible() && b->visibleRegion().contains(b->rect().center())
                && surface->rect().contains(QRect(b->mapTo(surface, QPoint()), b->size())), "sidebar leaves every fullscreen control visible and reachable");
    }
    capture("fullscreen-sidebar-narrow");
    button("toggle_sidebar")->click(); QTest::qWait(200);
    const auto index = thumbs->model()->index(2, 0);
    thumbs->scrollTo(index); QTest::qWait(100);
    QTest::mouseClick(thumbs->viewport(), Qt::LeftButton, Qt::NoModifier, thumbs->visualRect(index).center());
    QTest::qWait(400);
    require(info->toolTip().contains("scene-02"), "fullscreen thumbnail opens selected image");

    auto pointer = [&](const QPoint &global) {
        QMouseEvent move(QEvent::MouseMove, viewer->viewport()->mapFromGlobal(global), global,
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(viewer->viewport(), &move);
    };
    pin->click();
    viewer->setFocus();
    pointer(bar->parentWidget()->mapToGlobal(QPoint(100, bar->parentWidget()->height() - 20)));
    QTest::qWait(250);
    require(bar->y() == -bar->height(), "fullscreen panel hides away from controls");
    capture("fullscreen-hidden");
    pointer(bar->parentWidget()->mapToGlobal(QPoint(100, 4))); QTest::qWait(250);
    require(bar->y() == 0, "top edge reveals fullscreen controls");
    button("go_next")->setFocus(Qt::TabFocusReason); QTest::qWait(100);
    pointer(bar->parentWidget()->mapToGlobal(QPoint(100, bar->parentWidget()->height() - 20))); QTest::qWait(250);
    require(bar->y() == 0, "keyboard focus keeps controls available");
    QTest::keyClick(button("go_next"), Qt::Key_Space); QTest::qWait(400);
    require(info->toolTip().contains("scene-03"), "keyboard activates fullscreen navigation");
    viewer->setFocus(); pointer(bar->parentWidget()->mapToGlobal(QPoint(100, bar->parentWidget()->height() - 20)));
    QTest::qWait(1400);
    require(QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::BlankCursor, "idle fullscreen hides cursor once");
    findAction(window, "leave_fullscreen")->trigger(); QTest::qWait(600);
    require(!bar->isVisible() && !QApplication::overrideCursor(), "fullscreen exit restores cursor and hides overlay");
    for (int i = 0; i < 3; ++i) { full->trigger(); QTest::qWait(50); full->trigger(); QTest::qWait(50); }
    QTest::qWait(650);
    require(!bar->isVisible() && !QApplication::overrideCursor(), "rapid fullscreen changes leave no timers or hidden cursor");
    fprintf(stderr, "PASS: fullscreen floating layout, native navigation/zoom/rotation/slideshow, settings, compact sizing, thumbnails, keyboard focus, auto-hide and cursor restoration\n");
}

static void startup() {
    if (qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY").isEmpty()) return;
    if (QFileInfo(QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget()).fileName() != QStringLiteral("gwenview")) return;
    QTimer::singleShot(1000, qApp, [] {
        QWidget *window = nullptr;
        for (auto w : QApplication::topLevelWidgets()) if (w->inherits("Gwenview::MainWindow")) window = w;
        require(window, "main window");
        window->resize(1100, 760);
        QTest::qWait(200);
        auto welcome = window->findChild<QWidget *>("imageWelcomePage");
        auto button = window->findChild<QPushButton *>("addImagesButton");
        auto recent = window->findChild<QAbstractItemView *>("recentImagesView");
        auto empty = window->findChild<QLabel *>("recentImagesEmpty");
        require(welcome && welcome->isVisible() && button && button->isVisible(), "welcome page with Add files");
        const auto mode = qEnvironmentVariable("SEQUOIA_GWENVIEW_MODE");
        auto mainToolbar = window->findChild<QToolBar *>("mainToolBar");
        require(mainToolbar && mainToolbar->isEnabled(), "Start has enabled native toolbar");
        require(mainToolbar->isVisible() == (mode != "bars-restart"), "native toolbar preference survives restart");
        require(recent && empty, "recent images sidebar");
        require(!window->findChild<QWidget *>("mRecentFoldersView"), "no recent folders start grid");
        const auto output = qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY");
        auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "screenshot"); };
        if (mode == "fullscreen") {
            verifyFullscreen(window, output);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode.startsWith("browser")) {
            verifyBrowser(window, output, mode);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode.startsWith("welcome")) {
            verifyWelcome(window, output, mode);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "bars-restart" || mode == "bars-native-restart") {
            const auto file = QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/one/picture.png");
            require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{file})), "open image after bar preference restart");
            QTest::qWait(500);
            const bool native = mode == "bars-native-restart";
            auto showStatus = findAction(window, "options_show_statusbar");
            require(showStatus->isChecked() == native && mainToolbar->isVisible() == native, "both native checkbox preferences persisted");
            require(window->findChild<QWidget *>("floatingMainToolbar")->isVisible() != native
                && window->findChild<QWidget *>("floatingImageToolbar")->isVisible() != native, "both pills follow persisted preferences");
            if (!native) {
                mainToolbar->toggleViewAction()->trigger();
                showStatus->trigger();
                QTest::qWait(250);
            }
            fprintf(stderr, "PASS: %s bar preferences survive closing and reopening\n", native ? "native" : "pill");
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "sidebar") {
            auto sidebar = window->findChild<QTabWidget *>("sidebarTabs");
            require(sidebar && sidebar->count() == 3, "three sidebar tabs");
            require(recent->model()->rowCount() == 6, "six grid preview fixtures");
            const QRect first = recent->visualRect(recent->model()->index(0, 0));
            const QRect secondCell = recent->visualRect(recent->model()->index(1, 0));
            const QRect third = recent->visualRect(recent->model()->index(2, 0));
            require(first.width() == first.height() && first.size() == secondCell.size(), "equal square preview cells");
            capture("sidebar-grid");
            fprintf(stderr, "GRID: viewport=%d first=%d,%d %dx%d second=%d,%d third=%d,%d\n", recent->viewport()->width(), first.x(), first.y(), first.width(), first.height(), secondCell.x(), secondCell.y(), third.x(), third.y());
            require(first.top() == secondCell.top(), "preview grid has multiple columns");
            require(recent->visualRect(recent->model()->index(5, 0)).top() > first.top(), "preview grid wraps into rows");
            capture("sidebar-start");
            QTest::mouseClick(recent->viewport(), Qt::LeftButton, Qt::NoModifier, first.center());
            QTest::qWait(100);
            if (welcome->isVisible()) QTest::mouseDClick(recent->viewport(), Qt::LeftButton, Qt::NoModifier, first.center());
            QTest::qWait(700);
            require(!welcome->isVisible(), "preview tile opens the full image");
            const auto file = QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/one/picture.png");
            require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{file})), "open preview image");
            QTest::qWait(1000);
            const auto second = QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/two/picture.png");
            require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{second})), "open second preview image");
            QTest::qWait(1000);
            auto viewer = window->findChild<QWidget *>("roundedImageViewer");
            verifyMainToolbar(window, second, output);
            verifyToggleProximity(window, output);
            auto floating = window->findChild<QWidget *>("floatingImageToolbar");
            auto toggle = window->findChild<QToolButton *>("imageToolbarToggle");
            auto flyout = window->findChild<QScrollArea *>("imageToolbarFlyout");
            auto controls = window->findChild<QWidget *>("imageToolbarControls");
            require(viewer && floating && toggle && flyout && controls, "floating viewer toolbar widgets");
            require(!viewer->mask().isEmpty() && !viewer->mask().contains(QPoint(0,0)), "rounded viewer corners");
            require(toggle->isVisible() && !toggle->isChecked() && !flyout->isVisible(), "toolbar starts as a circle");
            const QRect viewerBefore = viewer->geometry();
            const int anchor = floating->geometry().right();
            capture("viewer-toolbar-collapsed");
            QTest::mouseClick(toggle, Qt::LeftButton);
            QTest::qWait(60);
            capture("viewer-toolbar-animating");
            QTest::qWait(250);
            require(toggle->isChecked() && flyout->isVisible() && flyout->isEnabled(), "toolbar expanded");
            require(floating->geometry().right() == anchor && floating->width() > toggle->width(), "toolbar expands left with fixed circle");
            require(viewer->geometry() == viewerBefore, "expanding toolbar does not resize image");
            auto surface = flyout->parentWidget();
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)}) {
                QImage rendered(glass->size(), QImage::Format_ARGB32_Premultiplied);
                rendered.fill(Qt::transparent);
                glass->render(&rendered, QPoint(), QRegion(), QWidget::DrawChildren);
                require(rendered.save(output + (glass == surface ? "/toolbar-surface.png" : "/toolbar-toggle.png")), "glass screenshot");
                require(qAlpha(rendered.pixel(0, 0)) == 0, "glass rounded corners stay clear");
                const int alpha = qAlpha(rendered.pixel(rendered.width() / 2, 4));
                require(alpha == 255, "glass composites a blurred scene behind its tint");
            }
            require(!controls->autoFillBackground() && !flyout->viewport()->autoFillBackground(), "controls preserve glass background");
            auto combo = controls->findChild<QComboBox *>();
            require(combo && !controls->findChild<QWidget *>("imageZoomArrow"), "native themed zoom dropdown");
            QStyleOptionComboBox comboOption;
            comboOption.initFrom(combo);
            comboOption.editable = true;
            const QRect arrow = combo->style()->subControlRect(QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, combo);
            require(arrow.width() >= 12, "native zoom dropdown arrow hit area");
            QTest::mouseClick(combo, Qt::LeftButton, Qt::NoModifier, arrow.center());
            QTest::qWait(100);
            require(combo->view()->isVisible(), "zoom arrow opens preset menu");
            QTest::keyClick(combo->view(), Qt::Key_Escape);
            QList<QRect> buttons;
            for (auto control : controls->findChildren<QToolButton *>())
                if (control->isVisible()) buttons.append(QRect(control->mapTo(controls, QPoint()), control->size()));
            buttons.append(QRect(combo->mapTo(controls, QPoint()), combo->size()));
            std::sort(buttons.begin(), buttons.end(), [](const QRect &a, const QRect &b) { return a.left() < b.left(); });
            for (int i = 1; i < buttons.size(); ++i)
                require(buttons[i].left() - buttons[i - 1].right() >= 5, "toolbar controls have visible separation");

            // Put sharp stripes in the actual scene behind both glass pieces.
            // Uniform output across stripes proves backdrop blur, not alpha tint.
            auto graphics = qobject_cast<QGraphicsView *>(viewer);
            require(graphics, "graphics viewer for backdrop check");
            QImage stripes(graphics->viewport()->size(), QImage::Format_RGB32);
            for (int y = 0; y < stripes.height(); ++y)
                for (int x = 0; x < stripes.width(); ++x) stripes.setPixel(x, y, (x / 4) % 2 ? qRgb(255,255,255) : qRgb(0,0,0));
            auto pattern = graphics->scene()->addPixmap(QPixmap::fromImage(stripes));
            pattern->setPos(graphics->mapToScene(QPoint()));
            pattern->setTransform(graphics->transform().inverted());
            pattern->setZValue(100000);
            QTest::qWait(150);
            capture("viewer-toolbar-blur-check");
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)}) {
                const auto rendered = glass->grab().toImage();
                int low = 255, high = 0;
                for (int x = glass == surface ? 50 : 16; x < (glass == surface ? 200 : 29); ++x) {
                    const int value = qGray(rendered.pixel(x, 5));
                    low = qMin(low, value); high = qMax(high, value);
                }
                fprintf(stderr, "BLUR: %s stripe contrast=%d\n", glass == surface ? "panel" : "toggle", high - low);
                require(high - low < 15, "glass blurs high-contrast image details");
            }
            QList<QWidget *> opaqueControls{combo};
            for (auto control : controls->findChildren<QToolButton *>())
                if (control->isVisible()) opaqueControls.append(control);
            QList<QColor> redControlSamples;
            QList<QColor> redSamples;
            stripes.fill(Qt::red);
            pattern->setPixmap(QPixmap::fromImage(stripes));
            QTest::qWait(150);
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)})
                redSamples.append(glass->grab().toImage().pixelColor(glass->width() / 2, 5));
            for (auto control : opaqueControls)
                redControlSamples.append(control->grab().toImage().pixelColor(control->width() / 2, 5));
            stripes.fill(Qt::blue);
            pattern->setPixmap(QPixmap::fromImage(stripes));
            QTest::qWait(150);
            int sample = 0;
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)}) {
                const auto blue = glass->grab().toImage().pixelColor(glass->width() / 2, 5);
                require(redSamples[sample].red() - blue.red() > 25 && blue.blue() - redSamples[sample].blue() > 25,
                        "blurred backdrop refreshes when image content changes");
                ++sample;
            }
            for (int i = 0; i < opaqueControls.size(); ++i) {
                auto control = opaqueControls[i];
                const QColor blue = control->grab().toImage().pixelColor(control->width() / 2, 5);
                require(qAbs(blue.red() - redControlSamples[i].red()) <= 2
                        && qAbs(blue.blue() - redControlSamples[i].blue()) <= 2,
                        "control backgrounds stay opaque over changing image colors");
            }
            delete pattern;
            QTest::qWait(150);
            auto zoom = controls->findChild<QSlider *>();
            require(zoom && zoom->isEnabled(), "native zoom control preserved");
            const int zoomBefore = zoom->value();
            QTest::keyClick(zoom, Qt::Key_Right);
            QTest::qWait(100);
            require(zoom->value() != zoomBefore, "native zoom responds");
            capture("viewer-toolbar-expanded");
            // Switch palettes on the same live window, including an open
            // dropdown, and inspect both text roles and actual rendered icons.
            const QPalette originalPalette = qApp->palette();
            for (bool dark : {false, true, false}) {
                QPalette theme = originalPalette;
                const QColor text = dark ? QColor("#f2f3f5") : QColor("#202124");
                const QColor disabled = dark ? QColor("#a0a3ab") : QColor("#64676e");
                for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
                    for (auto role : {QPalette::Window, QPalette::Button, QPalette::Base})
                        theme.setColor(group, role, dark ? QColor("#303135") : QColor("#f5f6f8"));
                    for (auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
                        theme.setColor(group, role, group == QPalette::Disabled ? disabled : text);
                    theme.setColor(group, QPalette::Highlight, dark ? QColor("#8db3ff") : QColor("#245ac4"));
                    theme.setColor(group, QPalette::HighlightedText, dark ? QColor("#15213a") : QColor(Qt::white));
                }
                combo->showPopup();
                QApplication::setPalette(theme);
                QTest::qWait(200);
                require(combo->view()->isVisible(), "theme change preserves open zoom menu");
                require(combo->lineEdit()->palette().color(QPalette::Text) == text
                        && combo->view()->palette().color(QPalette::Text) == text,
                        "native zoom field and open menu text follow live theme");
                for (auto label : controls->findChildren<QLabel *>())
                    require(label->palette().color(QPalette::WindowText) == text, "toolbar labels follow live theme");
                for (auto button : floating->findChildren<QToolButton *>()) {
                    if (button->icon().isNull()) continue;
                    for (auto mode : {QIcon::Normal, QIcon::Disabled}) {
                        for (auto state : {QIcon::Off, QIcon::On}) {
                            const auto icon = button->icon().pixmap(QSize(22,22), 2.0, mode, state).toImage().convertToFormat(QImage::Format_ARGB32);
                            const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
                            const QColor expected = theme.color(group, button != toggle && state == QIcon::On
                                                                 ? QPalette::HighlightedText : QPalette::ButtonText);
                            int pixels = 0;
                            for (int y = 0; y < icon.height(); ++y) for (int x = 0; x < icon.width(); ++x) {
                                const QColor pixel = icon.pixelColor(x,y);
                                if (pixel.alpha() < 200) continue;
                                ++pixels;
                                const bool matches = qAbs(pixel.red() - expected.red()) <= 2 && qAbs(pixel.green() - expected.green()) <= 2
                                    && qAbs(pixel.blue() - expected.blue()) <= 2;
                                if (!matches) fprintf(stderr, "ICON: dark=%d button=%s text=%s mode=%d state=%d actual=%s expected=%s palette=%s alpha=%d\n",
                                    dark, qPrintable(button->objectName()), qPrintable(button->text()), mode, state,
                                    qPrintable(pixel.name()), qPrintable(expected.name()), qPrintable(button->palette().color(QPalette::ButtonText).name()), pixel.alpha());
                                require(matches, "rendered toolbar icons follow live theme and state");
                            }
                            require(pixels > 0, "theme-aware toolbar icon remains visible");
                        }
                    }
                }
                require(combo->view()->window()->grab().save(output + (dark ? "/zoom-menu-live-dark.png" : "/zoom-menu-live-light.png")), "native themed zoom menu screenshot");
                combo->hidePopup();
                QTest::qWait(100);
                require(!combo->lineEdit()->text().isEmpty(), "custom zoom text survives popup and live theme changes");
                capture(dark ? "viewer-toolbar-live-dark" : "viewer-toolbar-live-light");
            }
            QApplication::setPalette(originalPalette);
            QTest::qWait(200);
            const int expandedWidth = floating->width();
            for (int cycle = 0; cycle < 3; ++cycle) {
                QTest::mouseClick(toggle, Qt::LeftButton);
                QTest::qWait(60);
                require(floating->width() > toggle->width() && floating->width() < expandedWidth, "repeated collapse slides through intermediate widths");
                require(floating->geometry().right() == anchor, "collapse keeps circle anchored");
                QTest::qWait(250);
                require(!flyout->isVisible() && floating->width() == toggle->width(), "repeated collapse finishes");
                QTest::mouseClick(toggle, Qt::LeftButton);
                QTest::qWait(60);
                require(floating->width() > toggle->width() && floating->width() < expandedWidth, "repeated expansion slides through intermediate widths");
                require(floating->geometry().right() == anchor, "expansion keeps circle anchored");
                QTest::qWait(250);
                require(flyout->isVisible() && floating->width() == expandedWidth, "repeated expansion finishes");
            }
            QTest::mouseClick(toggle, Qt::LeftButton);
            QTest::qWait(300);
            require(!flyout->isVisible() && floating->width() == toggle->width(), "toolbar collapses fully");
            // Reverse an in-flight opening animation without leaving controls behind.
            QTest::mouseClick(toggle, Qt::LeftButton);
            QTest::qWait(40);
            const int reversalWidth = floating->width();
            QTest::mouseClick(toggle, Qt::LeftButton);
            require(floating->width() == reversalWidth, "reversal preserves visible position without a jump");
            QTest::qWait(30);
            require(floating->width() > toggle->width() && floating->width() < reversalWidth, "reversal slides inward");
            QTest::qWait(300);
            require(!flyout->isVisible(), "animation reversal collapses cleanly");
            QAction *fullscreenAction = nullptr;
            for (auto widget : window->findChildren<QWidget *>())
                for (auto action : widget->actions()) if (action->objectName() == "fullscreen") fullscreenAction = action;
            require(fullscreenAction, "native fullscreen action");
            auto mainToggle = window->findChild<QToolButton *>("mainToolbarToggle");
            auto mainIsland = window->findChild<QWidget *>("floatingMainToolbar");
            auto mainFlyout = window->findChild<QScrollArea *>("mainToolbarFlyout");
            QTest::mouseClick(mainToggle, Qt::LeftButton);
            QTest::qWait(300);
            require(mainFlyout->isVisible(), "main toolbar expanded before fullscreen");
            fullscreenAction->trigger();
            QTest::qWait(400);
            require(viewer->mask().isEmpty(), "fullscreen keeps full image area");
            require(!window->findChild<QToolBar *>("mainToolBar")->isVisible()
                    && !mainIsland->isVisible() && !mainToggle->isVisible() && !mainFlyout->isVisible(),
                    "fullscreen hides the main toolbar, flyout, and toggle");
            fullscreenAction->trigger();
            QTest::qWait(400);
            require(!viewer->mask().isEmpty() && toggle->isVisible(), "rounded viewer and toggle return after fullscreen");
            require(mainToggle->isVisible() && !mainToggle->isChecked() && !mainFlyout->isVisible(),
                    "main toolbar returns collapsed after fullscreen");
            QTest::mouseClick(mainToggle, Qt::LeftButton);
            QTest::qWait(300);
            require(mainFlyout->isVisible(), "main toolbar opens after leaving fullscreen");
            QTest::mouseClick(mainToggle, Qt::LeftButton);
            QTest::qWait(300);

            for (int i = 0; i < sidebar->count(); ++i) {
                QTest::mouseClick(sidebar->tabBar(), Qt::LeftButton, Qt::NoModifier, sidebar->tabBar()->tabRect(i).center());
                QTest::qWait(200);
                require(sidebar->currentIndex() == i, "sidebar tab click");
                const auto name = QStringLiteral("sidebar-%1").arg(i).toUtf8();
                capture(name.constData());
            }
            sidebar->setCurrentIndex(0);
            auto panel = window->findChild<QWidget *>("sidebarIsland");
            auto splitter = qobject_cast<QSplitter *>(panel->parentWidget());
            require(splitter, "sidebar splitter");
            splitter->setSizes({360, 740});
            QTest::qWait(500);
            const QRect widerFirst = recent->visualRect(recent->model()->index(0,0));
            const QRect widerThird = recent->visualRect(recent->model()->index(2,0));
            require(widerFirst.top() == widerThird.top(), "wider sidebar adds columns");
            capture("sidebar-wide");
            window->resize(760, 640);
            QTest::qWait(300);
            QTest::mouseClick(toggle, Qt::LeftButton);
            QTest::qWait(300);
            require(floating->x() >= 0 && floating->geometry().right() < viewer->width(), "toolbar fits narrow viewer");
            fprintf(stderr, "TOOLBAR: window=%d viewer=%d panel=%d controls=%d scroll=%d\n", window->width(), viewer->width(), flyout->width(), controls->width(), flyout->horizontalScrollBar()->maximum());
            capture("viewer-toolbar-narrow");
            require(controls->width() <= flyout->viewport()->width() || flyout->horizontalScrollBar()->isVisible(), "narrow toolbar keeps all controls reachable");
            capture("viewer-toolbar-narrow");
            QTest::mouseClick(toggle, Qt::LeftButton);
            QTest::qWait(300);

            auto thumbnailIsland = window->findChild<QWidget *>("thumbnailBarIsland");
            require(thumbnailIsland, "rounded thumbnail island exists");
            auto thumbnails = thumbnailIsland->findChild<QAbstractItemView *>("thumbnailBarView");
            auto thumbnailSplitter = qobject_cast<QSplitter *>(thumbnailIsland->parentWidget());
            require(thumbnails && thumbnailSplitter, "native thumbnail view remains in resizable area");
            QAction *thumbnailsAction = nullptr;
            for (auto widget : window->findChildren<QWidget *>())
                for (auto action : widget->actions()) if (action->objectName() == "toggle_thumbnailbar") thumbnailsAction = action;
            require(thumbnailsAction && !thumbnailIsland->isVisible(), "thumbnails start hidden");
            QList<QUrl> images;
            for (const auto name : {"one", "two", "three", "four", "five", "six"})
                images.append(QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + '/' + name + "/picture.png"));
            require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, images)), "open thumbnail navigation fixtures");
            QTest::qWait(700);
            thumbnailsAction->trigger();
            QTest::qWait(350);
            require(thumbnailIsland->isVisible() && thumbnails->isVisible() && thumbnails->model()->rowCount() == 6, "thumbnail toggle reveals complete native view");
            require(thumbnails->selectionMode() == QAbstractItemView::ExtendedSelection, "thumbnail comparison selection preserved");
            const QRect viewerRect(viewer->mapTo(window, QPoint()), viewer->size());
            const QRect islandRect(thumbnailIsland->mapTo(window, QPoint()), thumbnailIsland->size());
            require(!viewerRect.intersects(islandRect), "thumbnail panel has separate space outside image");
            const bool horizontalStrip = thumbnailSplitter->orientation() == Qt::Vertical;
            if (horizontalStrip) {
                require(islandRect.left() == viewerRect.left() && islandRect.right() == viewerRect.right(),
                        "horizontal thumbnail surface has exactly the viewer's width and aligned edges");
            } else {
                require(islandRect.top() == viewerRect.top() && islandRect.bottom() == viewerRect.bottom(),
                        "vertical thumbnail surface matches viewer height");
            }
            auto imagePage = window->findChild<QWidget *>("imageViewPage");
            require(imagePage, "image page owns the shared shadow gutters");
            QImage pageShadows(imagePage->size(), QImage::Format_ARGB32_Premultiplied);
            pageShadows.fill(Qt::transparent);
            imagePage->render(&pageShadows, QPoint(), QRegion(), QWidget::RenderFlags());
            for (QWidget *panel : {viewer, thumbnailIsland}) {
                const QPoint shadowPoint = panel->mapTo(imagePage, QPoint(panel->width() + 2, panel->height() / 2));
                require(pageShadows.rect().contains(shadowPoint) && qAlpha(pageShadows.pixel(shadowPoint)) > 0,
                        "viewer and thumbnail panel both paint shadows outside their right edges");
            }
            require(pageShadows.save(output + "/viewer-panel-shadows.png"), "viewer and thumbnail shadow capture");
            require(thumbnails->x() >= 8 && thumbnails->y() >= 8, "thumbnail view inset inside rounded panel");
            QImage islandImage(thumbnailIsland->size(), QImage::Format_ARGB32_Premultiplied);
            islandImage.fill(Qt::transparent);
            thumbnailIsland->render(&islandImage, QPoint(), QRegion(), QWidget::DrawChildren);
            require(qAlpha(islandImage.pixel(0, 0)) == 0 && qAlpha(islandImage.pixel(16, 2)) == 255, "full-width rounded thumbnail surface with clear corners");
            capture("viewer-thumbnail-island");
            const auto lastThumbnail = thumbnails->model()->index(5, 0);
            thumbnails->scrollTo(lastThumbnail);
            QTest::qWait(350);
            QTest::mouseClick(thumbnails->viewport(), Qt::LeftButton, Qt::NoModifier, thumbnails->visualRect(lastThumbnail).center());
            QTest::qWait(350);
            require(thumbnails->currentIndex() == lastThumbnail, "thumbnail click selects another image");
            const auto sizesBefore = thumbnailSplitter->sizes();
            thumbnailSplitter->setSizes({sizesBefore[0] - 30, sizesBefore[1] + 30});
            QTest::qWait(200);
            require(thumbnailSplitter->sizes()[1] > sizesBefore[1], "thumbnail panel remains resizable");
            const QSize shownSize = thumbnailIsland->size();
            thumbnailsAction->trigger();
            QTest::qWait(150);
            require(!thumbnailIsland->isVisible(), "toggle hides entire thumbnail island");
            thumbnailsAction->trigger();
            QTest::qWait(150);
            require(thumbnailIsland->isVisible() && thumbnailIsland->size() == shownSize, "toggle restores thumbnail panel size");
            fullscreenAction->trigger();
            QTest::qWait(250);
            require(!thumbnailIsland->isVisible(), "fullscreen hides entire thumbnail island");
            require(!mainIsland->isVisible(), "fullscreen also hides a collapsed main toolbar");
            fullscreenAction->trigger();
            QTest::qWait(250);
            require(thumbnailIsland->isVisible(), "thumbnail island returns after fullscreen");
            capture(horizontalStrip ? "viewer-thumbnails-horizontal" : "viewer-thumbnails-vertical");
            fprintf(stderr, "PASS: rounded %s thumbnail area, selection, resizing, visibility, fullscreen restoration\n", horizontalStrip ? "horizontal" : "vertical");
            fprintf(stderr, "PASS: preview grid, tabs, rounded viewer, blurred glass, opaque controls, native zoom dropdown, live light/dark text and icons, repeated slides/reversal, zoom, narrow layout, fullscreen\n");
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "restart") {
            require(recent->model()->rowCount() == 2, "persistent recent images");
            require(recent->model()->index(0, 0).data(Qt::ToolTipRole).toString().endsWith("one/picture.png"), "persistent most recent first");
            capture("welcome-recent");
            fprintf(stderr, "PASS: persistent recent images ordered newest first\n");
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "disabled") {
            require(recent->model()->rowCount() == 0 && empty->isVisible(), "history disabled hides old images");
            require(empty->text().contains("disabled"), "history disabled explanation");
            capture("history-disabled");
            fprintf(stderr, "PASS: disabled history respected\n");
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        require(recent->model()->rowCount() == 0 && empty->isVisible(), "empty history message");
        capture("welcome-empty");
        fprintf(stderr, "CHECK: file picker\n");
        QTest::mouseClick(button, Qt::LeftButton);
        QTest::qWait(300);
        QFileDialog *dialog = window->findChild<QFileDialog *>();
        require(dialog && dialog->isVisible(), "Add files opens file picker");
        require(dialog->fileMode() == QFileDialog::ExistingFiles, "file picker permits multiple images");
        dialog->reject(); QTest::qWait(150);
        QMimeData text; text.setText("not an image");
        QDragEnterEvent invalid(QPoint(80,80), Qt::CopyAction, &text, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(welcome, &invalid);
        require(!invalid.isAccepted(), "text drops rejected");
        const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(fixtures + "/one/picture.png"), QUrl::fromLocalFile(fixtures + "/two/picture.png"), QUrl::fromLocalFile(fixtures + "/one/picture.png")});
        QDragEnterEvent enter(QPoint(80,80), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(welcome, &enter);
        require(enter.isAccepted(), "image drag accepted");
        QDropEvent drop(QPointF(80,80), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        fprintf(stderr, "CHECK: drop\n");
        QApplication::sendEvent(welcome, &drop);
        require(drop.isAccepted(), "image drop accepted");
        QTest::qWait(1800);
        require(!welcome->isVisible(), "drop opens native viewer");
        auto action = [&](const char *name) { QList<QWidget *> widgets = window->findChildren<QWidget *>(); widgets.prepend(window);
            for (auto w : widgets) for (auto a : w->actions()) if (a->objectName() == name) return a;
            for (auto w : widgets) for (auto a : w->actions()) fprintf(stderr, "ACTION: %s %s\n", qPrintable(a->objectName()), qPrintable(a->text()));
            require(false, name); return static_cast<QAction *>(nullptr); };
        require(recent->model()->rowCount() == 1, "viewed image added to history");
        require(recent->model()->index(0,0).data(Qt::ToolTipRole).toString() == fixtures + "/one/picture.png", "history uses original path");
        action("go_next")->trigger(); QTest::qWait(1200);
        require(recent->model()->rowCount() == 2, "next opens second same-named selected image");
        require(recent->model()->index(0,0).data(Qt::ToolTipRole).toString() == fixtures + "/two/picture.png", "newest image first");
        action("go_next")->trigger(); QTest::qWait(600);
        require(recent->model()->rowCount() == 2, "duplicate selected URL omitted");
        require(recent->model()->index(0,0).data(Qt::ToolTipRole).toString() == fixtures + "/one/picture.png", "native navigation wraps to first selected image");
        action("go_next")->trigger(); QTest::qWait(600);
        action("go_previous")->trigger(); QTest::qWait(700);
        require(recent->model()->rowCount() == 2, "reopened image not duplicated");
        require(recent->model()->index(0,0).data(Qt::ToolTipRole).toString() == fixtures + "/one/picture.png", "reopened image moves first");
        action("rotate_right")->trigger(); QTest::qWait(400);
        require(action("file_save")->isEnabled(), "native editing available");
        action("file_save")->trigger(); QTest::qWait(1000);
        require(QImage(fixtures + "/one/picture.png").size() == QSize(80,120), "save through selected-file link updates original");
        capture("viewer-recent");
        action("go_start_page")->trigger(); QTest::qWait(300);
        require(welcome->isVisible() && recent->isVisible(), "return to clean welcome with recents");
        // Activate a real thumbnail, using the view's native activation signal.
        auto index = recent->model()->index(0,0);
        require(QMetaObject::invokeMethod(recent, "indexActivated", Q_ARG(QModelIndex,index)), "recent thumbnail activation");
        QTest::qWait(800);
        require(!welcome->isVisible(), "recent thumbnail opens native viewer");
        action("go_start_page")->trigger(); QTest::qWait(200);
        capture("welcome-recent");
        fprintf(stderr, "PASS: welcome, picker, drag/drop, multi-file navigation, MRU, native edit/save, recent activation\n");
        require(window->close(), "clean close"); QCoreApplication::exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
