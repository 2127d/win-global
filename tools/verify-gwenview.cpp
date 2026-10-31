// SPDX-License-Identifier: MIT
// Test-only startup hook: exercises the real GUI with isolated configuration.
#include <QBitArray>
#include <QSaveFile>
#include <QStandardPaths>
#include <QMenuBar>
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
#include <QGraphicsEffect>
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
#include <QWheelEvent>
#include <QElapsedTimer>
#include <QVariantAnimation>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStackedWidget>
#include <QWidgetAction>
#include <QWindow>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <sys/xattr.h>
// Observe actual blur requests from both the desktop style and Gwenview while
// forwarding to KWindowSystem. This also works in the isolated offscreen GUI.
namespace KWindowEffects {
void enableBlurBehind(QWindow *window, bool enabled, const QRegion &region)
{
    using Function = void (*)(QWindow *, bool, const QRegion &);
    static auto real = reinterpret_cast<Function>(dlsym(RTLD_NEXT, "_ZN14KWindowEffects16enableBlurBehindEP7QWindowbRK7QRegion"));
    if (window) {
        window->setProperty("verifiedBlurRequested", enabled);
        window->setProperty("verifiedBlurRegion", QVariant::fromValue(region));
        window->setProperty("verifiedVisibleBlur", enabled && (region.isEmpty() || region.intersects(QRect(QPoint(), window->size()))));
        window->setProperty("verifiedBlurRequests", window->property("verifiedBlurRequests").toInt() + 1);
    }
    if (real) real(window, enabled, region);
}
}
static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
#ifdef GWENVIEW_NATIVE_MENU_INPUT
#include "test-support/gwenview-menu-input.h"
#endif
class NoSidebarMotionStyle : public QProxyStyle
{
public:
    NoSidebarMotionStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))) {}
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *data = nullptr) const override
    {
        if (hint == SH_Widget_Animation_Duration) return 0;
        return QProxyStyle::styleHint(hint, option, widget, data);
    }
};

static void verifySidebarSlide(QTabWidget *tabs, const QString &output)
{
    auto animation = tabs->findChild<QVariantAnimation *>("sidebarSlideAnimation");
    require(animation && animation->duration() == 200, "sidebar slide stays brief");
    tabs->setCurrentIndex(0); QTest::qWait(240);
    const QPoint origin = tabs->currentWidget()->pos();
    const QRect footer = tabs->tabBar()->geometry();
    tabs->setCurrentIndex(1);
    require(tabs->currentIndex() == 1 && tabs->currentWidget()->isEnabled(), "sidebar selection takes effect immediately");
    QTest::qWait(45);
    require(animation->state() == QAbstractAnimation::Running, "sidebar visibly slides between pages");
    require(tabs->widget(0)->x() < origin.x() && tabs->widget(1)->x() > origin.x(), "forward sidebar slide has direction");
    require(tabs->tabBar()->geometry() == footer, "sidebar footer stays anchored during slide");
    require(tabs->window()->grab().save(output + "/sidebar-slide-midway.png"), "capture sidebar transition");
    QTest::qWait(240);
    require(animation->state() == QAbstractAnimation::Stopped && tabs->currentWidget()->pos() == origin
        && !tabs->widget(0)->isVisible(), "sidebar settles without a lingering outgoing page");
    tabs->setCurrentIndex(0); QTest::qWait(35);
    require(tabs->widget(0)->x() < origin.x(), "reverse navigation slides back");
    tabs->setCurrentIndex(2); QTest::qWait(20);
    tabs->setCurrentIndex(1); QTest::qWait(20);
    tabs->setCurrentIndex(0); QTest::qWait(240);
    for (int i = 0; i < tabs->count(); ++i)
        require(tabs->widget(i)->isVisible() == (i == 0) && tabs->widget(i)->pos() == origin,
                "rapid sidebar switching leaves only the selected page visible and aligned");
    tabs->tabBar()->setFocus();
    QTest::keyClick(tabs->tabBar(), Qt::Key_Right); QTest::qWait(240);
    require(tabs->currentIndex() == 1, "keyboard sidebar switching works");
    tabs->setCurrentIndex(2); QTest::qWait(30);
    const QSize size = tabs->window()->size();
    tabs->window()->resize(size + QSize(70, 30)); QTest::qWait(60);
    require(animation->state() == QAbstractAnimation::Stopped && tabs->currentWidget()->pos() == origin,
            "resizing cancels the slide without displacing content");
    tabs->window()->resize(size); QTest::qWait(100);
    NoSidebarMotionStyle noMotion;
    tabs->setStyle(&noMotion);
    tabs->setCurrentIndex(0);
    require(animation->state() == QAbstractAnimation::Stopped && tabs->currentWidget()->pos() == origin,
            "disabled desktop animations switch sidebar pages immediately");
    tabs->setStyle(nullptr);
    QTest::qWait(100);
    fprintf(stderr, "PASS: brief sidebar slide, fixed footer, rapid reversal, keyboard, resize and disabled animations\n");
}

static QAction *findAction(QWidget *window, const char *name);

class ViewPaintCounter : public QObject
{
public:
    int paints = 0;
    bool eventFilter(QObject *, QEvent *event) override { if (event->type() == QEvent::Paint) ++paints; return false; }
};

static void verifyMainPageSlide(QWidget *window, const QString &output)
{
    auto stack = window->findChild<QStackedWidget *>("mainPageStack");
    auto animation = stack ? stack->findChild<QVariantAnimation *>("mainPageSlideAnimation") : nullptr;
    auto overlay = stack ? stack->findChild<QWidget *>("mainPageSlide") : nullptr;
    auto browse = window->findChild<QListView *>("mThumbnailView");
    require(stack && animation && overlay && browse && animation->duration() == 200, "quick main-page slide exists");
    auto toBrowse = findAction(window, "browse"), toView = findAction(window, "view");
    require(toBrowse && toView, "native view mode actions");
    QWidget *imagePage = stack->currentWidget();
    toBrowse->trigger();
    require(stack->currentWidget() != imagePage, "browser becomes current without waiting for transition");
    QTest::qWait(40);
    require(overlay->isVisible() && animation->state() == QAbstractAnimation::Running, "browser/image transition visibly slides");
    require(overlay->testAttribute(Qt::WA_TransparentForMouseEvents), "transition overlay never intercepts input");
    require(window->grab().save(output + "/browser-image-slide.png"), "capture main-page slide");
    ViewPaintCounter paints; browse->viewport()->installEventFilter(&paints);
    int frames = 0;
    const auto connection = QObject::connect(animation, &QVariantAnimation::valueChanged, window, [&] { ++frames; });
    QTest::qWait(50);
    QObject::disconnect(connection);
    browse->viewport()->removeEventFilter(&paints);
    require(frames >= 2 && paints.paints <= 1, "slide animates cached frames without repainting the thumbnail grid each frame");
    QTest::qWait(160);
    require(!overlay->isVisible() && animation->state() == QAbstractAnimation::Stopped, "main-page slide ends promptly");
    toView->trigger(); QTest::qWait(30);
    toBrowse->trigger(); QTest::qWait(25);
    toView->trigger(); QTest::qWait(240);
    require(stack->currentWidget() == imagePage && !overlay->isVisible(), "rapid view changes do not queue stale transitions");
    toBrowse->trigger(); QTest::qWait(30);
    QTest::keyPress(stack, Qt::Key_Shift);
    require(!overlay->isVisible(), "input dismisses transition immediately");
    QTest::keyRelease(stack, Qt::Key_Shift);
    toView->trigger();
    QTest::keyPress(stack, Qt::Key_Shift);
    QTest::keyRelease(stack, Qt::Key_Shift);
    QTest::qWait(30);
    require(!overlay->isVisible() && animation->state() == QAbstractAnimation::Stopped,
            "input before the first frame skips the pending transition");
    NoSidebarMotionStyle noMotion;
    stack->setStyle(&noMotion);
    toBrowse->trigger();
    toView->trigger(); QTest::qWait(30);
    require(stack->currentWidget() == imagePage && !overlay->isVisible(), "disabled animations switch view modes immediately");
    stack->setStyle(nullptr); QTest::qWait(240);
    fprintf(stderr, "PASS: quick cached view slides, immediate mode changes/input, rapid reversal and disabled motion\n");
}

static void verifyHomeSlide(QWidget *window, const QString &output)
{
    auto stack = window->findChild<QStackedWidget *>("mainPageStack");
    auto animation = stack->findChild<QVariantAnimation *>("mainPageSlideAnimation");
    auto overlay = stack->findChild<QWidget *>("mainPageSlide");
    QWidget *imagePage = stack->currentWidget();
    auto home = findAction(window, "go_start_page");
    const QString folder = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/two";
    for (bool fromImage : {true, false}) {
        if (fromImage) { findAction(window, "toggle_sidebar")->setChecked(false); QTest::qWait(230); }
        home->trigger(); QTest::qWait(40);
        require(overlay->isVisible() && animation->state() == QAbstractAnimation::Running, "Home slides in from both image and Browse views");
        require(window->grab().save(output + (fromImage ? "/image-home-slide.png" : "/browse-home-slide.png")), "capture Home slide");
        QTest::qWait(150);
        if (fromImage) {
            require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, QUrl::fromLocalFile(folder))), "leave Home for Browse");
            QTest::qWait(40);
            require(overlay->isVisible(), "leaving Home also slides");
            QTest::qWait(240);
        }
    }
    auto recent = window->findChild<QAbstractItemView *>("recentImagesView");
    const QPoint first = recent->visualRect(recent->model()->index(0, 0)).center();
    QTest::mouseClick(recent->viewport(), Qt::LeftButton, Qt::NoModifier, first); QTest::qWait(80);
    if (stack->currentWidget() != imagePage) QTest::mouseDClick(recent->viewport(), Qt::LeftButton, Qt::NoModifier, first);
    QTest::qWait(300);
    require(stack->currentWidget() == imagePage && !overlay->isVisible(), "Home round trip restores image controls");
    fprintf(stderr, "PASS: quick cached Home/Browse/View slides, immediate mode changes/input, rapid reversal and disabled motion\n");
}

