// SPDX-License-Identifier: GPL-2.0-or-later
#include "floatingsurface.h"
#include "floatingviewtoolbar.h"
#include <KLocalizedString>
#include <QApplication>
#include <QEvent>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QComboBox>
#include <QGraphicsView>
#include <QImage>
#include <QAction>
#include <QAbstractItemView>
#include <QIconEngine>
#include <QTimer>
#include <QIcon>
#include <QLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVariantAnimation>

namespace Gwenview {
struct ToolbarBackdrop {
    QImage image;
    QRect rect;
    QWidget *viewer;
};
namespace {
constexpr int ButtonSize = 44;
constexpr int EdgeMargin = 16;
constexpr int Gap = 8;
constexpr int PanelHorizontalInset = 8;
constexpr int PanelVerticalInset = 6;

// Three separable box passes approximate a Gaussian. The capture is sampled at
// quarter resolution, so this stays inexpensive even during image animation.
QImage blurBackdrop(QImage image)
{
    constexpr int radius = 2;
    constexpr int samples = 2 * radius + 1;
    for (int pass = 0; pass < 3; ++pass) {
        for (bool horizontal : {true, false}) {
            QImage result(image.size(), image.format());
            for (int y = 0; y < image.height(); ++y) {
                auto out = reinterpret_cast<QRgb *>(result.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    int red = 0, green = 0, blue = 0;
                    for (int offset = -radius; offset <= radius; ++offset) {
                        const QRgb pixel = image.pixel(qBound(0, x + (horizontal ? offset : 0), image.width() - 1),
                                                       qBound(0, y + (horizontal ? 0 : offset), image.height() - 1));
                        red += qRed(pixel);
                        green += qGreen(pixel);
                        blue += qBlue(pixel);
                    }
                    out[x] = qRgb(red / samples, green / samples, blue / samples);
                }
            }
            image = result;
        }
    }
    return image;
}

void paintGlass(QPainter &painter, QWidget *widget, const ToolbarBackdrop &backdrop)
{
    painter.setRenderHint(QPainter::Antialiasing);
    const auto palette = widget->palette();
    const QRectF bounds = QRectF(widget->rect()).adjusted(1, 1, -1, -1);
    const qreal radius = bounds.height() / 2;
    if (!backdrop.image.isNull()) {
        QPainterPath clip;
        clip.addRoundedRect(bounds, radius, radius);
        painter.save();
        painter.setClipPath(clip);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QPoint origin = widget->mapTo(backdrop.viewer, QPoint());
        painter.drawImage(QRectF(backdrop.rect.translated(-origin)), backdrop.image);
        painter.restore();
    }
    QColor tint = palette.color(QPalette::Window);
    const bool dark = tint.lightness() < 128;
    QLinearGradient body(bounds.topLeft(), bounds.bottomLeft());
    tint.setAlpha(dark ? 185 : 170);
    body.setColorAt(0, tint);
    tint.setAlpha(dark ? 155 : 140);
    body.setColorAt(1, tint);
    painter.setPen(QPen(QColor(0, 0, 0, dark ? 95 : 35), 1));
    painter.setBrush(body);
    painter.drawRoundedRect(bounds, radius, radius);

    QLinearGradient sheen(bounds.topLeft(), bounds.bottomLeft());
    sheen.setColorAt(0, QColor(255, 255, 255, dark ? 28 : 55));
    sheen.setColorAt(0.48, QColor(255, 255, 255, 12));
    sheen.setColorAt(0.52, QColor(255, 255, 255, 0));
    sheen.setColorAt(1, QColor(255, 255, 255, dark ? 10 : 32));
    QLinearGradient rim(bounds.topLeft(), bounds.bottomLeft());
    rim.setColorAt(0, QColor(255, 255, 255, dark ? 65 : 130));
    rim.setColorAt(0.5, QColor(255, 255, 255, 30));
    rim.setColorAt(1, QColor(255, 255, 255, dark ? 24 : 65));
    painter.setPen(QPen(QBrush(rim), 1));
    painter.setBrush(sheen);
    painter.drawRoundedRect(bounds.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
}

class ToolbarShadow : public QWidget {
public:
    explicit ToolbarShadow(QWidget *viewer) : QWidget(viewer)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::NoFocus);
        hide();
    }
    QRectF panel;
    QRectF toggle;
    qreal toggleOpacity = 0;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        paintFloatingShadow(painter, panel, 22, palette(), true);
        paintFloatingShadow(painter, toggle, 22, palette(), true, toggleOpacity);
    }
};

