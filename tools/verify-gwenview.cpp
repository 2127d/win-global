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
#include <QStyleOptionComboBox>
#include <QLineEdit>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <algorithm>
#include <QLabel>
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
#include <QDropEvent>
#include <QMouseEvent>
#include <cstdio>
#include <cstdlib>
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
        auto opacity = [&] { return toggle->property("revealOpacity").toReal(); };
        toggle->clearFocus();
        moveTo(viewer->rect().center()); QTest::qWait(250);
        require(opacity() == 0 && island->testAttribute(Qt::WA_TransparentForMouseEvents), "collapsed toggle invisible and passes image input through");
        const QPoint nearby = toggle->mapTo(viewer, toggle->rect().center()) - QPoint(60, 0);
        moveTo(nearby); QTest::qWait(60);
        require(opacity() > 0 && opacity() < 1, "nearby cursor smoothly fades toggle in");
        QTest::qWait(200);
        require(opacity() == 1 && !island->testAttribute(Qt::WA_TransparentForMouseEvents), "revealed toggle usable before pointer reaches button");
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
    require(!docked->isVisible() && !docked->isEnabled(), "docked toolbar hidden and disabled in image view");
    require(!docked->toggleViewAction()->isEnabled(), "docked toolbar cannot be toggled back in image view");
    docked->show();
    QTest::qWait(200);
    require(!docked->isVisible(), "restoring docked toolbar cannot bypass image mode");
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
    require(docked->isVisible() && docked->isEnabled() && !island->isVisible(), "Browse restores docked toolbar");
    require(window->grab().save(output + "/browse-rounded.png"), "rounded Browse capture");
    require(docked->toggleViewAction()->isEnabled(), "Browse restores toolbar visibility control");
    require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{image})), "return to image view");
    QTest::qWait(500);
    require(!docked->isVisible() && !docked->isEnabled() && toggle->isVisible() && !toggle->isChecked(), "return to viewer collapses main island");
    QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(300);
    QTest::mouseClick(button("go_start_page"), Qt::LeftButton); QTest::qWait(300);
    require(docked->isVisible() && docked->isEnabled() && !island->isVisible(), "Start restores docked toolbar");
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
    fprintf(stderr, "PASS: floating main toolbar actions, animation, docked toolbar suppression, Browse/Start restoration\n");
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
        auto mainToolbar = window->findChild<QToolBar *>("mainToolBar");
        require(mainToolbar && mainToolbar->isVisible() && mainToolbar->isEnabled(), "Start has usable docked toolbar, including after restart");
        require(recent && empty, "recent images sidebar");
        require(!window->findChild<QWidget *>("mRecentFoldersView"), "no recent folders start grid");
        const auto output = qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY");
        auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "screenshot"); };
        const auto mode = qEnvironmentVariable("SEQUOIA_GWENVIEW_MODE");
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
            require(first.top() == secondCell.top() && third.top() > first.top(), "preview columns and rows");
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
            require(thumbnails->x() >= 8 && thumbnails->y() >= 8, "thumbnail view inset inside rounded panel");
            QImage islandImage(thumbnailIsland->size(), QImage::Format_ARGB32_Premultiplied);
            islandImage.fill(Qt::transparent);
            thumbnailIsland->render(&islandImage, QPoint(), QRegion(), QWidget::DrawChildren);
            require(qAlpha(islandImage.pixel(0, 0)) == 0 && qAlpha(islandImage.pixel(16, 2)) == 255, "rounded thumbnail surface with clear corners");
            capture("viewer-thumbnail-island");
            const bool horizontalStrip = thumbnailSplitter->orientation() == Qt::Vertical;
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