static void verifyZoomPopup(QComboBox *combo, QSlider *slider)
{
    require(slider, "zoom slider exists for popup checks");
    const int originalZoom = slider->value();
    const QString originalText = combo->lineEdit()->text();
    auto view = combo->view();
    auto viewport = view->viewport();
    auto bar = view->verticalScrollBar();
    require(bar->maximum() > bar->minimum(), "zoom presets overflow the popup");
    auto hover = [&] {
        const QPoint point = viewport->rect().center();
        QMouseEvent move(QEvent::MouseMove, point, viewport->mapToGlobal(point),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(viewport, &move);
        QTest::qWait(20);
    };
    hover();
    require(slider->value() == originalZoom, "hovering zoom presets does not change image scale");
    for (int direction : {120, -120, 120}) {
        for (int step = 0; step < combo->count(); ++step) {
            const int before = bar->value();
            const QPoint point = viewport->rect().center();
            QWheelEvent wheel(point, viewport->mapToGlobal(point), QPoint(), QPoint(0, direction),
                              Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(viewport, &wheel);
            hover();
            require(direction > 0 ? bar->value() <= before : bar->value() >= before,
                    "zoom popup scrolls monotonically without launching backward");
            require(slider->value() == originalZoom, "scrolling zoom presets does not change image scale");
        }
        require(bar->value() == (direction > 0 ? bar->minimum() : bar->maximum()),
                "wheel scrolling reaches both ends of zoom presets");
    }
    QTest::keyClick(view, Qt::Key_Escape);
    QTest::qWait(50);
    require(slider->value() == originalZoom && combo->lineEdit()->text() == originalText,
            "cancelling zoom popup preserves image scale and text");
    combo->showPopup();
    QTest::qWait(50);
    QTest::keyClick(view, Qt::Key_Up);
    require(slider->value() == originalZoom, "keyboard browsing waits for explicit zoom selection");
    QTest::keyClick(view, Qt::Key_Return);
    QTest::qWait(100);
    require(!view->isVisible() && slider->value() != originalZoom, "Enter commits the selected zoom preset");
    combo->showPopup();
    QTest::qWait(50);
    int actualSizeIndex = -1;
    for (int index = 0; index < combo->count(); ++index) {
        auto action = combo->itemData(index).value<QAction *>();
        if (action && action->objectName() == QLatin1String("view_actual_size")) actualSizeIndex = index;
    }
    require(actualSizeIndex >= 0, "actual size preset available");
    const auto index = combo->model()->index(actualSizeIndex, 0);
    view->scrollTo(index);
    QTest::qWait(30);
    QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, view->visualRect(index).center());
    QTest::qWait(100);
    require(!view->isVisible() && combo->itemData(actualSizeIndex).value<QAction *>()->isChecked(),
            "click commits the chosen zoom preset");
    fprintf(stderr, "PASS: zoom popup hover, bidirectional wheel endpoints, cancellation, keyboard and mouse selection\n");
}

static void verifyClosedZoomWheel(QComboBox *combo, QSlider *slider, QAction *fit)
{
    auto scale = [&] { return combo->property("value").toReal(); };
    auto wheel = [&](int delta, bool pixels = false, QWidget *target = nullptr) {
        if (!target) target = combo;
        const QPoint point = target->rect().center();
        QWheelEvent event(point, target->mapToGlobal(point), pixels ? QPoint(0, delta) : QPoint(),
                          pixels ? QPoint() : QPoint(0, delta), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(target, &event);
        QTest::qWait(15);
        require(!combo->view()->isVisible(), "hover wheel never opens the zoom popup");
    };
    auto near = [](qreal a, qreal b) { return qAbs(a - b) < 1e-5; };
    fit->trigger(); QTest::qWait(70);
    const qreal minimum = combo->property("minimum").toReal();
    const qreal maximum = combo->property("maximum").toReal();
    QList<qreal> presets{minimum};
    qreal previous = maximum + 1;
    for (int i = 0; i < combo->count(); ++i) {
        auto action = combo->itemData(i).value<QAction *>();
        if (action && action->objectName() != QLatin1String("view_actual_size")) continue;
        const qreal value = action ? 1.0 : combo->itemData(i).toReal();
        require(value < previous, "zoom percentages remain in descending order after range regeneration");
        previous = value;
        if (value > minimum && value <= maximum) presets.append(value);
    }
    std::sort(presets.begin(), presets.end());
    require(near(scale(), minimum), "closed wheel starts at fit minimum");
    for (int round = 0; round < 2; ++round) {
        for (int i = 1; i < presets.size(); ++i) {
            wheel(120, false, round ? static_cast<QWidget *>(combo->lineEdit()) : combo);
            require(near(scale(), presets[i]), "closed hover wheel reaches every larger percentage without bouncing");
        }
        wheel(360);
        require(near(scale(), maximum), "closed hover wheel stays at maximum");
        for (int i = presets.size() - 2; i >= 0; --i) {
            wheel(-120);
            require(near(scale(), presets[i]), "closed hover wheel reaches every smaller percentage without bouncing");
        }
        wheel(-360);
        require(near(scale(), minimum), "closed hover wheel stays at minimum");
    }
    for (int i = 0; i < 3; ++i) wheel(30);
    require(near(scale(), minimum), "fractional wheel motion accumulates without premature zoom");
    wheel(30);
    require(near(scale(), presets[1]), "four fractional wheel deltas make one step");
    wheel(-30); wheel(120);
    require(near(scale(), presets[2]), "reversing wheel direction discards stale partial motion");
    wheel(-120);
    for (int i = 0; i < 4; ++i) wheel(-10, true);
    require(near(scale(), minimum), "pixel-only trackpad movement can reach fit");
    wheel(360);
    require(near(scale(), presets[qMin(3, int(presets.size()) - 1)]), "multi-notch wheel advances multiple presets");
    // Slider zoom normally lands between presets: this was the bounce path.
    QTest::keyClick(slider, Qt::Key_Right); QTest::qWait(40);
    const qreal custom = scale();
    qreal above = maximum, below = minimum;
    for (qreal value : presets) {
        if (value > custom) { above = value; break; }
    }
    for (qreal value : presets) if (value < custom) below = value;
    wheel(120);
    require(near(scale(), above), "wheel from arbitrary scale picks nearest larger preset");
    wheel(-120);
    require(near(scale(), below), "wheel returns to nearest smaller preset without oscillation");
    fit->trigger(); QTest::qWait(70);
    fprintf(stderr, "PASS: closed zoom wheel full-range round trips, editable-field hover, fractional and multi-step wheels, trackpad and custom-scale recovery\n");
}

class BackdropRenderProbe : public QGraphicsRectItem
{
public:
    using QGraphicsRectItem::QGraphicsRectItem;
    int imagePaints = 0;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        // Viewport painting uses a widget; glass capture renders the scene
        // into a QImage. Observe the extra rendering without changing pixels.
        if (painter->device()->devType() == QInternal::Image) ++imagePaints;
    }
};

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
    moveTo(viewer->rect().center()); QTest::qWait(250);
    auto probe = new BackdropRenderProbe(viewer->mapToScene(viewer->viewport()->rect()).boundingRect());
    viewer->scene()->addItem(probe);
    viewer->viewport()->update();
    QTest::qWait(100);
    require(probe->imagePaints == 0, "fully hidden toolbar controls do not recapture the image scene");
    delete probe;
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
        QTest::qWait(240);
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
        QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(55);
        auto ripple = toggle->findChild<QVariantAnimation *>("toolbarToggleRipple");
        require(ripple && ripple->state() == QAbstractAnimation::Running && ripple->duration() <= 350,
                "toggle click starts a brief accent ripple");
        require(toggle->grab().save(output + '/' + toggle->objectName() + "-ripple.png"), "capture accent ripple");
        QTest::qWait(240);
        require(shadow->property("rippleProgress").toReal() > 0.6, "line wave expands beyond button edge");
        require(shadow->grab().save(output + '/' + toggle->objectName() + "-ripple-outside.png"), "capture outer line wave");
        QTest::mouseClick(toggle, Qt::LeftButton);
        QTest::mouseClick(toggle, Qt::LeftButton);
        require(toggle->isChecked() && ripple->currentTime() < 30, "rapid clicks restart one ripple and apply toggle state immediately");
        QTest::qWait(350);
        require(ripple->state() == QAbstractAnimation::Stopped, "ripple finishes without an idle animation timer");
        moveTo(viewer->rect().center()); QTest::qWait(220);
        require(toggle->isChecked() && opacity() == 1, "expanded toolbar remains visible away from cursor");
        QTest::mouseClick(toggle, Qt::LeftButton);
        toggle->clearFocus();
        moveTo(viewer->rect().center()); QTest::qWait(500);
        require(!toggle->isChecked() && opacity() == 0, "collapsing away from pointer hides toggle again");
        toggle->setFocus(Qt::TabFocusReason); QTest::qWait(220);
        require(toggle->hasFocus(), "collapsed toggle accepts keyboard focus");
        require(opacity() == 1, "keyboard focus reveals collapsed toggle");
        NoSidebarMotionStyle noMotion;
        toggle->setStyle(&noMotion);
        QTest::keyClick(toggle, Qt::Key_Space);
        require(toggle->isChecked() && ripple->state() == QAbstractAnimation::Stopped,
                "reduced motion skips ripple while keyboard activation works");
        QTest::keyClick(toggle, Qt::Key_Space);
        toggle->setStyle(nullptr);
        QTest::qWait(250);
        toggle->clearFocus(); QTest::qWait(260);
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
    for (auto button : status->findChildren<QToolButton *>()) {
        if (button->property("floatingPillGroup").toInt() == 1)
            require(button->toolButtonStyle() == Qt::ToolButtonTextBesideIcon, "docked panel toggles retain native label style");
    }
    for (int i = 0; i < status->layout()->count(); ++i)
        if (auto spacer = status->layout()->itemAt(i)->spacerItem())
            require(spacer->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding, "native status spacers restored");
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

class ToolbarWidgetProbe : public QWidgetAction
{
public:
    explicit ToolbarWidgetProbe(QObject *parent) : QWidgetAction(parent) { setObjectName(QStringLiteral("toolbar_widget_probe")); }
protected:
    QWidget *createWidget(QWidget *parent) override
    {
        auto edit = new QLineEdit(parent); edit->setFixedSize(90, 26); return edit;
    }
};

static void verifySidebarVisibilityMotion(QWidget *window, const QString &output)
{
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto action = findAction(window, "toggle_sidebar");
    require(sidebar && sidebar->isVisible(), "sidebar starts visible for motion check");
    const int width = sidebar->width();
    action->trigger(); QTest::qWait(45);
    auto animation = window->findChild<QVariantAnimation *>("sidebarVisibilityAnimation");
    require(animation && animation->state() == QAbstractAnimation::Running && sidebar->width() > 0 && sidebar->width() < width,
            "sidebar hides with a short real slide");
    require(window->grab().save(output + "/sidebar-visibility-midway.png"), "sidebar motion capture");
    const int intermediate = sidebar->width();
    action->trigger();
    require(sidebar->width() == intermediate, "rapid sidebar reversal keeps current position");
    QTest::qWait(230);
    require(sidebar->isVisible() && sidebar->width() == width, "sidebar returns to exact preferred width");
    action->trigger(); QTest::qWait(230);
    require(!sidebar->isVisible(), "sidebar fully hides after transition");
    action->trigger(); QTest::qWait(230);
    require(sidebar->isVisible() && sidebar->width() == width, "sidebar width survives repeated toggle");
    fprintf(stderr, "PASS: sidebar visibility slide, reversal and width restoration\n");
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
    verifyMainPageSlide(window, output);
    verifySidebarVisibilityMotion(window, output);
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
    const auto actions = controls->actions();
    require(actions.indexOf(button("go_next")->defaultAction()) == actions.indexOf(button("go_previous")->defaultAction()) + 1,
            "navigation arrows share a pill without a separator");
    const int lastArrow = actions.indexOf(button("go_next")->defaultAction());
    require(actions.value(lastArrow + 1) && actions[lastArrow + 1]->isSeparator(), "navigation pill separates from other tools");
    auto surface = flyout->parentWidget();
    QImage grouped(surface->size(), QImage::Format_ARGB32_Premultiplied); grouped.fill(Qt::transparent);
    surface->render(&grouped, QPoint(), QRegion(), QWidget::DrawChildren);
    const QRect separator = controls->actionGeometry(actions[lastArrow + 1]);
    const QPoint gap = controls->mapTo(surface, separator.center());
    require(qAlpha(grouped.pixel(gap)) == 0, "floating pills leave transparent image space between groups");
    require(!island->mask().contains(surface->mapTo(island, gap)), "gaps between pills pass image input through");
    int clippedBorderPixels = 0;
    for (int y = 0; y < qMin(44, grouped.height()); ++y) for (int x = 0; x < grouped.width(); ++x) {
        const QPoint point = surface->mapTo(island, QPoint(x, y));
        if (island->rect().contains(point) && qAlpha(grouped.pixel(x, y)) > 8 && !island->mask().contains(point))
            ++clippedBorderPixels;
    }
    require(clippedBorderPixels == 0, "input mask preserves antialiased pill edges instead of cutting them into steps");
    require(grouped.save(output + "/floating-toolbar-pills.png"), "capture grouped floating surfaces");
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
    QTest::qWait(240);
    require(controls->actions().contains(&custom), "floating copy follows toolbar customization");
    QList<QAction *> mirrored;
    for (auto action : controls->actions()) if (!action->isSeparator()) mirrored.append(action);
    require(mirrored.indexOf(&custom) + 1 == mirrored.indexOf(button("go_next")->defaultAction()),
            "grouping preserves the user's custom action order");
    custom.setVisible(false); QTest::qWait(50);
    require(!controls->widgetForAction(&custom)->isVisible(), "floating pills track custom action visibility");
    custom.setVisible(true); QTest::qWait(50);
    custom.setEnabled(false);
    require(!controls->widgetForAction(&custom)->isEnabled(), "floating copy tracks action enabled state");
    docked->removeAction(&custom); QTest::qWait(240);
    require(!controls->actions().contains(&custom), "floating copy removes customized action");
    ToolbarWidgetProbe widgetProbe(window);
    docked->insertAction(button("hamburger_menu")->defaultAction(), &widgetProbe); QTest::qWait(240);
    auto editor = qobject_cast<QLineEdit *>(controls->widgetForAction(&widgetProbe));
    require(editor && editor != docked->widgetForAction(&widgetProbe), "custom widget actions create real controls in both toolbars");
    QTest::keyClicks(editor, "ready");
    require(editor->text() == "ready", "custom floating widget remains interactive");
    docked->removeAction(&widgetProbe); QTest::qWait(150);
    require(!controls->actions().contains(&widgetProbe), "custom widget removal updates floating pills");
    const QSize wideSize = window->size();
    window->resize(640, wideSize.height()); QTest::qWait(250);
    require(flyout->horizontalScrollBar()->maximum() > 0, "customizable floating toolbar scrolls in a narrow pane");
    require(flyout->viewport()->height() >= controls->height(), "narrow floating toolbar reserves space for its scrollbar");
    flyout->horizontalScrollBar()->setValue(flyout->horizontalScrollBar()->maximum()); QTest::qWait(50);
    const QPoint lastCenter = button("hamburger_menu")->mapTo(island, button("hamburger_menu")->rect().center());
    require(island->rect().contains(lastCenter) && island->mask().contains(lastCenter), "last floating tool remains reachable after scrolling");
    require(window->grab().save(output + "/floating-pills-narrow.png"), "capture narrow grouped toolbar");
    window->resize(wideSize); QTest::qWait(250);
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
    verifySidebarSlide(tabs, output);
    require(count->text() == "6", "recent count reflects six fixtures");
    require(tabs->tabText(0) == "Recent" && tabs->tabText(1) == "Information" && tabs->tabText(2) == "Tools", "footer labels are discoverable");
    auto sidebarIsland = window->findChild<QWidget *>("sidebarIsland");
    auto splitter = qobject_cast<QSplitter *>(sidebarIsland->parentWidget());
    require(splitter, "sidebar splitter exists");
    splitter->setSizes({340, window->width() - 340}); QTest::qWait(240);
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
    splitter->setSizes({190, window->width() - 190}); QTest::qWait(240);
    require(bar->property("tabButtonStyle").toInt() == 0, "narrow sidebar switches to icons only");
    require(bar->tabRect(2).right() < bar->width(), "all three narrow tabs remain visible");
    require(tabs->tabToolTip(0) == "Recent" && tabs->tabToolTip(2) == "Tools", "icon-only tabs retain descriptive tooltips");
    capture("welcome-footer-narrow");
    QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->tabRect(2).center());
    require(tabs->currentIndex() == 2, "narrow Tools tab remains clickable");
    splitter->setSizes(savedSizes); tabs->setCurrentIndex(0); QTest::qWait(240);
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
    picker->reject(); QTest::qWait(240);
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
    QTest::keyClick(rotateButton, Qt::Key_Space); QTest::qWait(240);
    require(rotated, "wrapped tool row triggers the native action by keyboard");
    QObject::disconnect(rotationConnection);
    findAction(window, "edit_undo")->trigger(); QTest::qWait(240);
    // Action rows are rebuilt after the document changes.
    rotateAction->setText(originalToolText);
    const QRect footerRect = bar->geometry();
    require(toolsScroll->verticalScrollBar()->maximum() > 0, "long Tools page scrolls vertically");
    toolsScroll->verticalScrollBar()->setValue(toolsScroll->verticalScrollBar()->maximum()); QTest::qWait(100);
    require(bar->geometry() == footerRect, "Tools scrolling keeps footer anchored");
    capture("context-tools-narrow-bottom");
    tabs->setCurrentIndex(1); QTest::qWait(240);
    require(sidebarIsland->width() <= 205, "Information respects a narrow sidebar");
    require(infoScroll->horizontalScrollBar()->maximum() == 0, "Information needs no horizontal scrolling");
    auto values = tabs->widget(1)->findChildren<QTextEdit *>("metadataValue");
    require(!values.isEmpty(), "Information shows image metadata");
    auto value = values.last(); const QString originalValue = value->toPlainText();
    value->setPlainText(QString(90, 'W') + ".png");
    value->parentWidget()->updateGeometry();
    splitter->setSizes({192, 908}); QTest::qWait(240);
    require(value->heightForWidth(value->width()) > value->fontMetrics().height() * 2, "long metadata values wrap at narrow widths");
    require(value->height() >= value->heightForWidth(value->width()), "metadata value height contains its wrapped text");
    value->selectAll(); value->copy();
    require(QApplication::clipboard()->text() == value->toPlainText(), "wrapped metadata copies its exact original text");
    value->moveCursor(QTextCursor::Start);
    capture("context-information-narrow");
    value->setPlainText(originalValue); value->parentWidget()->updateGeometry();
    splitter->setSizes({190, 910}); QTest::qWait(240);
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
    splitter->setSizes({340, 760}); infoScroll->verticalScrollBar()->setValue(0); QTest::qWait(240);
    if (!tags.isEmpty()) require(tags.first()->width() > narrowTagWidth, "wider Information panel reveals more of long tags");
    capture("context-information-wide");
    auto details = window->findChild<QLabel *>("showMoreImageDetails");
    require(details && details->focusPolicy() == Qt::StrongFocus, "full image details link is keyboard accessible");
    QMetaObject::invokeMethod(details, "linkActivated", Q_ARG(QString, "#")); QTest::qWait(240);
    QDialog *detailsDialog = nullptr;
    for (auto dialog : window->findChildren<QDialog *>()) if (dialog->isVisible()) detailsDialog = dialog;
    require(detailsDialog, "full metadata dialog remains available"); detailsDialog->close();
    window->resize(contextSize); splitter->setSizes(savedSizes); QTest::qWait(240);
    fprintf(stderr, "PASS: centered welcome actions, wrapped metadata and tools, narrow panels, native tool keyboard activation and details dialog\n");

    findAction(window, "go_start_page")->trigger(); QTest::qWait(250);
    tabs->setCurrentIndex(1); require(infoEmpty->isVisible(), "welcome never shows stale image information");
    tabs->setCurrentIndex(0);
    bool menuOpened = false;
    QTimer::singleShot(120, window, [&] {
        menuOpened = more->menu()->isVisible(); capture("welcome-recents-menu");
        list->trigger(); more->menu()->close();
    });
    QTest::mouseClick(more, Qt::LeftButton); QTest::qWait(240);
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
    window->resize(originalSize); QTest::qWait(240);
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

static void verifyThumbnailScrolling(QWidget *window, const QString &output)
{
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    const auto gallery = QUrl::fromLocalFile(fixtures + "/Gallery");
    require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, gallery)), "open large gallery");
    auto view = window->findChild<QListView *>("mThumbnailView");
    require(view, "large-folder browser view");
    for (int i = 0; i < 100 && view->model()->rowCount() < 2049; ++i) QTest::qWait(50);
    require(view->model()->rowCount() == 2049, "all 2048 images and folder are listed");
    QTest::qWait(700);
    auto wheel = [](QListView *list, QPoint pixels, QPoint angle) {
        auto viewport = list->viewport(); const QPoint point = viewport->rect().center();
        QWheelEvent event(point, viewport->mapToGlobal(point), pixels, angle,
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(viewport, &event);
    };
    for (bool full : {false, true}) {
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(400); }
        const auto empty = QUrl::fromLocalFile(fixtures + "/Gallery/Empty folder");
        require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, empty)), "leave large folder"); QTest::qWait(240);
        QElapsedTimer navigationPulse; navigationPulse.start(); qint64 navigationGap = 0;
        QTimer pulse; pulse.setInterval(8);
        QObject::connect(&pulse, &QTimer::timeout, window, [&] { navigationGap = qMax(navigationGap, navigationPulse.restart()); });
        pulse.start();
        QElapsedTimer requestTime; requestTime.start();
        require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, gallery)), "return to large folder with slide");
        const qint64 navigationRequest = requestTime.elapsed();
        QTest::qWait(35);
        auto folderAnimation = window->findChild<QVariantAnimation *>("folderSlideAnimation");
        auto folderEffect = view->viewport()->graphicsEffect();
        require(folderAnimation && folderAnimation->state() == QAbstractAnimation::Running && folderEffect->isEnabled(),
                "large-folder navigation retains its visible slide");
        require(folderEffect->boundingRect().size().toSize() == view->viewport()->size(), "animation source is bounded to the visible viewport, not the folder size");
        QTest::qWait(220); pulse.stop();
        require(navigationRequest < 80 && navigationGap < 100, "large-folder slide keeps navigation and event loop responsive");
        require(!folderEffect->isEnabled(), "large-folder graphics effect is disabled after transition");
        fprintf(stderr, "PERF: %s 2048-image folder navigation request %lld ms; slide heartbeat max %lld ms\n",
                full ? "fullscreen" : "normal", static_cast<long long>(navigationRequest), static_cast<long long>(navigationGap));
        for (int i = 0; i < 100 && view->model()->rowCount() < 2049; ++i) QTest::qWait(50);
        require(view->model()->rowCount() == 2049, "large-folder transition does not delay or drop listing results");
        QTest::qWait(300);
        auto bar = view->verticalScrollBar();
        require(bar->maximum() > 10 * bar->pageStep(), "large folder has many scrollable pages");
        qint64 worstRefresh = 0;
        for (int fraction : {0, 1, 2, 3, 4, 2, 0}) {
            bar->setValue(bar->maximum() * fraction / 4);
            QTest::qWait(100);
            QElapsedTimer time; time.start();
            require(QMetaObject::invokeMethod(view, "generateThumbnailsForItems", Qt::DirectConnection), "refresh visible thumbnails");
            worstRefresh = qMax(worstRefresh, time.nsecsElapsed());
        }
        bar->setValue(0);
        QElapsedTimer heartbeat; heartbeat.start(); qint64 worstGap = 0;
        QTimer timer; timer.setInterval(8);
        QObject::connect(&timer, &QTimer::timeout, window, [&] { worstGap = qMax(worstGap, heartbeat.restart()); });
        timer.start();
        for (int i = 0; i < 100; ++i) {
            const int before = bar->value();
            wheel(view, QPoint(0, -18), QPoint()); QTest::qWait(10);
            require(bar->value() >= before, "pixel scrolling never snaps backward");
        }
        timer.stop();
        require(bar->value() > 0, "touchpad scroll moves through thumbnails");
        bar->setValue(bar->maximum() - bar->pageStep());
        for (int i = 0; i < 50; ++i) wheel(view, QPoint(), QPoint(0, -120));
        require(bar->value() == bar->maximum(), "wheel reaches last thumbnails");
        bar->setValue(bar->pageStep());
        for (int i = 0; i < 50; ++i) wheel(view, QPoint(), QPoint(0, 120));
        require(bar->value() == 0, "wheel returns to first thumbnails");
        fprintf(stderr, "PERF: %s 2048-image gallery refresh max %.3f ms; scrolling heartbeat max %lld ms\n",
                full ? "fullscreen" : "normal", worstRefresh / 1000000.0, static_cast<long long>(worstGap));
        auto toggleSidebar = findAction(window, "toggle_sidebar");
        auto sidebar = window->findChild<QWidget *>("sidebarIsland");
        const bool wasVisible = sidebar->isVisible();
        worstGap = 0; heartbeat.restart(); timer.start();
        for (int cycle = 0; cycle < 4; ++cycle) { toggleSidebar->trigger(); QTest::qWait(230); }
        timer.stop();
        require(sidebar->isVisible() == wasVisible, "large-folder sidebar toggles restore visibility");
        require(worstGap < 100, "sidebar animation keeps large-folder event loop responsive");
        fprintf(stderr, "PERF: %s 2048-image sidebar slide heartbeat max %lld ms\n",
                full ? "fullscreen" : "normal", static_cast<long long>(worstGap));
        require(window->grab().save(output + (full ? "/large-fullscreen.png" : "/large-normal.png")), "large-gallery capture");
    }
    const auto file = QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png");
    require(QMetaObject::invokeMethod(window, "openImages", Q_ARG(QList<QUrl>, QList<QUrl>{file})), "open image from large gallery");
    QTest::qWait(600);
    auto thumbs = window->findChild<QListView *>("fullscreenThumbnails");
    auto pin = window->findChild<QToolButton *>("fullscreen_keep_controls_visible");
    require(thumbs && pin, "fullscreen image strip");
    if (!pin->isChecked()) pin->click(); QTest::qWait(150);
    require(thumbs->height() >= 104 && thumbs->viewport()->height() >= 80, "larger previews leave room above the scrollbar");
    auto bar = thumbs->horizontalScrollBar();
    thumbs->setCurrentIndex(thumbs->model()->index(100, 0));
    bar->setValue(200);
    wheel(thumbs, QPoint(-24, 0), QPoint());
    const int moved = bar->value();
    require(moved > 200, "horizontal touchpad scrolling moves image strip");
    QTest::qWait(300);
    require(bar->value() == moved, "image-strip scrolling does not snap back");
    require(window->grab().save(output + "/large-image-strip.png"), "larger image previews capture");
    fprintf(stderr, "PASS: large-folder normal/fullscreen pixel and wheel scrolling, endpoints, and larger fullscreen image strip\n");
}