class ToolbarSurface : public QWidget {
public:
    ToolbarSurface(QWidget *parent, const QSharedPointer<ToolbarBackdrop> &backdrop)
        : QWidget(parent), mBackdrop(backdrop) {}
private:
    QSharedPointer<ToolbarBackdrop> mBackdrop;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        paintGlass(painter, this, *mBackdrop);
    }
};

class ToolbarButton : public QToolButton {
public:
    ToolbarButton(QWidget *parent, const QSharedPointer<ToolbarBackdrop> &backdrop)
        : QToolButton(parent), mBackdrop(backdrop) {}
private:
    QSharedPointer<ToolbarBackdrop> mBackdrop;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setOpacity(property("revealOpacity").toReal());
        paintGlass(painter, this, *mBackdrop);
        if (isChecked() || underMouse() || hasFocus() || isDown()) {
            QColor tint = palette().color(QPalette::Highlight);
            tint.setAlpha(isDown() ? 70 : isChecked() || hasFocus() ? 45 : 24);
            painter.setPen(Qt::NoPen);
            painter.setBrush(tint);
            painter.drawEllipse(QRectF(rect()).adjusted(2, 2, -2, -2));
        }
        if (hasFocus()) {
            painter.setPen(QPen(palette().color(QPalette::Highlight), 1.5));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QRectF(rect()).adjusted(3, 3, -3, -3));
        }
        icon().paint(&painter, rect().adjusted(11, 11, -11, -11), Qt::AlignCenter,
                     isEnabled() ? QIcon::Normal : QIcon::Disabled, isChecked() ? QIcon::On : QIcon::Off);
    }
};
// Toolbar action icons are monochrome. Keep the theme's glyphs, but render
// their alpha masks in the current text color instead of caching old SVG colors.
class ToolbarIconEngine : public QIconEngine {
public:
    ToolbarIconEngine(const QIcon &source, const QPalette &palette, bool accentWhenChecked)
        : mSource(source), mPalette(palette), mAccentWhenChecked(accentWhenChecked) {}
    QIconEngine *clone() const override { return new ToolbarIconEngine(mSource, mPalette, mAccentWhenChecked); }
    QString iconName() override { return mSource.name(); }
    bool isNull() override { return mSource.isNull(); }
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return mSource.actualSize(size, mode, state); }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return scaledPixmap(size, mode, state, 1); }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override
    {
        QPixmap result = mSource.pixmap(size, scale, QIcon::Normal, state);
        if (result.isNull()) return result;
        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const auto role = mAccentWhenChecked && state == QIcon::On ? QPalette::HighlightedText : QPalette::ButtonText;
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(result.rect(), mPalette.color(group, role));
        return result;
    }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        const QPixmap image = scaledPixmap(rect.size(), mode, state, painter->device()->devicePixelRatioF());
        painter->drawPixmap(rect, image);
    }
private:
    QIcon mSource;
    QPalette mPalette;
    bool mAccentWhenChecked;
};

QColor mixedColor(const QColor &background, const QColor &foreground, qreal amount)
{
    return QColor::fromRgbF(background.redF() * (1 - amount) + foreground.redF() * amount,
                           background.greenF() * (1 - amount) + foreground.greenF() * amount,
                           background.blueF() * (1 - amount) + foreground.blueF() * amount);
}

}