static void verifyFolderSlide(QWidget *window, const QUrl &gallery, const QString &output)
{
    auto page = window->findChild<QWidget *>("BrowseMainPage");
    auto view = page->findChild<QListView *>("mThumbnailView");
    auto animation = page->findChild<QVariantAnimation *>("folderSlideAnimation");
    auto effect = view->viewport()->graphicsEffect();
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto splitter = qobject_cast<QSplitter *>(sidebar->parentWidget());
    require(animation && effect && animation->duration() == 200 && sidebar->isVisible(), "short gallery slide available with sidebar open");
    const auto child = QUrl::fromLocalFile(gallery.toLocalFile() + "/Empty folder");
    auto navigate = [&](const QUrl &url) { require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, url)), "navigate folder for slide checks"); };
    const QRect geometry = view->viewport()->geometry();
    navigate(child); QTest::qWait(35);
    require(animation->state() == QAbstractAnimation::Running && effect->isEnabled()
            && view->property("folderSlideOffset").toReal() > 0 && view->property("folderSlideOffset").toReal() < 28,
            "entering a child folder visibly slides its content from the right");
    require(view->viewport()->geometry() == geometry, "folder slide leaves native viewport geometry and hit targets unchanged");
    ViewPaintCounter paints; view->viewport()->installEventFilter(&paints);
    int frames = 0;
    auto connection = QObject::connect(animation, &QVariantAnimation::valueChanged, window, [&] { ++frames; });
    QTest::qWait(55);
    QObject::disconnect(connection); view->viewport()->removeEventFilter(&paints);
    require(frames >= 2 && paints.paints <= 2, "settled folder frames reuse the viewport cache without repainting items each frame");
    QTest::qWait(160);
    require(!effect->isEnabled() && animation->state() == QAbstractAnimation::Stopped, "folder slide leaves no idle graphics-effect overhead");
    navigate(gallery); QTest::qWait(35);
    require(view->property("folderSlideOffset").toReal() < 0 && animation->state() == QAbstractAnimation::Running,
            "returning to parent folder slides in the opposite direction");
    require(window->grab().save(output + "/folder-slide-back.png"), "capture folder slide");
    const QPoint point = view->viewport()->rect().center();
    QWheelEvent wheel(point, view->viewport()->mapToGlobal(point), QPoint(), QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(view->viewport(), &wheel);
    require(!effect->isEnabled() && view->property("folderSlideOffset").toReal() == 0, "scrolling dismisses the slide immediately");
    navigate(child); QTest::qWait(25); navigate(gallery); QTest::qWait(25); navigate(child); QTest::qWait(25); navigate(gallery);
    QTest::qWait(240);
    require(animation->state() == QAbstractAnimation::Stopped && !effect->isEnabled()
            && page->findChild<QLabel *>("browseFolderTitle")->text() == "Gallery", "rapid folder changes show the latest folder without queued motion");
    navigate(child); QTest::qWait(30);
    const auto sizes = splitter->sizes(); splitter->setSizes({sizes[0] + 40, sizes[1] - 40}); QTest::qWait(50);
    require(!effect->isEnabled(), "sidebar resize ends the slide without clipping or delayed interaction");
    splitter->setSizes(sizes); QTest::qWait(100);
    NoSidebarMotionStyle noMotion; view->setStyle(&noMotion);
    navigate(gallery); QTest::qWait(40);
    require(!effect->isEnabled() && animation->state() == QAbstractAnimation::Stopped, "reduced motion switches folders directly");
    view->setStyle(nullptr); QTest::qWait(250);
    auto breadcrumb = page->findChild<QWidget *>("browseBreadcrumb");
    require(breadcrumb, "breadcrumb field exists");
    QImage field(breadcrumb->size(), QImage::Format_ARGB32_Premultiplied); field.fill(Qt::transparent);
    breadcrumb->render(&field, QPoint(), QRegion(), QWidget::RenderFlags());
    require(qAlpha(field.pixel(2, 2)) == 0 && qAlpha(field.pixel(4, 4)) < 50 && qAlpha(field.pixel(1, field.height() / 2)) > 200
            && qAlpha(field.pixel(field.width() / 2, 2)) > 200, "actual breadcrumb text background has capsule ends, not an inset rectangular frame");
    require(field.save(output + "/breadcrumb-field-pill.png"), "capture breadcrumb field background");
    fprintf(stderr, "PASS: folder slide direction/cache, immediate scrolling, rapid changes, sidebar resize, reduced motion and breadcrumb field capsule\n");
}

static void verifyBreadcrumbMotion(QWidget *window, const QUrl &gallery, const QString &output)
{
    auto location = window->findChild<QWidget *>("mUrlNavigatorContainer");
    auto animation = location->findChild<QVariantAnimation *>("breadcrumbWidthAnimation");
    auto chrome = window->findChild<QWidget *>("fullscreenBrowseChrome");
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto splitter = qobject_cast<QSplitter *>(sidebar->parentWidget());
    require(animation && animation->duration() == 200 && sidebar->isVisible(), "quick breadcrumb animation available with sidebar open");
    auto navigate = [&](const QUrl &url) {
        require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, url)), "navigate during breadcrumb animation");
    };
    const QUrl root = QUrl::fromLocalFile("/");
    const int wide = location->width();
    const int left = location->x();
    const int rowHeight = chrome->height();
    navigate(root); QTest::qWait(45);
    const int small = location->property("pathTargetWidth").toInt();
    require(animation->state() == QAbstractAnimation::Running && location->width() < wide && location->width() > small,
            "breadcrumb visibly shrinks through intermediate widths");
    int previous = location->width();
    for (int i = 0; i < 10; ++i) {
        QTest::qWait(20);
        require(location->width() <= previous && location->x() == left && chrome->height() == rowHeight,
                "shrinking is monotonic with fixed left edge and row height");
        previous = location->width();
    }
    require(animation->state() == QAbstractAnimation::Stopped && location->width() == small, "breadcrumb finishes shrinking promptly");
    navigate(gallery); QTest::qWait(45);
    require(animation->state() == QAbstractAnimation::Running && location->width() > small && location->width() < wide,
            "breadcrumb visibly expands through intermediate widths");
    require(window->grab().save(output + "/breadcrumb-expanding.png"), "capture breadcrumb width transition");
    const int intermediate = location->width();
    navigate(root);
    require(qAbs(location->width() - intermediate) <= 2, "rapid navigation retargets without a width jump");
    QTest::qWait(35);
    require(location->width() < intermediate && location->width() > small, "width animation reverses smoothly");
    navigate(gallery); QTest::qWait(240);
    require(location->width() == wide && location->x() == left && animation->state() == QAbstractAnimation::Stopped,
            "rapid folder changes settle on the latest path without queued animations");
    navigate(root); QTest::qWait(240);
    navigate(gallery); QTest::qWait(35);
    const auto sizes = splitter->sizes();
    splitter->setSizes({sizes[0] + 80, sizes[1] - 80}); QTest::qWait(80);
    require(animation->state() == QAbstractAnimation::Stopped && chrome->rect().contains(QRect(location->mapTo(chrome, QPoint()), location->size())),
            "sidebar resize immediately fits the breadcrumb without clipping or waiting for animation");
    splitter->setSizes(sizes); QTest::qWait(100);
    NoSidebarMotionStyle noMotion;
    location->setStyle(&noMotion);
    navigate(root); QTest::qWait(25);
    require(animation->state() == QAbstractAnimation::Stopped && location->width() == location->property("pathTargetWidth").toInt(),
            "disabled desktop animation switches breadcrumb width immediately");
    navigate(gallery); QTest::qWait(250);
    location->setStyle(nullptr); QTest::qWait(100);
    fprintf(stderr, "PASS: breadcrumb expansion/shrink, left anchoring, rapid reversal, sidebar resizing and reduced motion\n");
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
    verifyBreadcrumbMotion(window, gallery, output);
    verifyFolderSlide(window, gallery, output);
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
    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() - 20)); QTest::qWait(240);
    require(!sort->isDown() && !sort->underMouse() && !sort->hasFocus(), "closed Browse menu does not retain pressed, hover or click-focus highlight");
    auto sidebarToggle = window->findChild<QToolButton *>("mToggleSideBarButton");
    require(sidebarToggle && sidebarToggle->width() >= 36 && sidebarToggle->height() >= 32 && sidebarToggle->iconSize().width() >= 22,
            "Browse sidebar toggle has a readable icon and larger target");
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
    filters->menu()->actions().last()->trigger(); QTest::qWait(240);
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
    findAction(window, "leave_fullscreen")->trigger(); QTest::qWait(240);
    require(!full->isChecked() && mainToolbar->isVisible() && page->layout()->contentsMargins().left() > 0,
            "leaving fullscreen restores browser presentation");
    auto showStatus = findAction(window, "options_show_statusbar");
    if (showStatus->isChecked()) showStatus->trigger(); QTest::qWait(100);
    require(!sliderContainer->isVisible() && filters->isVisible(), "status preference hides footer without losing filters");
    if (!showStatus->isChecked()) showStatus->trigger();
    view->setCurrentIndex(view->model()->index(1, 0));
    QTest::keyClick(view, Qt::Key_Return); QTest::qWait(400);
    require(!page->isVisible(), "browser activation opens native image viewer");
    findAction(window, "browse")->trigger(); QTest::qWait(240);
    window->resize(720, 500); splitter->setSizes({190, 530}); QTest::qWait(150);
    fprintf(stderr, "PASS: browser header, sorting, hidden files, details, selection, adaptive filters, empty states, narrow layout, thumbnail fitting, fullscreen and activation\n");
}