FloatingViewToolbar::FloatingViewToolbar(QWidget *controls, QWidget *viewer, Position position)
    : QWidget(viewer), mViewer(viewer), mControls(controls), mPosition(position)
{
    setObjectName(position == TopRight ? QStringLiteral("floatingMainToolbar") : QStringLiteral("floatingImageToolbar"));
    mBackdrop = QSharedPointer<ToolbarBackdrop>::create();
    mBackdrop->viewer = viewer;
    mBackdropTimer = new QTimer(this);
    mBackdropTimer->setSingleShot(true);
    mBackdropTimer->setInterval(16);
    connect(mBackdropTimer, &QTimer::timeout, this, &FloatingViewToolbar::refreshBackdrop);
    if (auto view = qobject_cast<QGraphicsView *>(viewer)) view->viewport()->installEventFilter(this);
    // The clipped scroll area lets every native control remain reachable even
    // when the image pane is too narrow to fit the entire toolbar in one row.
    mSurface = new ToolbarSurface(this, mBackdrop);
    mSurface->hide();
    mPanel = new QScrollArea(mSurface);
    mPanel->setObjectName(position == TopRight ? QStringLiteral("mainToolbarFlyout") : QStringLiteral("imageToolbarFlyout"));
    mPanel->setFrameShape(QFrame::NoFrame);
    mPanel->setWidgetResizable(false);
    mPanel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mPanel->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mPanel->setWidget(controls);
    mPanel->setAlignment(Qt::AlignVCenter);
    mPanel->viewport()->setBackgroundRole(QPalette::Window);
    mControls->setBackgroundRole(QPalette::Window);
    mNativeStyleSheet = mControls->styleSheet();
    for (auto layout : mControls->findChildren<QLayout *>()) {
        mNativeLayoutSpacing.insert(layout, layout->spacing());
        layout->setSpacing(6);
    }
    mPanel->hide();
    mToggle = new ToolbarButton(this, mBackdrop);
    mToggle->setObjectName(position == TopRight ? QStringLiteral("mainToolbarToggle") : QStringLiteral("imageToolbarToggle"));
    mToggle->setIcon(QIcon::fromTheme(QStringLiteral("configure-toolbars"), QIcon::fromTheme(QStringLiteral("view-more-symbolic"))));
    mToggle->setCheckable(true);
    mToggle->setFixedSize(ButtonSize, ButtonSize);
    mToggle->setToolTip(position == TopRight ? i18n("Show main toolbar") : i18n("Show image toolbar"));
    mToggle->setAccessibleName(mToggle->toolTip());
    mToggle->setProperty("revealOpacity", 0.0);
    // A separate, input-transparent sibling lets shadows extend beyond the
    // clipped sliding panel without changing any control or image hit areas.
    mShadow = new ToolbarShadow(viewer);
    mShadow->setObjectName(position == TopRight ? QStringLiteral("mainToolbarShadow") : QStringLiteral("imageToolbarShadow"));
    mRevealAnimation = new QVariantAnimation(this);
    mRevealAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(mRevealAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        mToggle->setProperty("revealOpacity", value);
        mToggle->update();
        updateShadow();
        // The invisible collapsed island must not intercept image gestures.
        setAttribute(Qt::WA_TransparentForMouseEvents, value.toReal() == 0 && mProgress == 0 && !mToggle->isChecked());
    });
    viewer->setMouseTracking(true);
    if (auto view = qobject_cast<QGraphicsView *>(viewer)) view->viewport()->setMouseTracking(true);
    mAnimation = new QVariantAnimation(this);
    mAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        mProgress = value.toReal();
        updateGeometryAndMask();
    });
    connect(mToggle, &QToolButton::toggled, this, [this](bool expanded) {
        const qreal start = mProgress;
        const qreal end = expanded ? 1.0 : 0.0;
        mAnimation->stop();
        const auto label = mPosition == TopRight
            ? (expanded ? i18n("Hide main toolbar") : i18n("Show main toolbar"))
            : (expanded ? i18n("Hide image toolbar") : i18n("Show image toolbar"));
        mToggle->setToolTip(label);
        mToggle->setAccessibleName(label);
        if (!expanded && mPanel->isAncestorOf(QApplication::focusWidget())) mToggle->setFocus();
        const bool animate = style()->styleHint(QStyle::SH_Widget_Animation_Duration) > 0;
        {
            // A finished animation retains its end time. Reconfiguring its
            // values can emit valueChanged before start() resets that time.
            // Preserve the actual visible position, including during reversal.
            const QSignalBlocker blocker(mAnimation);
            mAnimation->setStartValue(start);
            mAnimation->setEndValue(end);
            mAnimation->setDuration(animate ? qRound(220 * qAbs(end - start)) : 0);
            mAnimation->setCurrentTime(0);
        }
        mAnimation->start();
        // Zero-duration animations may already hold their end value after
        // setup, so also apply it explicitly when motion is disabled.
        if (mAnimation->duration() == 0) {
            mProgress = end;
            updateGeometryAndMask();
        }
    });
    mThemeTimer = new QTimer(this);
    mThemeTimer->setSingleShot(true);
    connect(mThemeTimer, &QTimer::timeout, this, &FloatingViewToolbar::refreshTheme);
    // ApplicationPaletteChange reaches top-level windows even when a child has
    // an explicit palette, so listen at the application instead of the viewer.
    qApp->installEventFilter(this);
    refreshTheme();
    viewer->installEventFilter(this);
    controls->installEventFilter(this);
    updateGeometryAndMask();
}