static void verifyFullscreenBrowser(QWidget *window, const QString &output)
{
    // The isolated session cannot launch the desktop tags worker.
    if (auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
        for (auto label : dialog->findChildren<QLabel *>()) {
            if (label->text().contains("Unable to create KIO worker") && label->text().contains("protocol 'tags'")) {
                dialog->reject(); break;
            }
        }
    }
    QApplication::setActiveWindow(window);
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    const auto galleryUrl = QUrl::fromLocalFile(fixtures + "/Gallery");
    require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, galleryUrl)), "open fullscreen browser fixture");
    QTest::qWait(500);
    auto full = findAction(window, "fullscreen"); full->trigger(); QTest::qWait(300);
    auto page = window->findChild<QWidget *>("BrowseMainPage");
    auto chrome = page->findChild<QWidget *>("fullscreenBrowseChrome");
    auto gallery = page->findChild<QWidget *>("fullscreenBrowseGallery");
    auto navigation = page->findChild<QWidget *>("fullscreenBrowseNavigation");
    auto location = page->findChild<QWidget *>("mUrlNavigatorContainer");
    auto view = page->findChild<QListView *>("mThumbnailView");
    auto title = page->findChild<QLabel *>("browseFolderTitle");
    auto sort = page->findChild<QToolButton *>("browseSortButton");
    auto details = page->findChild<QToolButton *>("browseDetailsButton");
    auto filters = page->findChild<QToolButton *>("mAddFilterButton");
    auto filterScroll = page->findChild<QScrollArea *>("browseFilterScroll");
    auto slider = page->findChild<QWidget *>("mThumbnailSlider")->findChild<QSlider *>();
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto split = qobject_cast<QSplitter *>(sidebar->parentWidget());
    require(chrome && gallery && navigation && location && view && title && sort && details && filters && slider, "fullscreen browser surfaces and controls exist");
    auto button = [&](const char *name) {
        auto b = page->findChild<QToolButton *>(QStringLiteral("fullscreenBrowser_") + QString::fromLatin1(name));
        require(b && b->isVisible() && !b->icon().isNull() && !b->toolTip().isEmpty(), "fullscreen browser action is visible and described"); return b;
    };
    auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "fullscreen browser screenshot"); };
    if (!sidebar->isVisible()) button("toggle_sidebar")->click();
    window->resize(1280, 800); split->setSizes({240, 1040}); slider->setValue(145); QTest::qWait(800);
    require(view->model()->rowCount() == 13 && title->text() == "Gallery", "fullscreen browser displays folder contents");
    require(qAbs(navigation->geometry().center().y() - location->geometry().center().y()) <= 8
        && location->x() > navigation->x(), "wide navigation shares one row with location");
    require(location->height() == 60 && navigation->height() == location->height(), "breadcrumb and navigation use equal 44px surfaces with shadow space");
    auto breadcrumb = location->findChild<QWidget *>("browseBreadcrumb");
    require(breadcrumb && qAbs(QRectF(breadcrumb->geometry()).center().y() - QRectF(location->rect()).adjusted(6, 6, -6, -10).center().y()) <= 0.5,
            "breadcrumb field is vertically centered in the visible pill");
    require(location->x() - navigation->geometry().right() < 12, "breadcrumb island sits directly beside the navigation controls");
    verifyBreadcrumbMotion(window, galleryUrl, output);
    auto header = page->findChild<QWidget *>("browseHeader");
    require(navigation->mapTo(chrome, QPoint()).x() == header->mapTo(chrome, QPoint()).x(), "navigation islands align with the header's left edge");
    require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, QUrl::fromLocalFile("/"))), "open shallow path");
    QTest::qWait(250);
    const int shortWidth = location->width();
    require(shortWidth < chrome->width() / 2, "shallow breadcrumb island leaves open space to its right");
    require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, galleryUrl)), "restore deep path");
    QTest::qWait(450);
    require(location->width() > shortWidth && location->geometry().right() < chrome->width(), "breadcrumb island grows with path depth within the available width");
    const int availableLocationWidth = chrome->width() - location->x() - chrome->layout()->contentsMargins().right();
    require(location->width() >= qMin(location->maximumWidth(), availableLocationWidth) - 2,
            "deep paths use their natural width before collapsing ancestor breadcrumbs");
    capture("fullscreen-browser-wide");
    window->resize(640, 480); split->setSizes({190, 450}); QTest::qWait(300);
    require(window->width() <= 640 && page->width() < 760, "fullscreen browser fits small window with sidebar");
    require(location->y() > navigation->y(), "location gains a full row beside an open sidebar");
    require(location->mapTo(chrome, QPoint()).x() == header->mapTo(chrome, QPoint()).x(), "stacked breadcrumb island aligns with the header");
    auto visible = [&](QWidget *control, QWidget *surface) {
        require(control->isVisible() && control->visibleRegion().contains(control->rect().center())
            && surface->rect().contains(QRect(control->mapTo(surface, QPoint()), control->size())), "compact fullscreen control is visible and reachable");
    };
    for (const char *name : {"toggle_sidebar", "view", "browse_history_back", "browse_history_forward", "go_up", "leave_fullscreen"}) visible(button(name), chrome);
    for (auto control : {static_cast<QWidget *>(sort), static_cast<QWidget *>(details), static_cast<QWidget *>(filters), location}) visible(control, chrome);
    visible(slider->parentWidget(), gallery);
    require(view->viewport()->height() >= 150 && view->horizontalScrollBar()->maximum() == 0, "compact gallery retains browsing space");
    capture("fullscreen-browser-sidebar-narrow");
    verifyFolderSlide(window, galleryUrl, output);

    auto openMenu = [&](QToolButton *control) {
        bool opened = false;
        QTimer::singleShot(80, window, [&] { opened = control->menu()->isVisible(); control->menu()->close(); });
        QTest::mouseClick(control, Qt::LeftButton); require(opened, "compact browser menu opens");
    };
    openMenu(sort); openMenu(details); openMenu(filters);
    findAction(window, "sort_desc")->setChecked(true); QTest::qWait(150);
    require(view->model()->index(1, 0).data().toString() == "scene-11.png", "fullscreen sorting remains native");
    findAction(window, "sort_desc")->setChecked(false);
    filters->menu()->actions().first()->trigger(); QTest::qWait(150);
    auto name = filterScroll->findChild<QLineEdit *>(); require(name, "fullscreen name filter available");
    name->setText("scene-02"); QTest::qWait(500);
    require(view->model()->rowCount() == 1 && filterScroll->horizontalScrollBar()->maximum() == 0, "fullscreen filter fits and filters images");
    require(view->viewport()->height() >= 100, "fullscreen filters preserve image browsing space");
    capture("fullscreen-browser-filtered");
    filterScroll->findChild<QToolButton *>("removeBrowseFilter")->click(); QTest::qWait(300);
    view->setCurrentIndex(view->model()->index(1, 0)); view->setFocus();
    QTest::keyClick(view, Qt::Key_Right); QTest::qWait(80);
    require(view->currentIndex().row() > 1, "fullscreen thumbnails support keyboard navigation");
    view->selectionModel()->select(view->model()->index(4, 0), QItemSelectionModel::Select); QTest::qWait(80);
    require(page->findChild<QLabel *>("mDocumentCountLabel")->text().contains("Selected"), "fullscreen selection count updates");
    slider->setValue(400); QTest::qWait(150);
    require(view->gridSize().width() <= view->viewport()->width() && view->gridSize().height() <= view->viewport()->height(), "fullscreen thumbnail size fits beside sidebar");
    slider->setValue(145);

    require(QMetaObject::invokeMethod(window, "openDirUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/Empty folder"))), "open child folder");
    QTest::qWait(250);
    require(button("browse_history_back")->isEnabled(), "folder back action enabled");
    button("browse_history_back")->click(); QTest::qWait(250);
    require(title->toolTip().endsWith("/Gallery"), "folder Back returns to gallery");
    button("browse_history_forward")->click(); QTest::qWait(250);
    require(title->toolTip().endsWith("/Empty folder"), "folder Forward restores child folder");
    button("go_up")->click(); QTest::qWait(250);
    require(title->toolTip().endsWith("/Gallery"), "Up navigates to parent folder");
    findAction(window, "edit_location")->trigger(); QTest::qWait(100);
    bool editable = false;
    for (auto edit : location->findChildren<QLineEdit *>()) editable |= edit->isVisible();
    require(editable, "F6 location editing stays available");
    capture("fullscreen-browser-location-edit");
    auto status = findAction(window, "options_show_statusbar");
    if (status->isChecked()) status->trigger();
    require(!slider->isVisible() && button("toggle_sidebar")->isVisible(), "sidebar toggle remains available when status bar hidden");
    button("toggle_sidebar")->click(); QTest::qWait(230); require(!sidebar->isVisible(), "fullscreen sidebar toggle closes sidebar");
    button("toggle_sidebar")->click(); QTest::qWait(230); require(sidebar->isVisible(), "fullscreen sidebar toggle restores sidebar");
    if (!status->isChecked()) status->trigger();
    view->setCurrentIndex(view->model()->index(1, 0));
    button("view")->click(); QTest::qWait(300);
    require(!page->isVisible() && window->findChild<QWidget *>("fullScreenBar")->isVisible(), "View opens matching fullscreen image controls");
    findAction(window, "browse")->trigger(); QTest::qWait(250);
    require(page->isVisible() && !window->findChild<QWidget *>("fullScreenBar")->isVisible(), "Browse restores fullscreen gallery without image overlay");
    button("leave_fullscreen")->click(); QTest::qWait(250);
    require(!full->isChecked() && !navigation->isVisible() && window->findChild<QToolBar *>("mainToolBar")->isVisible(), "exit restores standard browser controls");
    for (int i = 0; i < 2; ++i) {
        full->trigger(); QTest::qWait(100);
        view->setFocus(); QTest::keyClick(view, Qt::Key_Escape); QTest::qWait(150);
        require(!full->isChecked() && !navigation->isVisible(), "Escape exits fullscreen browser repeatedly");
    }
    capture("fullscreen-browser-restored");
    fprintf(stderr, "PASS: fullscreen browser floating surfaces, sidebar resizing, menus, filters, folder history, location editing, thumbnail zoom, view transitions and exit restoration\n");
}

static void verifyProperties(QWidget *window, const QString &output)
{
    auto action = findAction(window, "file_show_properties");
    require(action->isEnabled(), "native Properties action remains available");
    action->trigger(); QTest::qWait(45);
    auto dialog = window->findChild<QDialog *>("gwenviewPropertiesDialog");
    require(dialog && dialog->isVisible() && dialog->isModal(), "polished native Properties dialog opens");
    auto entrance = dialog->findChild<QWidget *>("dialogEntrance");
    auto animation = dialog->findChild<QVariantAnimation *>("dialogEntranceAnimation");
    require(entrance && animation && animation->duration() == 200 && dialog->graphicsEffect(), "Properties opens with a short visible entrance");
    require(entrance->testAttribute(Qt::WA_TransparentForMouseEvents), "Properties animation does not intercept input");
    require(dialog->grab().save(output + "/properties-opening.png"), "Properties entrance capture");
    QTest::qWait(240);
    require(!entrance->isVisible() && animation->state() == QAbstractAnimation::Stopped, "Properties entrance ends and releases controls");
    auto tabs = dialog->findChild<QTabWidget *>();
    require(tabs && tabs->count() >= 2, "native file property pages remain available");
    for (int i = 0; i < tabs->count(); ++i) { tabs->setCurrentIndex(i); QTest::qWait(25); }
    tabs->setCurrentIndex(0); QTest::qWait(240);
    auto stack = tabs->findChild<QStackedWidget *>(QString(), Qt::FindDirectChildrenOnly);
    require(stack, "Properties tab stack exists");
    const auto backgroundPoint = dialog->devicePixelRatioF() * stack->mapTo(dialog, QPoint(2, 2));
    const auto backgroundBefore = dialog->grab().toImage().pixelColor(backgroundPoint);
    tabs->setCurrentIndex(1); QTest::qWait(35);
    require(dialog->grab().toImage().pixelColor(backgroundPoint) == backgroundBefore, "page transition preserves the actual background material");
    auto pageTransition = tabs->findChild<QWidget *>("dialogPageTransition");
    require(pageTransition && !pageTransition->isVisible() && tabs->currentWidget()->graphicsEffect(), "page changes fade live content without an opaque overlay");
    require(dialog->grab().save(output + "/properties-switching.png"), "in-flight Properties capture");
    QTest::qWait(240);
    require(!tabs->currentWidget()->graphicsEffect(), "page effect is released after transition");
    tabs->setCurrentIndex(2); QTest::qWait(25); tabs->setCurrentIndex(0); QTest::qWait(240);
    for (int i = 0; i < tabs->count(); ++i) require(!tabs->widget(i)->graphicsEffect(), "rapid page switching releases all effects");
    require(dialog->grab().save(output + "/properties-polished.png"), "Properties capture");
    const QSize fullSize = dialog->size();
    dialog->resize(320, 540); QTest::qWait(240);
    auto bar = tabs->tabBar();
    auto forward = bar->findChild<QToolButton *>("ScrollRightButton");
    auto backward = bar->findChild<QToolButton *>("ScrollLeftButton");
    require(forward && backward && forward->isVisible() && backward->isVisible(), "narrow Properties exposes both overflow arrows");
    require(dialog->grab().save(output + "/properties-narrow.png"), "narrow Properties capture");
    const int firstTabLeft = bar->tabRect(0).left();
    QTest::mouseClick(forward, Qt::LeftButton); QTest::qWait(240);
    require(bar->tabRect(0).left() < firstTabLeft && backward->isEnabled(), "overflow pill scrolls to hidden views");
    QTest::mouseClick(backward, Qt::LeftButton); QTest::qWait(240);
    require(bar->tabRect(0).left() == firstTabLeft, "overflow pill scrolls back to the first view");
    dialog->setLayoutDirection(Qt::RightToLeft); QTest::qWait(240);
    require(dialog->grab().save(output + "/properties-narrow-rtl.png"), "right-to-left overflow capture");
    dialog->setLayoutDirection(Qt::LeftToRight);
    dialog->resize(fullSize); QTest::qWait(240);
    auto filenameEdit = [](QDialog *dialog) {
        for (auto edit : dialog->findChildren<QLineEdit *>())
            if (edit->isVisible() && edit->isEnabled() && !edit->isReadOnly()) return edit;
        return static_cast<QLineEdit *>(nullptr);
    };
    auto name = filenameEdit(dialog);
    require(name, "native editable filename field preserved");
    const QString original = name->text();
    dialog->resize(1400, 650); QTest::qWait(120);
    const int shortWidth = name->width();
    require(shortWidth < 250, "short filename stays compact in wide Properties");
    const QString longName = "1478766-download-free-spring-in-the-mountains-wallpaper-2560x1600-windows-10.jpg";
    name->setText(longName); QTest::qWait(120);
    const int longWidth = name->width();
    require(longWidth > shortWidth + 200 && longWidth < dialog->width() - 200, "filename grows to fit text without filling wide dialog");
    require(longWidth >= name->fontMetrics().horizontalAdvance(longName) + 16, "filename fits with comfortable padding");
    require(dialog->grab().save(output + "/properties-smart-filename.png"), "content-sized filename capture");
    dialog->resize(430, 540); QTest::qWait(120);
    require(dialog->width() == 430 && name->width() < longWidth, "long filename contracts with narrow dialog without forcing window expansion");
    name->setFocus(); QTest::keyClick(name, Qt::Key_End); QTest::keyClicks(name, "-edit");
    require(name->text() == longName + "-edit", "constrained filename remains editable");
    dialog->resize(1400, 650); name->setText(original); QTest::qWait(120);
    require(name->width() == shortWidth, "filename shrinks again after shortening");
    name->setText(QString(220, 'W')); QTest::qWait(120);
    require(dialog->width() == 1400 && name->width() < dialog->width(), "very long filename is bounded by available space");
    name->setText(original + "-cancelled");
    QTest::keyClick(dialog, Qt::Key_Escape); QTest::qWait(80);
    require(!window->findChild<QDialog *>("gwenviewPropertiesDialog"), "Escape cancels and destroys Properties safely");
    action->trigger();
    dialog = window->findChild<QDialog *>("gwenviewPropertiesDialog");
    require(dialog, "Properties can reopen");
    QTest::keyClick(dialog, Qt::Key_Shift); QTest::qWait(30);
    entrance = dialog->findChild<QWidget *>("dialogEntrance");
    require(!entrance->isVisible(), "immediate dialog input cancels a pending entrance");
    name = filenameEdit(dialog);
    require(name && name->text() == original, "cancelled property edits do not alter the file");
    dialog->reject(); QTest::qWait(80);
    action->trigger();
    dialog = window->findChild<QDialog *>("gwenviewPropertiesDialog");
    NoSidebarMotionStyle noMotion; dialog->setStyle(&noMotion); QTest::qWait(50);
    require(!dialog->findChild<QWidget *>("dialogEntrance")->isVisible(), "disabled motion skips Properties entrance");
    dialog->setStyle(nullptr); dialog->reject(); QTest::qWait(80);
    // The offscreen platform has no window manager to reactivate the owner.
    QApplication::setActiveWindow(window);
    fprintf(stderr, "PASS: Properties filename sizing/edit/cancel, overflow pills, page material, rapid switching, immediate input and reduced motion\n");
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
    const QColor normalControlColor = qApp->palette().color(QPalette::Window);
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
    auto pin = button("keep_controls_visible"); pin->click(); QTest::qWait(240);
    require(pin->isChecked() && bar->y() == 0, "pin keeps fullscreen controls revealed");
    for (const char *name : {"go_previous", "toggle_slideshow", "go_next", "rotate_left", "rotate_right", "view_zoom_in", "view_zoom_out", "view_zoom_to_fit", "view_actual_size", "leave_fullscreen", "options", "browse", "toggle_sidebar"}) button(name);
    require(info->toolTip().contains("scene-00") && count->text().contains("of"), "image information and count are shown");
    require((surface->palette().color(QPalette::Window).lightness() < 128) == (normalControlColor.lightness() < 128), "fullscreen pills follow the desktop light/dark palette");
    require(button("go_previous")->parentWidget() == button("go_next")->parentWidget()
            && button("view_zoom_in")->parentWidget() != button("rotate_left")->parentWidget(),
            "fullscreen controls are grouped into related floating pills");
    require(!surface->property("floatingBackdropImage").value<QImage>().isNull(), "fullscreen controls share a blurred image backdrop");
    QImage stripes(viewer->viewport()->size(), QImage::Format_RGB32);
    for (int y = 0; y < stripes.height(); ++y)
        for (int x = 0; x < stripes.width(); ++x) stripes.setPixel(x, y, (x / 8) % 2 ? qRgb(255,255,255) : qRgb(0,0,0));
    auto pattern = viewer->scene()->addPixmap(QPixmap::fromImage(stripes));
    pattern->setPos(viewer->mapToScene(QPoint())); pattern->setTransform(viewer->transform().inverted()); pattern->setZValue(100000);
    QTest::qWait(240);
    auto blurred = surface->property("floatingBackdropImage").value<QImage>();
    int low = 255, high = 0;
    for (int x = 20; x < blurred.width() - 20; ++x) { const int gray = qGray(blurred.pixel(x, 5)); low = qMin(low, gray); high = qMax(high, gray); }
    require(high - low < 30, "fullscreen backdrop softens high-contrast details behind controls");
    capture("fullscreen-blur");
    stripes.fill(Qt::red); pattern->setPixmap(QPixmap::fromImage(stripes)); QTest::qWait(120);
    const QColor red = surface->property("floatingBackdropImage").value<QImage>().pixelColor(20, 5);
    stripes.fill(Qt::blue); pattern->setPixmap(QPixmap::fromImage(stripes)); QTest::qWait(120);
    const QColor blue = surface->property("floatingBackdropImage").value<QImage>().pixelColor(20, 5);
    require(red.red() > blue.red() + 100 && blue.blue() > red.blue() + 100, "fullscreen blur refreshes with the image");
    delete pattern; QTest::qWait(240);
    auto idleProbe = new BackdropRenderProbe(viewer->mapToScene(viewer->viewport()->rect()).boundingRect());
    viewer->scene()->addItem(idleProbe); QTest::qWait(150); idleProbe->imagePaints = 0;
    QTest::qWait(240);
    require(idleProbe->imagePaints <= 1, "settled fullscreen blur does not continuously recapture the image");
    delete idleProbe;
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

    verifyProperties(window, output);

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
    window->resize(640, 480); QTest::qWait(240);
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
    button("toggle_sidebar")->click(); QTest::qWait(240);
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


static void verifyImageSlide(QWidget *window, const QString &output)
{
    QApplication::setActiveWindow(window);
    const QString root = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/Large/";
    const QUrl first = QUrl::fromLocalFile(root + "large-0.jpg");
    require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, first)), "open 48-megapixel image");
    QTest::qWait(1800);
    auto viewer = window->findChild<QGraphicsView *>("roundedImageViewer");
    auto animation = viewer->findChild<QVariantAnimation *>("imageSlideAnimation");
    auto frame = viewer->findChild<QWidget *>("imageSlideFrame");
    auto sidebar = window->findChild<QWidget *>("sidebarIsland");
    auto split = qobject_cast<QSplitter *>(sidebar->parentWidget());
    require(animation && frame && animation->duration() == 200, "quick image transition available");
    auto waitSlide = [&] {
        QElapsedTimer wait; wait.start();
        while (animation->state() != QAbstractAnimation::Running && wait.elapsed() < 2000) QTest::qWait(5);
        require(animation->state() == QAbstractAnimation::Running && frame->isVisible(), "slide starts after incoming image is ready");
    };
    for (bool fullscreen : {false, true}) {
        if (fullscreen) { findAction(window, "fullscreen")->trigger(); QTest::qWait(400); }
        if (!sidebar->isVisible()) findAction(window, "toggle_sidebar")->trigger();
        window->resize(1050, 740); split->setSizes({240, 810}); QTest::qWait(300);
        viewer->setFocus();
        QTimer heartbeat; heartbeat.setInterval(8);
        QElapsedTimer clock; clock.start(); qint64 last = 0, maxGap = 0;
        auto connection = QObject::connect(&heartbeat, &QTimer::timeout, window, [&] {
            const qint64 now = clock.elapsed(); maxGap = qMax(maxGap, now - last); last = now;
        });
        heartbeat.start();
        QElapsedTimer request; request.start();
        if (fullscreen) {
            auto next = window->findChild<QToolButton *>("fullscreen_go_next");
            require(next, "fullscreen arrow exists"); next->click();
        } else {
            auto toolbar = window->findChild<QToolBar *>("mainToolBar");
            auto next = qobject_cast<QToolButton *>(toolbar->widgetForAction(findAction(window, "go_next")));
            require(next, "native toolbar arrow exists"); next->click();
        }
        const auto requestMs = request.elapsed();
        waitSlide();
        require(frame->property("direction").toInt() == 1, "next image slides forward");
        require(frame->property("framePixels").toSize() == viewer->viewport()->size() * viewer->devicePixelRatioF(),
                "cached frame bounded by screen size, not source image resolution");
        require(frame->geometry() == viewer->viewport()->geometry() && sidebar->isVisible(), "slide fits beside open sidebar");
        ViewPaintCounter paints; viewer->viewport()->installEventFilter(&paints);
        int frames = 0;
        auto values = QObject::connect(animation, &QVariantAnimation::valueChanged, window, [&] { ++frames; });
        QTest::qWait(55);
        require(frames >= 2 && paints.paints <= 1, "animation moves cached frames without repainting large image");
        viewer->viewport()->removeEventFilter(&paints); QObject::disconnect(values);
        require(window->grab().save(output + (fullscreen ? "/image-slide-fullscreen.png" : "/image-slide-normal.png")), "capture image slide");
        QTest::qWait(170);
        require(!frame->isVisible() && animation->state() == QAbstractAnimation::Stopped, "slide releases overlay on completion");
        heartbeat.stop(); QObject::disconnect(connection);
        fprintf(stderr, "INFO: %s 48MP navigation request %lldms, heartbeat %lldms\n", fullscreen ? "fullscreen" : "normal", requestMs, maxGap);
        require(requestMs < 100 && maxGap < 180, "large image navigation stays responsive");
        // The actual image adapter handles Left/Right, sharing toolbar navigation.
        viewer->setFocus(); QTest::keyClick(viewer->viewport(), Qt::Key_Left); waitSlide();
        require(frame->property("direction").toInt() == -1, "keyboard previous slides backward");
        const QRect before = viewer->viewport()->geometry();
        QTest::qWait(240);
        require(viewer->viewport()->geometry() == before, "image navigation never moves native viewport geometry");
        findAction(window, "go_next")->trigger(); waitSlide();
        // Rapid next/previous must take effect without waiting for animations.
        findAction(window, "go_next")->trigger();
        findAction(window, "go_previous")->trigger();
        QTest::qWait(700);
        require(!frame->isVisible() && window->windowTitle().contains("large-1"), "rapid navigation lands on latest image without queued slides");
        findAction(window, "go_previous")->trigger(); waitSlide();
        split->setSizes({320, 730}); QTest::qWait(30);
        require(!frame->isVisible(), "sidebar resize dismisses slide immediately");
        QTest::qWait(240);
        NoSidebarMotionStyle noMotion; viewer->setStyle(&noMotion);
        findAction(window, "go_next")->trigger(); QTest::qWait(250);
        require(!frame->isVisible(), "reduced motion skips image slide");
        viewer->setStyle(nullptr);
        findAction(window, "go_previous")->trigger(); QTest::qWait(500);
    }
    fprintf(stderr, "PASS: image slides in normal/fullscreen, toolbar/keyboard, 48MP frames, sidebar resize, rapid reversals and reduced motion\n");
}



static bool transparentMenusExpected()
{
    return qEnvironmentVariable("SEQUOIA_GWENVIEW_MATERIAL") == QLatin1String("transparent");
}
static void requirePopupBlur(QWidget *popup)
{
    auto handle = popup->windowHandle();
    require(handle && handle->property("verifiedBlurRequests").toInt() > 0, "popup submits a native material region");
    const QRegion region = handle->property("verifiedBlurRegion").value<QRegion>();
    require(!region.isEmpty(), "popup blur never uses the whole-window sentinel");
    require(handle->property("verifiedVisibleBlur").toBool() == transparentMenusExpected(), "blur region follows transparent/solid material");
    if (transparentMenusExpected()) {
        require(region.contains(popup->rect().center()), "transparent menu requests blur behind its body");
        if (region.contains(QPoint(14, 14))) {
            const QRect bounds = region.boundingRect();
            fprintf(stderr, "BLUR DETAIL: %s %s size=%dx%d region=%d,%d,%d,%d alpha=%d\n", popup->metaObject()->className(), qPrintable(popup->objectName()), popup->width(), popup->height(), bounds.x(), bounds.y(), bounds.width(), bounds.height(), popup->palette().color(QPalette::Window).alpha());
        }
        require(!region.contains(QPoint(14, 14)), "blur follows rounded menu corners");
        require(QRect(14, 14, popup->width() - 28, popup->height() - 26).contains(region.boundingRect()), "blur stays out of shadow margins, including during slide");
    }
}