FloatingViewToolbar::~FloatingViewToolbar()
{
    delete mShadow.data();
}

void FloatingViewToolbar::updateShadow()
{
    if (!mShadow) return;
    auto shadow = static_cast<ToolbarShadow *>(mShadow.data());
    const QPointF padding(16, 16);
    shadow->setGeometry(geometry().adjusted(-16, -16, 16, 16));
    shadow->toggle = QRectF(mToggle->geometry()).adjusted(1, 1, -1, -1).translated(padding);
    shadow->panel = mProgress > 0
        ? QRectF(mSurface->geometry().intersected(rect())).adjusted(1, 1, -1, -1).translated(padding) : QRectF();
    shadow->toggleOpacity = mToggle->property("revealOpacity").toReal();
    shadow->setVisible(isVisible() && !mControlsDocked && (mProgress > 0 || shadow->toggleOpacity > 0));
    shadow->stackUnder(this);
    shadow->update();
}

void FloatingViewToolbar::setFullScreen(bool fullScreen)
{
    mFullScreen = fullScreen;
    updateGeometryAndMask();
}

void FloatingViewToolbar::collapse()
{
    mToggle->setChecked(false);
    mAnimation->stop();
    mProgress = 0;
    updateGeometryAndMask();
}

void FloatingViewToolbar::setControlsDocked(QWidget *parent)
{
    const bool docked = parent != nullptr;
    if (mControlsDocked == docked) return;
    mControlsDocked = docked;
    collapse();
    if (docked) {
        hide();
        mThemeTimer->stop();
        mPanel->takeWidget();
        mControls->setParent(parent);
        mControls->setStyleSheet(mNativeStyleSheet);
        mControls->setPalette(QPalette());
        for (auto child : mControls->findChildren<QWidget *>()) child->setPalette(QPalette());
        for (auto combo : mControls->findChildren<QComboBox *>()) {
            combo->view()->setPalette(QPalette());
            combo->view()->window()->setPalette(QPalette());
        }
        for (auto button : mControls->findChildren<QToolButton *>()) {
            QIcon original = button->defaultAction() ? button->defaultAction()->icon() : mOriginalIcons.value(button, button->icon());
            if (!original.name().isEmpty()) original = QIcon::fromTheme(original.name(), original);
            button->setIcon(original);
        }
        mOriginalIcons.clear();
        mThemedIconKeys.clear();
        for (auto it = mNativeLayoutSpacing.cbegin(); it != mNativeLayoutSpacing.cend(); ++it)
            it.key()->setSpacing(it.value());
        parent->layout()->addWidget(mControls);
        mControls->show();
    } else {
        mControls->parentWidget()->layout()->removeWidget(mControls);
        mPanel->setWidget(mControls);
        for (auto layout : mControls->findChildren<QLayout *>()) layout->setSpacing(6);
        refreshTheme();
        mControls->show();
        show();
    }
}

void FloatingViewToolbar::updatePointerProximity(const QPoint &globalPosition)
{
    const QPoint inViewer = mViewer->mapFromGlobal(globalPosition);
    const QRect buttonRect(mToggle->mapTo(mViewer, QPoint()), mToggle->size());
    // Reveal before the pointer reaches the 44px target; a slightly wider exit
    // boundary prevents flicker when moving along the edge of the reveal area.
    const int margin = mPointerNear ? 64 : 48;
    mPointerNear = isVisible() && mViewer->rect().contains(inViewer)
        && buttonRect.adjusted(-margin, -margin, margin, margin).contains(inViewer);
    updateToggleReveal();
}

void FloatingViewToolbar::updateToggleReveal()
{
    const bool reveal = mToggle->isChecked() || mProgress > 0 || mPointerNear || mKeyboardFocus;
    const qreal target = reveal ? 1.0 : 0.0;
    const qreal current = mToggle->property("revealOpacity").toReal();
    if (mRevealAnimation->state() == QAbstractAnimation::Running && mRevealAnimation->endValue().toReal() == target) return;
    mRevealAnimation->stop();
    setAttribute(Qt::WA_TransparentForMouseEvents, !reveal && current == 0);
    if (current == target) return;
    if (!isVisible() || style()->styleHint(QStyle::SH_Widget_Animation_Duration) == 0) {
        mToggle->setProperty("revealOpacity", target);
        mToggle->update();
        updateShadow();
        setAttribute(Qt::WA_TransparentForMouseEvents, !reveal);
        return;
    }
    {
        const QSignalBlocker blocker(mRevealAnimation);
        mRevealAnimation->setStartValue(current);
        mRevealAnimation->setEndValue(target);
        mRevealAnimation->setDuration(qMax(1, qRound(180 * qAbs(target - current))));
        mRevealAnimation->setCurrentTime(0);
    }
    mRevealAnimation->start();
}

bool FloatingViewToolbar::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        auto widget = qobject_cast<QWidget *>(object);
        if (widget && widget->window() == window())
            updatePointerProximity(static_cast<QMouseEvent *>(event)->globalPosition().toPoint());
    } else if (event->type() == QEvent::Enter) {
        auto widget = qobject_cast<QWidget *>(object);
        if (widget && widget->window() == window())
            updatePointerProximity(static_cast<QEnterEvent *>(event)->globalPosition().toPoint());
    } else if ((event->type() == QEvent::Leave && (object == mViewer || object == window()))
               || (event->type() == QEvent::Hide && object == this)) {
        mPointerNear = false;
        updateToggleReveal();
        updateShadow();
    } else if (event->type() == QEvent::Show && object == this) {
        updateShadow();
        QTimer::singleShot(0, this, [this] { updatePointerProximity(QCursor::pos()); });
    } else if (object == mToggle && event->type() == QEvent::FocusIn) {
        const auto reason = static_cast<QFocusEvent *>(event)->reason();
        mKeyboardFocus = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason || reason == Qt::ShortcutFocusReason;
        updateToggleReveal();
    } else if (object == mToggle && event->type() == QEvent::FocusOut) {
        mKeyboardFocus = false;
        updateToggleReveal();
    }
    // Some controls (notably the sidebar toggle) replace their icons directly
    // rather than through an action. Recolor the new glyph on the next turn.
    if (event->type() == QEvent::Paint) {
        if (auto button = qobject_cast<QToolButton *>(object); button && isAncestorOf(button)
            && button->icon().cacheKey() != mThemedIconKeys.value(button) && !mThemeTimer->isActive()) mThemeTimer->start();
    }
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ApplicationFontChange
        || (object == window() && (event->type() == QEvent::ThemeChange || event->type() == QEvent::StyleChange))
        || (event->type() == QEvent::ActionChanged && qobject_cast<QToolButton *>(object)
            && isAncestorOf(static_cast<QWidget *>(object)))) {
        // Defer until Qt and the platform theme finish updating their palettes
        // and actions. Repeated notifications coalesce into one refresh.
        if (!mThemeTimer->isActive()) mThemeTimer->start();
    }
    auto view = qobject_cast<QGraphicsView *>(mViewer);
    // Capture on the next event turn, never recursively from a viewport paint.
    if (view && object == view->viewport() && event->type() == QEvent::Paint
        && isVisible() && !mBackdropTimer->isActive()) mBackdropTimer->start();
    if ((object == mViewer && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        || (object == mControls && event->type() == QEvent::LayoutRequest)
        || (object == mControls && event->type() == QEvent::StyleChange)) {
        updateGeometryAndMask();
        if (object == mViewer && event->type() == QEvent::Resize) updatePointerProximity(QCursor::pos());
    }
    return QWidget::eventFilter(object, event);
}