static void verifyPopupOpening(QMenu *menu, const QPoint &anchor, const QString &output, const QString &name)
{
    QElapsedTimer request; request.start(); menu->popup(anchor);
    const auto requestMs = request.elapsed();
    const QRect nativeGeometry = menu->geometry();
    QTest::qWait(25);
    auto animation = menu->findChild<QVariantAnimation *>("popupEntranceAnimation");
    auto effect = menu->findChild<QGraphicsEffect *>("popupEntranceEffect");
    require(animation && animation->duration() == 200 && !effect && !menu->graphicsEffect()
        && animation->state() == QAbstractAnimation::Running, "menu uses a live 200ms entrance without a cached window effect");
    ViewPaintCounter paints; menu->installEventFilter(&paints);
    int frames = 0;
    auto connection = QObject::connect(animation, &QVariantAnimation::valueChanged, menu, [&] { ++frames; });
    QTest::qWait(45);
    QObject::disconnect(connection); menu->removeEventFilter(&paints);
    require(frames >= 2 && paints.paints >= 2, "menu fades live rows without swapping paint paths");
    require(menu->geometry() == nativeGeometry, "menu opening never moves native hit targets");
    require(menu->grab().save(output + '/' + name + "-opening.png"), "capture menu entrance");
    QTest::qWait(160);
    require(menu->property("gwenviewPopupProgress").toReal() == 1.0, "menu fade completes");
    requirePopupBlur(menu);
    menu->move(menu->pos() + QPoint(1, 0)); menu->resize(menu->width(), menu->height() + 2); QTest::qWait(30);
    requirePopupBlur(menu);
    menu->close();
    menu->popup(anchor); QTest::keyClick(menu, Qt::Key_Down);
    require(animation->state() == QAbstractAnimation::Stopped && menu->activeAction(), "immediate keyboard navigation cancels pending entrance without waiting");
    QTest::keyClick(menu, Qt::Key_Escape);
    menu->popup(anchor); QTest::qWait(20);
    auto first = menu->actions().first(); const QPoint point = menu->actionGeometry(first).center();
    QMouseEvent move(QEvent::MouseMove, point, menu->mapToGlobal(point), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(menu, &move);
    require(!menu->graphicsEffect() && menu->geometry().size() == nativeGeometry.size(), "pointer hover keeps live geometry and paint path");
    menu->close();
    for (int i = 0; i < 3; ++i) { menu->popup(anchor); menu->close(); }
    QTest::qWait(160);
    require(animation->state() == QAbstractAnimation::Stopped && !menu->isVisible(), "rapid close/reopen leaves no delayed animation");
    auto originalStyle = menu->style(); NoSidebarMotionStyle noMotion; menu->setStyle(&noMotion);
    menu->popup(anchor); QTest::qWait(25);
    require(animation->state() == QAbstractAnimation::Stopped, "reduced motion skips menu entrance"); menu->close(); menu->setStyle(originalStyle);
    require(requestMs < 60, "menu opening does not wait for animation or expensive preparation");
    fprintf(stderr, "INFO: %s menu opens in %lldms; live entrance frames=%d paints=%d\n", qPrintable(name), requestMs, frames, paints.paints);
}

// Headless KWin has no pointer-grab serial. Tool-role QMenus exercise real
// wl_surface destruction/recreation without needing input on the user's desktop.
// Seed the theme's blur request explicitly because Tool roles skip its popup
// registration; the production PopupMotion filter must suppress it safely.
static void verifyNativeMenuSurfaces(QWidget *window)
{
    const QUrl gallery = QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/Gallery");
    require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, gallery)), "open native menu fixture");
    QTest::qWait(400);
    auto sort = window->findChild<QToolButton *>("browseSortButton");
    require(sort && sort->menu(), "native dropdown fixture exists");
    QMenu *menu = sort->menu();
    menu->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    auto cycle = [&](QMenu *popup) {
        popup->popup(window->mapToGlobal(QPoint(350, 160))); QTest::qWait(40);
        require(popup->isVisible() && popup->windowHandle(), "native menu surface is shown");
        KWindowEffects::enableBlurBehind(popup->windowHandle(), true, QRegion(popup->rect()));
        QEvent expose(QEvent::Expose); QApplication::sendEvent(popup, &expose);
        QTest::qWait(80);
        requirePopupBlur(popup);
        QTest::keyClick(popup, Qt::Key_Down);
        require(popup->activeAction(), "native menu keyboard selection works");
        popup->close(); QTest::qWait(30);
    };
    for (bool full : {false, true}) {
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(250); }
        for (int i = 0; i < 24; ++i) {
            cycle(menu); // Same QWindow, newly created Wayland surface every time.
            QMenu temporary(window); temporary.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            temporary.addAction("Context action"); temporary.addAction("Second action");
            cycle(&temporary); // Destroy the QWindow too, as right-click menus do.
        }
    }
    fprintf(stderr, "PASS: 96 real Wayland menu surface cycles, reused and destroyed windows, normal/fullscreen, no blur halo or protocol failure\n");
}

// Check the entire menu interior, including its opening frame and palette groups.
static void requirePopupMaterial(QWidget *popup, const char *stage)
{
    require(popup && popup->isVisible(), stage);
    const QRegion beforeGrab = popup->windowHandle()->property("verifiedBlurRegion").value<QRegion>();
    const QImage shot = popup->grab().toImage();
    const QRegion afterGrab = popup->windowHandle()->property("verifiedBlurRegion").value<QRegion>();
    if (beforeGrab != afterGrab) fprintf(stderr, "INFO: capture re-polished %s; allowing its queued native updates\n", popup->metaObject()->className());
    // QWidget::grab can synchronously polish lazy menu controls and invoke the
    // style's blur helper. Let the scheduled native-region repair run afterward.
    QTest::qWait(10);
    const qreal dpr = shot.devicePixelRatio();
    int minimumAlpha = 255;
    for (int y = 32 * dpr; y < shot.height() - 32 * dpr; y += 7)
        for (int x = 32 * dpr; x < shot.width() - 32 * dpr; x += 7)
            minimumAlpha = qMin(minimumAlpha, shot.pixelColor(x, y).alpha());
    if (!(transparentMenusExpected() ? minimumAlpha >= 180 && minimumAlpha <= 190 : minimumAlpha == 255)) fprintf(stderr, "FAIL DETAIL: %s alpha=%d class=%s\n", stage, minimumAlpha, popup->metaObject()->className());
    const bool materialMatches = transparentMenusExpected() ? minimumAlpha >= 180 && minimumAlpha <= 190 : minimumAlpha == 255;
    require(materialMatches && popup->windowOpacity() == 1.0, stage);
    requirePopupBlur(popup);
}

static int verifyMenuCatalog(QWidget *window)
{
    QList<QPointer<QMenu>> menus;
    QSet<QMenu *> seen;
    auto add = [&](QMenu *menu) {
        if (menu && !seen.contains(menu)) { seen.insert(menu); menus.append(menu); }
    };
    for (auto menu : window->findChildren<QMenu *>()) add(menu);
    for (auto widget : qApp->topLevelWidgets()) add(qobject_cast<QMenu *>(widget));
    for (auto action : window->findChildren<QAction *>()) add(action->menu());
    int opened = 0;
    for (int index = 0; index < menus.size(); ++index) {
        auto menu = menus[index];
        if (!menu || menu->isEmpty()) continue;
        for (int repeat = 0; repeat < 3; ++repeat) {
            menu->popup(window->mapToGlobal(QPoint(350, 180))); QTest::qWait(25);
            requirePopupMaterial(menu, "catalog menu material stable during entrance");
            QTest::qWait(120); requirePopupMaterial(menu, "catalog menu material stable after entrance");
            for (auto action : menu->actions()) add(action->menu());
            QTest::keyClick(menu, Qt::Key_Escape); QTest::qWait(10); ++opened;
        }
    }
    fprintf(stderr, "INFO: catalog checked %d populated menus and nested menus (%d openings)\n", opened / 3, opened);
    return opened;
}

class MenuStructureCounter : public QObject
{
public:
    int changes = 0;
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved) ++changes;
        return false;
    }
};
static void verifyStableMenuActions(QWidget *window)
{
    auto bar = window->findChild<QMenuBar *>(); require(bar, "global-menu source menu bar exists");
    MenuStructureCounter counter;
    QList<QMenu *> menus;
    for (auto action : bar->actions()) if (action->menu()) { menus << action->menu(); action->menu()->installEventFilter(&counter); }
    require(!menus.isEmpty(), "global-menu categories exist");
    auto openWith = findAction(window, "file_open_with");
    require(openWith->icon().name() == "system-run-symbolic", "Open With uses a symbolic application command icon");
    auto menu = openWith->menu();
    require(QMetaObject::invokeMethod(menu, "aboutToShow"), "populate Open With actions");
    const auto originalActions = menu->actions();
    require(!originalActions.isEmpty() && originalActions.last()->icon().name() == "system-run-symbolic", "Other Application uses a symbolic icon");
    for (int i = 0; i < 20; ++i) {
        findAction(window, i % 2 ? "go_previous" : "go_next")->trigger(); QTest::qWait(20);
        require(QMetaObject::invokeMethod(menu, "aboutToShow"), "refresh associated applications");
        require(menu->actions() == originalActions, "repeated Open With keeps QAction identity and order stable");
        for (auto category : menus) {
            QSet<QString> ids;
            for (auto action : category->actions()) {
                if (action->isSeparator() || action->objectName().isEmpty()) continue;
                require(!ids.contains(action->objectName()), "global-menu category has no duplicate command IDs");
                ids.insert(action->objectName());
            }
        }
    }
    require(counter.changes == 0, "image navigation does not remove/reinsert global-menu command lists");
    for (auto category : menus) category->removeEventFilter(&counter);
    fprintf(stderr, "PASS: stable global-menu category commands and Open With action identities across repeated image changes\n");
}