void FloatingViewToolbar::updateGeometryAndMask()
{
    // The bottom island owns the viewer mask; the top copy only positions itself.
    if (mPosition == BottomRight) {
        if (mFullScreen) {
            mViewer->clearMask();
        } else {
            QPainterPath outline;
            outline.addRoundedRect(QRectF(mViewer->rect()), 16, 16);
            mViewer->setMask(QRegion(outline.toFillPolygon().toPolygon()));
        }
    }
    // The native layout owns control size and styling while docked.
    if (mControlsDocked) return;
    mControls->ensurePolished();
    // These controls were created without a parent. A translucent application
    // style can leave window flags on them after they become embedded, making
    // Qt skip the parent glass surface when composing them.
    mControls->setAttribute(Qt::WA_TranslucentBackground, false);
    mControls->setAttribute(Qt::WA_NoSystemBackground, false);
    mControls->setAutoFillBackground(false);
    const QSize wanted = mControls->sizeHint().expandedTo(mControls->minimumSizeHint());
    const int available = qMax(1, mViewer->width() - 2 * EdgeMargin - ButtonSize - Gap);
    const int fullWidth = qMin(wanted.width() + 2 * PanelHorizontalInset, available);
    const int height = ButtonSize;
    const int top = mPosition == TopRight ? EdgeMargin : mViewer->height() - EdgeMargin - height;
    mCaptureRect = QRect(mViewer->width() - EdgeMargin - fullWidth - Gap - ButtonSize,
                         top, fullWidth + Gap + ButtonSize, height).adjusted(-24, -24, 24, 24);
    if (mCaptureRect != mBackdrop->rect && !mBackdropTimer->isActive()) mBackdropTimer->start();
    mControls->resize(wanted.width(), wanted.height());
    const int revealed = qRound(fullWidth * mProgress);
    const int width = ButtonSize + (revealed > 0 ? Gap + revealed : 0);
    setGeometry(qMax(0, mViewer->width() - EdgeMargin - width), qMax(0, top), width, height);
    mToggle->move(width - ButtonSize, (height - ButtonSize) / 2);
    // Move the full panel leftward as it emerges from behind the fixed button.
    mSurface->setGeometry(revealed - fullWidth, 0, fullWidth, height);
    mPanel->setGeometry(PanelHorizontalInset, PanelVerticalInset,
                        qMax(1, fullWidth - 2 * PanelHorizontalInset), height - 2 * PanelVerticalInset);
    mSurface->setVisible(revealed > 0);
    mPanel->setVisible(revealed > 0);
    // Disabling the controls on every frame also triggers the theme's fading
    // animation. Block pointer input during motion without changing their look.
    mSurface->setAttribute(Qt::WA_TransparentForMouseEvents, mProgress < 1.0);
    mToggle->raise();
    raise();
    updateToggleReveal();
    updateShadow();
}