// Exercise atomic kdeglobals replacement, delayed palette delivery, open
// menus, retained hidden popups and subsequent reopenings on the same process.
static void verifyLivePopupThemes(QWidget *window)
{
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals";
    QFile originalFile(configPath); require(originalFile.open(QIODevice::ReadOnly), "read isolated palette configuration");
    const QByteArray originalConfig = originalFile.readAll(); originalFile.close();
    const QPalette originalPalette = qApp->palette();
    const QByteArray originalMaterial = qgetenv("SEQUOIA_GWENVIEW_MATERIAL");
    auto writeConfig = [&](const QByteArray &contents) {
        QSaveFile file(configPath); require(file.open(QIODevice::WriteOnly), "write isolated live theme");
        require(file.write(contents) == contents.size() && file.commit(), "atomically replace isolated theme configuration");
        QTest::qWait(100);
    };
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png"))), "open image for live theme switches");
    QTest::qWait(300);
    verifyStableMenuActions(window);
    auto share = findAction(window, "share")->menu();
    auto openWith = findAction(window, "file_open_with")->menu();
    auto page = window->findChild<QWidget *>("viewMainPage");
    if (!page) for (auto child : window->findChildren<QWidget *>()) if (child->inherits("Gwenview::ViewMainPage")) page = child;
    require(page, "image page available for live zoom updates");
    auto combo = page->findChild<QComboBox *>(); require(combo, "live zoom popup available");
    for (bool full : {false, true}) {
        auto fullscreen = findAction(window, "fullscreen");
        if (fullscreen->isChecked() != full) { fullscreen->trigger(); QTest::qWait(250); }
        for (int target = 0; target < 3; ++target) {
            QMenu *menu = target == 0 ? share : target == 1 ? openWith : nullptr;
            if (menu) menu->popup(window->mapToGlobal(QPoint(400, 180))); else combo->showPopup();
            QTest::qWait(150);
            QWidget *popup = menu ? static_cast<QWidget *>(menu) : combo->view()->window();
            for (const QString &variant : {QStringLiteral("DarkTransparent"), QStringLiteral("LightSolid"), QStringLiteral("LightTransparent"), QStringLiteral("DarkSolid")}) {
                QFile scheme(qEnvironmentVariable("SEQUOIA_GWENVIEW_THEMES") + "/SequoiaEleven" + variant + ".colors");
                require(scheme.open(QIODevice::ReadOnly), "read alternate theme fixture");
                qputenv("SEQUOIA_GWENVIEW_MATERIAL", variant.endsWith("Transparent") ? "transparent" : "solid");
                writeConfig(scheme.readAll());
                requirePopupMaterial(popup, "open popup immediately follows atomic solid/transparent theme switch");
                for (auto retained : QApplication::topLevelWidgets()) {
                    if (!retained->property("gwenviewFloatingPopup").toBool()) continue;
                    require(retained->palette().color(QPalette::Window).alpha() == (transparentMenusExpected() ? 184 : 255),
                            "hidden retained menus and submenus update before their next opening");
                }
                if (target < 2 && variant.endsWith("Transparent"))
                    require(popup->grab().save(qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY") + QString("/live-%1-%2-%3.png").arg(target == 0 ? "share" : "open-with", full ? "fullscreen" : "normal", variant)), "capture live menu material and command icons");
                require((popup->palette().color(QPalette::Window).lightness() < 128) == variant.startsWith("Dark"), "open popup follows light/dark switch");
                // Simulate the platform theme delivering its palette later.
                qApp->setPalette(popup->palette()); QTest::qWait(40);
                requirePopupMaterial(popup, "late application palette event preserves current material");
                if (menu) { menu->close(); menu->popup(window->mapToGlobal(QPoint(400, 180))); }
                else { combo->hidePopup(); combo->showPopup(); }
                QTest::qWait(240);
                requirePopupMaterial(popup, "reopened popup retains live theme material");
            }
            if (menu) menu->close(); else combo->hidePopup();
        }
    }
    qputenv("SEQUOIA_GWENVIEW_MATERIAL", originalMaterial);
    writeConfig(originalConfig); qApp->setPalette(originalPalette); QTest::qWait(100);
    fprintf(stderr, "PASS: live light/dark solid/transparent updates on open/reopened Share, Open With submenu and zoom, normal/fullscreen\n");
}

static void verifyPopupLifecycle(QWidget *window)
{
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    int openings = 0;
    QApplication::setActiveWindow(window);
#ifdef GWENVIEW_NATIVE_MENU_INPUT
    window->showMaximized(); QTest::qWait(250);
    GwenviewMenuInput input;
    auto checkEffect = QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.kde.kwin.Effects", "isEffectLoaded");
    checkEffect << QStringLiteral("blur");
    const auto effectReply = QDBusConnection::sessionBus().call(checkEffect);
    require(effectReply.type() == QDBusMessage::ReplyMessage && !effectReply.arguments().isEmpty(), "read private compositor Blur state");
    const bool blurEnabled = qEnvironmentVariable("SEQUOIA_GWENVIEW_TEST_BLUR") != QLatin1String("0");
    require(effectReply.arguments().first().toBool() == blurEnabled, "private Plasma Blur setting matches requested test state");
    fprintf(stderr, "INFO: native compositor Blur effect is %s\n", blurEnabled ? "enabled" : "disabled");
    input.move(QPoint(1580, 1180), 1); QTest::qWait(60);
    input.button(true); QTest::qWait(60);
#endif
    auto hamburger = findAction(window, "hamburger_menu");
    require(QMetaObject::invokeMethod(hamburger, "aboutToShowMenu"), "populate main menu before catalog audit");
    for (bool full : {false, true}) {
        require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected",
            Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery"))), "open browser for popup lifecycle");
        QTest::qWait(350);
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(250); }
        auto sort = window->findChild<QToolButton *>("browseSortButton");
        require(sort && sort->menu(), "sort menu exists for repeated openings");
        for (int i = 0; i < 16; ++i) {
            sort->menu()->popup(sort->mapToGlobal(QPoint(0, sort->height())));
            QTest::qWait(i % 2 ? 160 : 40);
            require(sort->menu()->isVisible(), "reopened native menu stays usable");
            QTest::keyClick(sort->menu(), Qt::Key_Down);
            require(sort->menu()->activeAction(), "reopened menu responds to keyboard");
            QTest::keyClick(sort->menu(), Qt::Key_Escape); QTest::qWait(30);
            ++openings;
        }
        openings += verifyMenuCatalog(window);
        auto browser = window->findChild<QListView *>("mThumbnailView");
        require(browser, "browser thumbnail context target exists");
        for (int i = 0; i < 8; ++i) {
            QTimer::singleShot(160, window, [&] {
                auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                requirePopupMaterial(menu, "browser thumbnail context menu keeps theme material");
                QTest::keyClick(menu, Qt::Key_Escape);
            });
            require(QMetaObject::invokeMethod(browser, "showContextMenu"), "open browser context menu repeatedly");
            QTest::qWait(30); ++openings;
        }
        if (full) { findAction(window, "leave_fullscreen")->trigger(); QTest::qWait(240); }
        require(QMetaObject::invokeMethod(window, "openUrl",
            Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png"))), "open image for popup lifecycle");
        QTest::qWait(400);
        QWidget *page = nullptr;
        for (auto candidate : window->findChildren<QWidget *>()) if (candidate->inherits("Gwenview::ViewMainPage")) page = candidate;
        require(page, "image page for popup lifecycle");
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(250); }
        auto share = findAction(window, "share")->menu();
        require(share, "actual Purpose Share menu exists");
        for (int i = 0; i < 12; ++i) {
            if (i % 3 == 0) { findAction(window, "go_next")->trigger(); QTest::qWait(50); }
            share->popup(window->mapToGlobal(QPoint(400, 180))); QTest::qWait(25);
            requirePopupMaterial(share, "Share body material stable during opening");
            if (i % 2 == 0) {
                QPalette inherited = share->palette();
                QColor translucent = inherited.color(QPalette::Window); translucent.setAlpha(85);
                inherited.setColor(QPalette::Window, translucent); share->setPalette(inherited);
            }
            if (i % 3 == 0) findAction(window, "go_next")->trigger();
            QTest::qWait(160);
            requirePopupMaterial(share, "Share body material stable after live palette/action refresh");
            require(share->grab().save(qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY") + "/share.png"), "capture Share menu");
            share->close(); QTest::qWait(30); ++openings;
        }
        auto combo = page->findChild<QComboBox *>(); require(combo, "zoom combo for popup lifecycle");
        for (int i = 0; i < 16; ++i) {
            combo->showPopup(); QTest::qWait(i % 2 ? 160 : 40);
            requirePopupMaterial(combo->view()->window(), "reopened zoom list keeps theme material and remains usable");
            combo->hidePopup(); QTest::qWait(30); ++openings;
        }
        openings += verifyMenuCatalog(window);
        if (full) {
            for (int i = 0; i < 6; ++i) {
                QTimer::singleShot(160, window, [&] {
                    auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                    require(menu && menu->objectName() == "fullscreenOptionsMenu", "fullscreen settings open repeatedly");
                    requirePopupMaterial(menu, "fullscreen settings keep theme material");
                    require(menu->findChild<QSlider *>("mHeightSlider"), "fullscreen settings retain widget controls");
                    QTest::keyClick(menu, Qt::Key_Escape);
                });
                window->findChild<QToolButton *>("fullscreen_options")->click(); QTest::qWait(30); ++openings;
            }
        }
        for (int i = 0; i < 16; ++i) {
            bool inspected = false;
            QTimer::singleShot(100, window, [&] {
                auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                requirePopupMaterial(menu, "recreated context menu is usable with theme material");
                QAction *child = nullptr;
                for (auto action : menu->actions()) if (action->menu()) { child = action; break; }
                require(child, "context submenu available");
                menu->setActiveAction(child); QTest::keyClick(menu, Qt::Key_Right); QTest::qWait(70);
                requirePopupMaterial(child->menu(), "recreated context submenu is usable with theme material");
                QTest::keyClick(child->menu(), Qt::Key_Escape);
                QTest::keyClick(menu, Qt::Key_Escape); inspected = true;
            });
            require(QMetaObject::invokeMethod(page, "showContextMenu"), "open context menu repeatedly");
            require(inspected, "context menu completed native event loop");
            QTest::qWait(30); openings += 2;
        }
        fprintf(stderr, "INFO: %s popup lifecycle completed\n", full ? "fullscreen" : "normal");
    }
#ifdef GWENVIEW_NATIVE_MENU_INPUT
    // Change only the private compositor's effect while a real Share popup is
    // open. Material alpha must not flip, and the connection must remain alive.
    verifyLivePopupThemes(window);
    auto shareForBlur = findAction(window, "share")->menu();
    shareForBlur->popup(window->mapToGlobal(QPoint(400, 180))); QTest::qWait(160);
    for (bool enabled : {!blurEnabled, blurEnabled}) {
        auto change = QDBusMessage::createMethodCall("org.kde.KWin", "/Effects", "org.kde.kwin.Effects", enabled ? "loadEffect" : "unloadEffect");
        change << QStringLiteral("blur");
        require(QDBusConnection::sessionBus().call(change).type() == QDBusMessage::ReplyMessage, "toggle private compositor Blur effect");
        QTest::qWait(240);
        auto state = QDBusConnection::sessionBus().call(checkEffect);
        require(state.type() == QDBusMessage::ReplyMessage && state.arguments().first().toBool() == enabled, "live Blur toggle takes effect");
        requirePopupMaterial(shareForBlur, "Share keeps theme material across live Plasma Blur toggles");
    }
    shareForBlur->close();
    input.button(false);
    fprintf(stderr, "PASS: live Plasma Blur on/off leaves theme material and menu connection intact\n");
#else
    verifyLivePopupThemes(window);
#endif
    fprintf(stderr, "PASS: %d Qt popup openings, menu catalog, Share live refresh, zoom, context/submenus and fullscreen settings, theme material in normal/fullscreen\n", openings);
}

static void verifyFloatingPopups(QWidget *window, const QString &output)
{
    QApplication::setActiveWindow(window);
    auto checkSurface = [&](QWidget *popup, const QString &name) {
        require(popup && popup->isVisible() && popup->property("gwenviewFloatingPopup").toBool(), "native popup uses floating surface");
        QTest::qWait(60);
        requirePopupBlur(popup);
        QImage shot = popup->grab().toImage();
        require(shot.pixelColor(0, 0).alpha() == 0, "popup outer corners are transparent");
        const qreal dpr = shot.devicePixelRatio();
        require(shot.pixelColor(14 * dpr, 14 * dpr).alpha() < 100 && shot.pixelColor(30 * dpr, 30 * dpr).alpha() > 200,
                "popup body has rounded corners and readable themed content");
        require(shot.pixelColor(shot.width() / 2, shot.height() - qRound(12 * dpr)).alpha() > 0, "popup has a visible floating shadow below the surface");
        require((popup->palette().color(QPalette::Window).lightness() < 128)
                == (qApp->palette().color(QPalette::Window).lightness() < 128), "popup uses desktop palette even in fullscreen");
        require(shot.save(output + '/' + name + ".png"), "capture polished popup");
    };
    const QString fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    const QUrl gallery = QUrl::fromLocalFile(fixtures + "/Gallery");
    require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl, gallery)), "open browse menu fixture");
    QTest::qWait(500);
    auto sort = window->findChild<QToolButton *>("browseSortButton");
    require(sort && sort->menu(), "native sort dropdown exists");
    for (bool full : {false, true}) {
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(250); }
        QMenu *menu = sort->menu();
        const QPoint anchor = sort->mapToGlobal(QPoint(0, sort->height()));
        verifyPopupOpening(menu, anchor, output, full ? "sort-fullscreen" : "sort-normal");
        menu->popup(anchor); QTest::qWait(160);
        checkSurface(menu, full ? "sort-fullscreen" : "sort-normal");
        QAction *choice = nullptr;
        for (auto action : menu->actions()) if (action->isCheckable() && !action->isChecked() && action->isEnabled()) { choice = action; break; }
        require(choice, "sort menu retains native checkable actions");
        const QRect actionRect = menu->actionGeometry(choice);
        require(actionRect.left() >= 14 && actionRect.right() <= menu->width() - 14, "menu hit areas stay inside the floating body");
        QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, actionRect.center()); QTest::qWait(80);
        require(choice->isChecked() && !menu->isVisible(), "native menu mouse selection commits and dismisses");
    }
    findAction(window, "leave_fullscreen")->trigger(); QTest::qWait(240);
    const QUrl image = QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png");
    require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, image)), "open image menu fixture"); QTest::qWait(600);
    QWidget *page = nullptr;
    for (auto candidate : window->findChildren<QWidget *>()) if (candidate->inherits("Gwenview::ViewMainPage")) page = candidate;
    require(page, "image page available");
    auto combo = page->findChild<QComboBox *>(); require(combo, "native zoom dropdown available");
    combo->showPopup(); QTest::qWait(25);
    auto comboEffect = combo->view()->window()->findChild<QGraphicsEffect *>("popupEntranceEffect");
    require(comboEffect && comboEffect->isEnabled(), "combo dropdown uses the same quick entrance");
    QTest::keyClick(combo->view(), Qt::Key_Down);
    require(!comboEffect->isEnabled(), "combo keyboard navigation takes over immediately");
    checkSurface(combo->view()->window(), "zoom-dropdown");
    require(combo->view()->viewport()->height() >= combo->fontMetrics().height() * 2, "popup padding preserves usable list space");
    QTest::keyClick(combo->view(), Qt::Key_Escape); QTest::qWait(50);
    auto slider = page->findChild<QSlider *>();
    require(slider, "zoom slider available");
    combo->view()->setMaximumHeight(140); combo->showPopup(); QTest::qWait(80); verifyZoomPopup(combo, slider);
    auto showStatus = findAction(window, "options_show_statusbar");
    if (showStatus->isChecked()) showStatus->trigger(); QTest::qWait(100);
    auto statusToggle = window->findChild<QToolButton *>("imageToolbarToggle");
    statusToggle->click(); QTest::qWait(260);
    for (bool full : {false, true}) {
        if (full) { findAction(window, "fullscreen")->trigger(); QTest::qWait(300); }
        bool inspected = false;
        QTimer::singleShot(25, window, [&] {
            auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            require(menu, "native context menu is open");
            auto entrance = menu->findChild<QVariantAnimation *>("popupEntranceAnimation");
            require(entrance && entrance->state() == QAbstractAnimation::Running, "right-click menu animates in normal and fullscreen views");
            QTest::qWait(100);
            checkSurface(menu, full ? "context-fullscreen" : "context-normal");
            QAction *submenu = nullptr;
            for (auto action : menu->actions()) if (action->menu()) { submenu = action; break; }
            require(submenu, "native context menu retains submenu");
            menu->setActiveAction(submenu); QTest::keyClick(menu, Qt::Key_Right); QTest::qWait(25);
            require(submenu->menu()->isVisible(), "keyboard opens native submenu");
            auto submenuEntrance = submenu->menu()->findChild<QVariantAnimation *>("popupEntranceAnimation");
            require(submenuEntrance && submenuEntrance->state() == QAbstractAnimation::Running, "submenu shares the opening animation");
            QTest::qWait(100);
            checkSurface(submenu->menu(), full ? "submenu-fullscreen" : "submenu-normal");
            QTest::keyClick(submenu->menu(), Qt::Key_Escape); QTest::qWait(30);
            require(menu->isVisible(), "Escape closes submenu and retains parent");
            QTest::keyClick(menu, Qt::Key_Escape); inspected = true;
        });
        require(QMetaObject::invokeMethod(page, "showContextMenu"), "open actual image context menu");
        require(inspected, "native context menu checked");
        auto toggle = window->findChild<QToolButton *>(full ? "fullscreen_toggle_sidebar" : "imageSidebarToggle");
        require(toggle && toggle->findChild<QVariantAnimation *>("sidebarButtonRippleAnimation"), "sidebar toggle has ripple in both modes");
        auto animation = toggle->findChild<QVariantAnimation *>("sidebarButtonRippleAnimation");
        QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(210);
        require(animation->state() == QAbstractAnimation::Running, "sidebar button click starts a visible outline wave");
        QWidget *wave = nullptr;
        for (auto candidate : window->findChildren<QWidget *>("sidebarButtonRippleWave")) if (candidate->isVisible()) wave = candidate;
        require(wave && wave->testAttribute(Qt::WA_TransparentForMouseEvents), "ripple paints above controls without blocking clicks");
        require(wave->width() > toggle->width(), "outline wave can expand beyond button border");
        QImage ring(wave->size(), QImage::Format_ARGB32_Premultiplied); ring.fill(Qt::transparent);
        wave->render(&ring, QPoint(), QRegion(), QWidget::DrawChildren);
        const QRect buttonRect(toggle->mapTo(window, QPoint()) - wave->pos(), toggle->size());
        int outsidePixels = 0;
        for (int y = 0; y < ring.height(); ++y) for (int x = 0; x < ring.width(); ++x)
            if (!buttonRect.adjusted(-1, -1, 1, 1).contains(x, y) && qAlpha(ring.pixel(x, y)) > 10) ++outsidePixels;
        require(outsidePixels > 12, "accent outline actually paints outside button, not just inside the icon");

        require(window->grab().save(output + (full ? "/ripple-fullscreen.png" : "/ripple-normal.png")), "capture sidebar ripple");
        QTest::qWait(260); require(!wave->isVisible(), "ripple clears after its brief animation");
        QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(20); QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(350);
        require(animation->state() == QAbstractAnimation::Stopped, "repeated clicks restart one ripple without queued animations");
        NoSidebarMotionStyle noMotion; toggle->setStyle(&noMotion);
        QTest::mouseClick(toggle, Qt::LeftButton); QTest::qWait(30);
        require(animation->state() == QAbstractAnimation::Stopped, "reduced motion skips sidebar ripple"); toggle->setStyle(nullptr);
        QTest::qWait(250);
    }
    fprintf(stderr, "PASS: floating dropdown/context/submenus, keyboard/mouse selection, zoom scrolling, fullscreen palettes and sidebar outline ripples\n");
}

#include "test-support/gwenview-dialog-checks.h"