void FloatingViewToolbar::refreshTheme()
{
    if (mControlsDocked) return;
    // Keep image-background choices separate from the system control palette.
    const QPalette system = qApp->palette();
    setPalette(system);
    mSurface->setPalette(system);
    mToggle->setPalette(system);
    mShadow->setPalette(system);
    QPalette controls = system;
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        for (auto role : {QPalette::Window, QPalette::Base, QPalette::Button, QPalette::Highlight}) {
            QColor color = controls.color(group, role);
            color.setAlpha(255);
            controls.setColor(group, role, color);
        }
    }
    mControls->setPalette(controls);
    // Concrete colors avoid stale QStyleSheetStyle palette caches after a
    // running application switches between light and dark system schemes.
    const QColor foreground = controls.color(QPalette::ButtonText);
    const QColor background = controls.color(QPalette::Button);
    const QString css = QStringLiteral(R"(
        QToolBar { background: transparent; border: none; padding: 0px; spacing: 6px; }
        QToolBar::separator { background: %1; width: 1px; margin: 6px 2px; }
        QToolButton {
            min-height: 26px; max-height: 26px;
            border: 1px solid %1; border-radius: 14px;
            background: %2; padding: 0px 9px; color: %3;
        }
        QToolButton::menu-indicator, QToolButton::menu-arrow {
            image: none; width: 0px; height: 0px;
        }
        QToolButton:hover { background: %4; border-color: %3; }
        QToolButton:checked, QToolButton:pressed {
            background: %5; border-color: %5; color: %6;
        }
        QToolButton:focus { border-color: %5; }
        QToolButton:disabled { color: %7; background: %8; border-color: %1; }
        QSlider::groove:horizontal { height: 4px; border-radius: 2px; background: %1; }
        QSlider::sub-page:horizontal { border-radius: 2px; background: %5; }
        QSlider::handle:horizontal {
            width: 12px; margin: -5px 0; border-radius: 7px;
            border: 1px solid %1; background: %9;
        }
        QSlider::handle:horizontal:focus { border-color: %5; }
    )").arg(mixedColor(background, foreground, 0.22).name(),
           mixedColor(background, foreground, 0.07).name(), foreground.name(),
           mixedColor(background, foreground, 0.22).name(),
           controls.color(QPalette::Highlight).name(), controls.color(QPalette::HighlightedText).name(),
           controls.color(QPalette::Disabled, QPalette::ButtonText).name(),
           mixedColor(background, foreground, 0.05).name(), controls.color(QPalette::Base).name());
    if (mControls->styleSheet() != css) mControls->setStyleSheet(css);
    // No combo-box stylesheet or custom arrow: the field and popup are drawn
    // by the system style, with opaque theme surfaces and current text colors.
    for (auto child : mControls->findChildren<QWidget *>()) child->setPalette(controls);
    for (auto combo : mControls->findChildren<QComboBox *>()) {
        combo->view()->setPalette(controls);
        combo->view()->window()->setPalette(controls);
    }
    for (auto button : findChildren<QToolButton *>()) {
        QIcon source;
        if (button->defaultAction()) {
            source = button->defaultAction()->icon();
        } else {
            if (!mOriginalIcons.contains(button) || button->icon().cacheKey() != mThemedIconKeys.value(button))
                mOriginalIcons.insert(button, button->icon());
            source = mOriginalIcons.value(button);
        }
        if (!source.name().isEmpty()) source = QIcon::fromTheme(source.name(), source);
        if (!source.isNull()) {
            button->setIcon(QIcon(new ToolbarIconEngine(source, controls, button != mToggle)));
            mThemedIconKeys.insert(button, button->icon().cacheKey());
        }
    }
    mPanel->setAutoFillBackground(false);
    mPanel->viewport()->setAutoFillBackground(false);
    mControls->setAutoFillBackground(false);
    updateGeometryAndMask();
    mSurface->update();
    mToggle->update();
}

void FloatingViewToolbar::refreshBackdrop()
{
    auto view = qobject_cast<QGraphicsView *>(mViewer);
    if (!view || !isVisible() || mCaptureRect.isEmpty()) return;
    const QRect capture = mCaptureRect.intersected(QRect(view->viewport()->mapTo(mViewer, QPoint()), view->viewport()->size()));
    if (capture.isEmpty()) return;
    QImage image(QSize(qMax(1, capture.width() / 4), qMax(1, capture.height() / 4)), QImage::Format_RGB32);
    image.fill(view->palette().color(QPalette::Base));
    {
        QPainter painter(&image);
        // QGraphicsView::render draws only the image scene, excluding our
        // sibling QWidget overlay. Captures cannot feed the toolbar back into itself.
        view->render(&painter, QRectF(image.rect()), capture.translated(-view->viewport()->pos()), Qt::IgnoreAspectRatio);
    }
    image = blurBackdrop(image);
    // A toolbar repaint can also repaint the viewport beneath it. Comparing
    // the cache stops that from becoming a continuous capture/repaint loop.
    if (mBackdrop->rect == capture && mBackdrop->image == image) return;
    mBackdrop->rect = capture;
    mBackdrop->image = image;
    mSurface->update();
    mToggle->update();
}

}