static void startup() {
    if (qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY").isEmpty()) return;
    if (QFileInfo(QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget()).fileName() != QStringLiteral("gwenview")) return;
    QTimer::singleShot(1000, qApp, [] {
        QWidget *window = nullptr;
        for (auto w : QApplication::topLevelWidgets()) if (w->inherits("Gwenview::MainWindow")) window = w;
        require(window, "main window");
        window->resize(1100, 760);
        QTest::qWait(240);
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
        if (mode == "dialogs") {
            verifyMenuDialogs(window, output);
            window->close(); QCoreApplication::exit(0); return;
        }
        if (mode == "global-menu") {
            const auto fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
            require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png"))), "open global-menu fixture");
            window->showMaximized(); window->activateWindow(); QTest::qWait(400);
#ifdef GWENVIEW_NATIVE_MENU_INPUT
            GwenviewMenuInput globalInput;
            globalInput.move(QPoint(1400, 900), 1); QTest::qWait(50);
            globalInput.button(true); QTest::qWait(30); globalInput.button(false); QTest::qWait(300);
#endif
            const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
            for (int i = 0; i < 120 && !QFile::exists(runtime + "/global-menu-ready"); ++i) QTest::qWait(100);
            require(QFile::exists(runtime + "/global-menu-ready"), "real Plasma widget imports Gwenview menu categories");
            verifyStableMenuActions(window);
            for (int i = 0; i < 60; ++i) {
                if (i == 20 || i == 40) findAction(window, "fullscreen")->trigger();
                findAction(window, i % 2 ? "go_previous" : "go_next")->trigger();
                QMetaObject::invokeMethod(findAction(window, "file_open_with")->menu(), "aboutToShow");
                QTest::qWait(100);
            }
            QFile done(runtime + "/global-menu-done"); require(done.open(QIODevice::WriteOnly), "finish private global-menu check"); done.close();
            QTest::qWait(250);
            fprintf(stderr, "PASS: real global-menu export stress across image changes and fullscreen\n");
            window->close(); QCoreApplication::exit(0); return;
        }
        if (mode == "popup-lifecycle") {
            if (QGuiApplication::platformName() == QLatin1String("wayland") && !qEnvironmentVariableIsSet("SEQUOIA_GWENVIEW_NATIVE_INPUT")) verifyNativeMenuSurfaces(window);
            else verifyPopupLifecycle(window);
            require(window->close(), "clean lifecycle close"); QCoreApplication::exit(0); return;
        }
        if (mode == "popups") {
            verifyFloatingPopups(window, output);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "panel-edges") {
            const QUrl file = QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/one/picture.png");
            require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, file)), "open corner fixture"); QTest::qWait(600);
            auto viewer = window->findChild<QWidget *>("roundedImageViewer");
            const QImage screen = window->grab().toImage();
            const QPoint corner = viewer->mapTo(window, QPoint());
            const QColor inside = screen.pixelColor(corner), outside = screen.pixelColor(corner - QPoint(4, 0));
            require(qAbs(inside.red() - outside.red()) < 10 && qAbs(inside.green() - outside.green()) < 10
                && qAbs(inside.blue() - outside.blue()) < 10, "rounded viewer exposes parent background without opaque corner artifacts");
            const QImage cutout = viewer->grab().toImage();
            require(cutout.pixelColor(0, 0).alpha() == 0 && cutout.pixelColor(16, 16).alpha() == 255,
                    "actual viewer has transparent rounded corners and opaque content");
            bool fractional = false;
            for (int y = 0; y < 17; ++y) for (int x = 0; x < 17; ++x) {
                const int alpha = cutout.pixelColor(x, y).alpha();
                fractional |= alpha > 0 && alpha < 255;
            }
            require(fractional, "actual viewer edge retains antialiased alpha coverage");
            capture("viewer-rounded"); cutout.save(output + "/viewer-only.png");
            fprintf(stderr, "PASS: panel edge capture\n");
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "image-slide") {
            verifyImageSlide(window, output);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "thumbnail-scroll") {
            verifyThumbnailScrolling(window, output);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
        if (mode == "fullscreen-browser") {
            verifyFullscreenBrowser(window, output);
            require(window->close(), "clean close"); QCoreApplication::exit(0); return;
        }
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
            if (native) {
                auto controls = window->findChild<QWidget *>("imageToolbarControls");
                auto combo = controls ? controls->findChild<QComboBox *>() : nullptr;
                require(combo && combo->isVisible(), "native status bar zoom field visible");
                const int maxVisible = combo->maxVisibleItems();
                combo->setMaxVisibleItems(4);
                combo->showPopup(); QTest::qWait(100);
                verifyZoomPopup(combo, controls->findChild<QSlider *>());
                combo->setMaxVisibleItems(maxVisible);
                require(!combo->property("floatingZoomPill").toBool(), "native status bar restores system field painting");
                verifyClosedZoomWheel(combo, controls->findChild<QSlider *>(), findAction(window, "view_zoom_to_fit"));
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
            require(viewer->mask().isEmpty() && viewer->graphicsEffect() && viewer->graphicsEffect()->isEnabled(), "rounded viewer corners");
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
            QMap<int, QRect> statusGroups;
            for (auto widget : controls->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
                const int group = widget->property("floatingPillGroup").toInt();
                if (group && !widget->isHidden())
                    statusGroups[group] = statusGroups[group].united(QRect(widget->mapTo(surface, QPoint()), widget->size()));
            }
            require(statusGroups.size() == 3, "status bar groups layout, image information and zoom separately");
            for (const auto &group : statusGroups) {
                require(qAbs(QRectF(group).center().y() - 22.0) <= 0.5, "status pill contents are vertically centered in the painted capsule");
            }

            for (auto button : controls->findChildren<QToolButton *>()) {
                if (button->property("floatingPillGroup").toInt() != 1) continue;
                require(!button->icon().isNull() && button->toolButtonStyle() == Qt::ToolButtonIconOnly
                        && button->iconSize() == QSize(22, 22) && !button->toolTip().isEmpty(),
                        "floating panel toggles use matching icons with descriptive tooltips");
                if (button->defaultAction() && button->defaultAction()->objectName() == "toggle_thumbnailbar") {
                    auto action = button->defaultAction(); const bool checked = action->isChecked();
                    QTest::mouseClick(button, Qt::LeftButton); QTest::qWait(80);
                    require(action->isChecked() != checked, "floating thumbnail icon toggles the native strip");
                    QTest::mouseClick(button, Qt::LeftButton); QTest::qWait(80);
                    require(action->isChecked() == checked, "floating thumbnail toggle restores its state");
                }
            }
            QImage statusPills(surface->size(), QImage::Format_ARGB32_Premultiplied); statusPills.fill(Qt::transparent);
            surface->render(&statusPills, QPoint(), QRegion(), QWidget::DrawChildren);
            for (int group = 1; group < 3; ++group) {
                const QPoint gap((statusGroups[group].right() + statusGroups[group + 1].left()) / 2, 22);
                require(qAlpha(statusPills.pixel(gap)) == 0, "status pills leave clear space between related controls");
                require(!floating->mask().contains(surface->mapTo(floating, gap)), "status pill gaps pass clicks to image");
            }
            require(statusPills.save(output + "/floating-status-pills.png"), "capture grouped status controls");
            const int glassSampleX = statusGroups[3].center().x();
            auto topToggle = window->findChild<QToolButton *>("mainToolbarToggle");
            QTest::mouseClick(topToggle, Qt::LeftButton); QTest::qWait(300);
            auto paintedGap = [&](QWidget *bar, QWidget *pillSurface, QWidget *button) {
                QImage material(pillSurface->size(), QImage::Format_ARGB32_Premultiplied); material.fill(Qt::transparent);
                pillSurface->render(&material, QPoint(), QRegion(), QWidget::DrawWindowBackground);
                int right = material.width() - 1;
                while (right > 0 && qAlpha(material.pixel(right, 22)) < 40) --right;
                return button->mapTo(bar, QPoint(1, 22)).x() - pillSurface->mapTo(bar, QPoint(right, 22)).x();
            };
            auto topIsland = window->findChild<QWidget *>("floatingMainToolbar");
            auto topSurface = window->findChild<QScrollArea *>("mainToolbarFlyout")->parentWidget();
            const int topGap = paintedGap(topIsland, topSurface, topToggle);
            const int statusGap = paintedGap(floating, surface, toggle);
            fprintf(stderr, "INFO: painted toggle gaps toolbar=%d status=%d\n", topGap, statusGap);
            require(qAbs(topGap - statusGap) <= 1 && statusGap >= 6 && statusGap <= 12,
                    "status pill-to-toggle spacing matches toolbar without sacrificing hit targets");
            capture("floating-bars-polished");
            QTest::mouseClick(topToggle, Qt::LeftButton); QTest::qWait(300);
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)}) {
                QImage rendered(glass->size(), QImage::Format_ARGB32_Premultiplied);
                rendered.fill(Qt::transparent);
                glass->render(&rendered, QPoint(), QRegion(), QWidget::DrawChildren);
                require(rendered.save(output + (glass == surface ? "/toolbar-surface.png" : "/toolbar-toggle.png")), "glass screenshot");
                require(qAlpha(rendered.pixel(0, 0)) == 0, "glass rounded corners stay clear");
                const int alpha = qAlpha(rendered.pixel(glass == surface ? glassSampleX : rendered.width() / 2, 4));
                require(alpha == 255, "glass composites a blurred scene behind its tint");
            }
            require(!controls->autoFillBackground() && !flyout->viewport()->autoFillBackground(), "controls preserve glass background");
            auto combo = controls->findChild<QComboBox *>();
            require(combo && !controls->findChild<QWidget *>("imageZoomArrow"), "editable zoom dropdown retains native hit geometry");
            require(combo->property("floatingZoomPill").toBool(), "floating scale field uses capsule painting");
            const QPoint editorBlank = combo->lineEdit()->mapTo(combo, QPoint(combo->lineEdit()->width() - 3, combo->lineEdit()->height() / 2));
            const QColor blankColor = combo->grab().toImage().pixelColor(editorBlank);
            const QColor capsuleColor = combo->palette().color(QPalette::Button);
            require(qAbs(blankColor.red() - capsuleColor.red()) <= 2 && qAbs(blankColor.green() - capsuleColor.green()) <= 2
                    && qAbs(blankColor.blue() - capsuleColor.blue()) <= 2, "editor has no square background inside the scale capsule");
            QStyleOptionComboBox comboOption;
            comboOption.initFrom(combo);
            comboOption.editable = true;
            const QRect arrow = combo->style()->subControlRect(QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, combo);
            require(arrow.width() >= 12, "native zoom dropdown arrow hit area");
            const int maxVisiblePresets = combo->maxVisibleItems();
            combo->setMaxVisibleItems(4);
            QTest::mouseClick(combo, Qt::LeftButton, Qt::NoModifier, arrow.center());
            QTest::qWait(100);
            require(combo->view()->isVisible(), "zoom arrow opens preset menu");
            verifyZoomPopup(combo, controls->findChild<QSlider *>());
            combo->setMaxVisibleItems(maxVisiblePresets);
            verifyClosedZoomWheel(combo, controls->findChild<QSlider *>(), findAction(window, "view_zoom_to_fit"));
            findAction(window, "view_zoom_to_fit")->trigger();
            QTest::qWait(100);
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
                for (int x = glass == surface ? glassSampleX - 20 : 16; x < (glass == surface ? glassSampleX + 20 : 29); ++x) {
                    const int value = qGray(rendered.pixel(x, 5));
                    low = qMin(low, value); high = qMax(high, value);
                }
                fprintf(stderr, "BLUR: %s stripe contrast=%d\n", glass == surface ? "panel" : "toggle", high - low);
                require(high - low < 15, "glass blurs high-contrast image details");
            }
            QList<QWidget *> opaqueControls{combo};
            QList<QColor> redControlSamples;
            QList<QColor> redSamples;
            stripes.fill(Qt::red);
            pattern->setPixmap(QPixmap::fromImage(stripes));
            QTest::qWait(150);
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)})
                redSamples.append(glass->grab().toImage().pixelColor(glass == surface ? glassSampleX : glass->width() / 2, 5));
            for (auto control : opaqueControls)
                redControlSamples.append(control->grab().toImage().pixelColor(control->width() / 2, 5));
            stripes.fill(Qt::blue);
            pattern->setPixmap(QPixmap::fromImage(stripes));
            QTest::qWait(150);
            int sample = 0;
            for (QWidget *glass : {surface, static_cast<QWidget *>(toggle)}) {
                const auto blue = glass->grab().toImage().pixelColor(glass == surface ? glassSampleX : glass->width() / 2, 5);
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
                QTest::qWait(240);
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
                            const QColor expected = theme.color(group, QPalette::ButtonText);
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
            QTest::qWait(240);
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
            QTest::qWait(100);
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
            require(viewer->mask().isEmpty() && !viewer->graphicsEffect()->isEnabled(), "fullscreen keeps full image area without corner compositing");
            require(!window->findChild<QToolBar *>("mainToolBar")->isVisible()
                    && !mainIsland->isVisible() && !mainToggle->isVisible() && !mainFlyout->isVisible(),
                    "fullscreen hides the main toolbar, flyout, and toggle");
            fullscreenAction->trigger();
            QTest::qWait(400);
            require(viewer->graphicsEffect()->isEnabled() && toggle->isVisible(), "rounded viewer and toggle return after fullscreen");
            require(mainToggle->isVisible() && !mainToggle->isChecked() && !mainFlyout->isVisible(),
                    "main toolbar returns collapsed after fullscreen");
            QTest::mouseClick(mainToggle, Qt::LeftButton);
            QTest::qWait(300);
            require(mainFlyout->isVisible(), "main toolbar opens after leaving fullscreen");
            QTest::mouseClick(mainToggle, Qt::LeftButton);
            QTest::qWait(300);

            for (int i = 0; i < sidebar->count(); ++i) {
                QTest::mouseClick(sidebar->tabBar(), Qt::LeftButton, Qt::NoModifier, sidebar->tabBar()->tabRect(i).center());
                QTest::qWait(240);
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
            require(flyout->viewport()->height() >= controls->height(), "narrow status pills reserve scrollbar room below controls");
            for (auto widget : controls->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
                if (!widget->property("floatingPillGroup").toInt() || widget->isHidden()) continue;
                const QRectF bounds(widget->mapTo(surface, QPoint()), widget->size());
                require(qAbs(bounds.center().y() - 22.0) <= 0.5, "status contents stay centered above the horizontal scrollbar");
            }

            auto statusScroll = flyout->horizontalScrollBar();
            statusScroll->setValue(statusScroll->maximum()); QTest::qWait(80);
            const QPoint zoomPoint = combo->mapTo(floating, combo->rect().center());
            require(floating->rect().contains(zoomPoint) && floating->mask().contains(zoomPoint),
                    "zoom percentage remains reachable at the end of the narrow status strip");
            combo->showPopup(); QTest::qWait(50);
            require(combo->view()->isVisible(), "zoom dropdown opens from a scrolled status pill");
            combo->hidePopup();
            statusScroll->setValue(statusScroll->minimum()); QTest::qWait(80);
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
            require(qAlpha(islandImage.pixel(0, 0)) == 0 && qAlpha(islandImage.pixel(32, 2)) == 255, "full-width rounded thumbnail surface with clear corners");
            capture("viewer-thumbnail-island");
            const auto lastThumbnail = thumbnails->model()->index(5, 0);
            thumbnails->scrollTo(lastThumbnail);
            QTest::qWait(350);
            QTest::mouseClick(thumbnails->viewport(), Qt::LeftButton, Qt::NoModifier, thumbnails->visualRect(lastThumbnail).center());
            QTest::qWait(350);
            require(thumbnails->currentIndex() == lastThumbnail, "thumbnail click selects another image");
            const auto sizesBefore = thumbnailSplitter->sizes();
            thumbnailSplitter->setSizes({sizesBefore[0] - 30, sizesBefore[1] + 30});
            QTest::qWait(240);
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
            verifyHomeSlide(window, output);
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
        action("go_start_page")->trigger(); QTest::qWait(240);
        capture("welcome-recent");
        fprintf(stderr, "PASS: welcome, picker, drag/drop, multi-file navigation, MRU, native edit/save, recent activation\n");
        require(window->close(), "clean close"); QCoreApplication::exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
