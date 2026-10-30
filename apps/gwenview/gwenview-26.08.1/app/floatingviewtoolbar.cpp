#include <lib/sequoiadesign.h>
// SPDX-License-Identifier: GPL-2.0-or-later
#include "floatingsurface.h"
#include "backdropblur.h"
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
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QToolBar>
#include <QScrollBar>
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
constexpr int Gap = 6;
constexpr int PanelHorizontalInset = 8;
constexpr int PanelVerticalInset = 6;

void paintGlass(QPainter &painter, QWidget *widget, const ToolbarBackdrop &backdrop, QRectF bounds = QRectF())
{
    painter.setRenderHint(QPainter::Antialiasing);
    if (bounds.isEmpty()) bounds = QRectF(widget->rect()).adjusted(1, 1, -1, -1);
    const QPoint origin = widget->mapTo(backdrop.viewer, QPoint());
    paintPillSurface(painter, bounds, bounds.height() / 2, widget->palette(),
                     backdrop.image, QRectF(backdrop.rect.translated(-origin)), true);
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
    QList<QRectF> pills;
    QRectF toggle;
    qreal toggleOpacity = 0;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const qreal weight = palette().color(QPalette::Window).lightness() < 128 ? 0.8 : 0.6;
        paintFloatingShadow(painter, panel, 22, palette(), true, weight);
        for (const QRectF &pill : pills) paintFloatingShadow(painter, pill, 22, palette(), true, weight);
        paintFloatingShadow(painter, toggle, 22, palette(), true, toggleOpacity * weight);
        const qreal progress = property("rippleProgress").toReal();
        if (progress > 0 && progress < 1) {
            painter.setRenderHint(QPainter::Antialiasing);
            QColor accent = palette().color(QPalette::Highlight);
            accent.setAlphaF(0.9 * (1 - progress * progress) * toggleOpacity);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(accent, 2.0));
            const qreal radius = 6 + progress * 30;
            painter.drawEllipse(toggle.center(), radius, radius);
        }
    }
};

class ToolbarSurface : public QWidget {
public:
    ToolbarSurface(QWidget *parent, const QSharedPointer<ToolbarBackdrop> &backdrop)
        : QWidget(parent), mBackdrop(backdrop) {}
    bool grouped = false;
    QList<QRectF> pills;
private:
    QSharedPointer<ToolbarBackdrop> mBackdrop;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        if (grouped) {
            for (const QRectF &pill : pills) paintGlass(painter, this, *mBackdrop, pill);
        } else {
            paintGlass(painter, this, *mBackdrop);
        }
        for (auto combo : findChildren<QComboBox *>()) {
            if (combo->isVisible() && combo->property("floatingZoomPill").toBool()) {
                const QRectF field(combo->mapTo(this, QPoint()), combo->size());
                paintFloatingShadow(painter, field, field.height() / 2, palette(), false, 1.5);
            }
        }
    }
};

class ToolbarButton : public QToolButton {
public:
    ToolbarButton(QWidget *parent, const QSharedPointer<ToolbarBackdrop> &backdrop)
        : QToolButton(parent), mBackdrop(backdrop), mRipple(new QVariantAnimation(this))
    {
        mRipple->setObjectName(QStringLiteral("toolbarToggleRipple"));
        mRipple->setDuration(320);
        mRipple->setStartValue(0.0);
        mRipple->setEndValue(1.0);
        mRipple->setEasingCurve(QEasingCurve::Linear);
        connect(mRipple, &QVariantAnimation::valueChanged, this, [this] { update(); });
        connect(this, &QToolButton::pressed, this, [this] {
            mRipple->stop();
            if (style()->styleHint(QStyle::SH_Widget_Animation_Duration) != 0) mRipple->start();
            update();
        });
    }
private:
    QSharedPointer<ToolbarBackdrop> mBackdrop;
    QVariantAnimation *mRipple;
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setOpacity(property("revealOpacity").toReal());
        paintGlass(painter, this, *mBackdrop);
        if (isChecked() || underMouse() || hasFocus() || isDown()) {
            QColor tint = palette().color(QPalette::ButtonText);
            tint.setAlpha(underMouse() ? 16 : 10);
            painter.setPen(Qt::NoPen);
            painter.setBrush(tint);
            painter.drawEllipse(QRectF(rect()).adjusted(2, 2, -2, -2));
        }
        if (mRipple->state() == QAbstractAnimation::Running) {
            const qreal progress = mRipple->currentValue().toReal();
            painter.save();
            QPainterPath clip;
            clip.addEllipse(QRectF(rect()).adjusted(2, 2, -2, -2));
            painter.setClipPath(clip);
            QColor accent = palette().color(QPalette::Highlight);
            painter.setBrush(Qt::NoBrush);
            accent.setAlphaF(0.9 * (1 - progress * progress));
            painter.setPen(QPen(accent, 2.0));
            const qreal radius = 6 + progress * 30;
            painter.drawEllipse(QRectF(rect()).center(), radius, radius);
            painter.restore();
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
    ToolbarIconEngine(const QIcon &source, const QPalette &palette)
        : mSource(source), mPalette(palette) {}
    QIconEngine *clone() const override { return new ToolbarIconEngine(mSource, mPalette); }
    QString iconName() override { return mSource.name(); }
    bool isNull() override { return mSource.isNull(); }
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return mSource.actualSize(size, mode, state); }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override { return scaledPixmap(size, mode, state, 1); }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override
    {
        QPixmap result = mSource.pixmap(size, scale, QIcon::Normal, state);
        if (result.isNull()) return result;
        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const auto role = QPalette::ButtonText;
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
    static_cast<ToolbarSurface *>(mSurface)->grouped = true;
    mSurface->hide();
    mPanel = new QScrollArea(mSurface);
    mPanel->setObjectName(position == TopRight ? QStringLiteral("mainToolbarFlyout") : QStringLiteral("imageToolbarFlyout"));
    mPanel->setFrameShape(QFrame::NoFrame);
    mPanel->setWidgetResizable(false);
    mPanel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mPanel->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mPanel->setWidget(controls);
    connect(mPanel->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        updatePillGeometry(); updateShadow();
    });
    mPanel->setAlignment(Qt::AlignVCenter);
    mPanel->viewport()->setBackgroundRole(QPalette::Window);
    mControls->setBackgroundRole(QPalette::Window);
    mNativeStyleSheet = mControls->styleSheet();
    for (auto button : controls->findChildren<QToolButton *>()) {
        if (button->property("floatingPillGroup").toInt() == 1)
            mNativeButtonMetrics.insert(button, {button->toolButtonStyle(), button->iconSize()});
    }
    for (auto layout : mControls->findChildren<QLayout *>()) {
        mNativeLayoutSpacing.insert(layout, layout->spacing());
        layout->setSpacing(6);
    }
    // The status bar already separates its logical groups with stretch items.
    // Reserve actual air between the floating pills, then restore those items
    // exactly when KDE docks the original controls again.
    if (position == BottomRight && controls->layout()) {
        for (int i = 0; i < controls->layout()->count(); ++i) {
            if (auto spacer = controls->layout()->itemAt(i)->spacerItem()) {
                mNativeSpacers.insert(spacer, {spacer->sizeHint(), spacer->sizePolicy()});
                spacer->changeSize(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            }
        }
        controls->layout()->invalidate();
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
    auto ripple = mToggle->findChild<QVariantAnimation *>(QStringLiteral("toolbarToggleRipple"));
    connect(ripple, &QVariantAnimation::valueChanged, mShadow, [this](const QVariant &progress) {
        mShadow->setProperty("rippleProgress", progress);
        mShadow->update();
    });
    connect(ripple, &QVariantAnimation::stateChanged, mShadow, [this](QAbstractAnimation::State state) {
        if (state == QAbstractAnimation::Stopped) { mShadow->setProperty("rippleProgress", 1.0); mShadow->update(); }
    });
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
    mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
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
            mAnimation->setDuration(animate ? qRound(Sequoia::LongDuration * qAbs(end - start)) : 0);
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
    connect(mThemeTimer, &QTimer::timeout, this, [this] {
        if (mThemeDirty) refreshTheme();
        else refreshIcons();
    });
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
    shadow->pills.clear();
    {
        shadow->panel = QRectF();
        for (const QRectF &pill : static_cast<ToolbarSurface *>(mSurface)->pills) {
            const QRectF visible = pill.translated(mSurface->pos()).intersected(QRectF(rect()));
            if (!visible.isEmpty()) shadow->pills.append(visible.translated(padding));
        }
    }
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
            combo->setProperty("floatingZoomPill", false);
            combo->update();
            combo->view()->setPalette(QPalette());
            combo->view()->window()->setPalette(QPalette());
        }
        for (auto button : mControls->findChildren<QToolButton *>()) {
            QIcon original = button->defaultAction() ? button->defaultAction()->icon() : mOriginalIcons.value(button, button->icon());
            if (!original.name().isEmpty()) original = QIcon::fromTheme(original.name(), original);
            button->setIcon(original);
        }
        for (auto it = mNativeButtonMetrics.cbegin(); it != mNativeButtonMetrics.cend(); ++it) {
            it.key()->setToolButtonStyle(it.value().first);
            it.key()->setIconSize(it.value().second);
        }
        mOriginalIcons.clear();
        mThemedIconKeys.clear();
        mThemedIcons.clear();
        for (auto it = mNativeLayoutSpacing.cbegin(); it != mNativeLayoutSpacing.cend(); ++it)
            it.key()->setSpacing(it.value());
        for (auto it = mNativeSpacers.cbegin(); it != mNativeSpacers.cend(); ++it)
            it.key()->changeSize(it.value().first.width(), it.value().first.height(),
                                 it.value().second.horizontalPolicy(), it.value().second.verticalPolicy());
        mControls->layout()->invalidate();
        parent->layout()->addWidget(mControls);
        mControls->show();
    } else {
        mControls->parentWidget()->layout()->removeWidget(mControls);
        mPanel->setWidget(mControls);
        for (auto layout : mControls->findChildren<QLayout *>()) layout->setSpacing(6);
        for (auto spacer : mNativeSpacers.keys()) spacer->changeSize(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        mControls->layout()->invalidate();
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
        mRevealAnimation->setDuration(qMax(1, qRound(Sequoia::LongDuration * qAbs(target - current))));
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
            && isAncestorOf(static_cast<QWidget *>(object))
            && static_cast<QToolButton *>(object)->icon().cacheKey() != mThemedIconKeys.value(static_cast<QToolButton *>(object)))) {
        // Defer until Qt and the platform theme finish updating their palettes
        // and actions. Repeated notifications coalesce into one refresh.
        if (event->type() != QEvent::ActionChanged) mThemeDirty = true;
        if (!mThemeTimer->isActive()) mThemeTimer->start();
    }
    auto view = qobject_cast<QGraphicsView *>(mViewer);
    // Capture on the next event turn, never recursively from a viewport paint.
    if (view && object == view->viewport() && event->type() == QEvent::Paint
        && needsBackdrop() && !mBackdropTimer->isActive()) mBackdropTimer->start();
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
    const bool overflow = wanted.width() + 2 * PanelHorizontalInset > available;
    const int height = ButtonSize + (overflow ? mPanel->horizontalScrollBar()->sizeHint().height() : 0);
    const int top = mPosition == TopRight ? EdgeMargin : mViewer->height() - EdgeMargin - height;
    mCaptureRect = QRect(mViewer->width() - EdgeMargin - fullWidth - Gap - ButtonSize,
                         top, fullWidth + Gap + ButtonSize, height).adjusted(-24, -24, 24, 24);
    if (needsBackdrop() && mCaptureRect != mBackdrop->rect && !mBackdropTimer->isActive()) mBackdropTimer->start();
    mControls->resize(wanted.width(), wanted.height());
    const int revealed = qRound(fullWidth * mProgress);
    const int width = ButtonSize + (revealed > 0 ? Gap + revealed : 0);
    setGeometry(qMax(0, mViewer->width() - EdgeMargin - width), qMax(0, top), width, height);
    mToggle->move(width - ButtonSize, 0);
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
    updatePillGeometry();
    updateShadow();
}

void FloatingViewToolbar::updatePillGeometry()
{
    auto toolbar = qobject_cast<QToolBar *>(mControls);
    auto surface = static_cast<ToolbarSurface *>(mSurface);
    QList<QRectF> pills;
    QRect group;
    if (mControls->layout()) mControls->layout()->activate();
    const QPoint origin = mControls->mapTo(mSurface, QPoint());
    auto append = [&] {
        if (group.isEmpty()) return;
        QRectF pill(group.translated(origin));
        pill.setTop(1); pill.setHeight(ButtonSize - 2);
        pill.adjust(-6, 0, 6, 0);
        pill = pill.intersected(QRectF(mSurface->rect()));
        if (!pill.isEmpty()) pills.append(pill);
        group = QRect();
    };
    if (toolbar) {
        for (auto action : toolbar->actions()) {
            if (action->isSeparator()) { append(); continue; }
            if (!action->isVisible()) continue;
            if (auto widget = toolbar->widgetForAction(action); widget && !widget->isHidden())
                group = group.united(toolbar->actionGeometry(action));
        }
    } else if (auto layout = mControls->layout()) {
        int previousGroup = -1;
        for (int i = 0; i < layout->count(); ++i) {
            auto widget = layout->itemAt(i)->widget();
            if (!widget || widget->isHidden()) continue;
            const int family = widget->property("floatingPillGroup").toInt();
            if (previousGroup != -1 && family != previousGroup) append();
            group = group.united(widget->geometry());
            previousGroup = family;
        }
    }
    append();
    if (surface->pills != pills) { surface->pills = pills; surface->update(); }
    // Separate pills should also behave as separate floating hit areas.
    QPainterPath hitAreas;
    // The mask is for input routing only. Leave a small transparent fringe
    // around painted curves so its integer polygon cannot cut off antialiasing.
    hitAreas.addEllipse(QRectF(mToggle->geometry()).adjusted(-2, -2, 2, 2));
    for (const QRectF &pill : pills)
        hitAreas.addRoundedRect(pill.translated(mSurface->pos()).adjusted(-2, -2, 2, 2), 23, 23);
    auto scroll = mPanel->horizontalScrollBar();
    if (scroll->isVisible()) hitAreas.addRect(QRectF(scroll->mapTo(this, QPoint()), scroll->size()));
    const QRegion region = QRegion(hitAreas.toFillPolygon().toPolygon()).intersected(rect());
    if (mask() != region) setMask(region);
}

void FloatingViewToolbar::refreshTheme()
{
    if (mControlsDocked) return;
    mThemeDirty = false;
    // Keep image-background choices separate from the system control palette.
    for (auto button : mNativeButtonMetrics.keys()) {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIconSize(QSize(22, 22));
    }
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
    QString css = QStringLiteral(R"(
        QToolBar { background: transparent; border: none; padding: 0px; spacing: 6px; }
        QToolBar::separator { background: %1; width: 1px; margin: 6px 2px; }
        QToolButton {
            min-height: 28px; max-height: 28px;
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
    {
        css += QStringLiteral(R"(
            QToolBar { spacing: 3px; }
            QToolBar::separator { background: transparent; width: 16px; margin: 0px; }
            QToolButton { background: transparent; border: 1px solid transparent; border-radius: 15px; padding: 0px 8px; }
            QToolButton:hover { background: %1; border-color: transparent; }
            QToolButton:checked, QToolButton:pressed { background: %2; border-color: %2; color: %3; }
            QToolButton:focus { border-color: %2; }
            QToolButton:disabled { background: transparent; border-color: transparent; }
            QComboBox QLineEdit { background: transparent; border: none; }
        )").arg(mixedColor(background, foreground, 0.08).name(), mixedColor(background, controls.color(QPalette::Highlight), 0.16).name(),
                foreground.name());
    }
    if (mControls->styleSheet() != css) mControls->setStyleSheet(css);
    // The floating field paints a capsule using native edit/arrow geometry.
    // Its popup keeps the system style and current palette.
    for (auto child : mControls->findChildren<QWidget *>()) child->setPalette(controls);
    for (auto combo : mControls->findChildren<QComboBox *>()) {
        combo->setProperty("floatingZoomPill", true);
        if (auto editor = combo->lineEdit()) {
            QPalette editorPalette = controls;
            editorPalette.setColor(QPalette::Base, Qt::transparent);
            editor->setPalette(editorPalette);
        }
        combo->update();
        combo->view()->setPalette(controls);
        combo->view()->window()->setPalette(controls);
    }
    refreshIcons(true);
    mPanel->setAutoFillBackground(false);
    mPanel->viewport()->setAutoFillBackground(false);
    mControls->setAutoFillBackground(false);
    updateGeometryAndMask();
    mSurface->update();
    mToggle->update();
}

// QAction state changes can reset a button's icon even when its artwork has
// not changed. Reuse its tinted icon without restyling or laying out the bar again.
void FloatingViewToolbar::refreshIcons(bool force)
{
    if (mControlsDocked) return;
    const QPalette controls = mControls->palette();
    for (auto button : findChildren<QToolButton *>()) {
        const QIcon current = button->icon();
        if (!force && current.cacheKey() == mThemedIconKeys.value(button)) continue;
        QIcon source = button->defaultAction() ? button->defaultAction()->icon()
            : current.cacheKey() == mThemedIconKeys.value(button) ? mOriginalIcons.value(button) : current;
        if (force && !source.name().isEmpty()) source = QIcon::fromTheme(source.name(), source);
        // KDE's thumbnail toggle is text-only. Supply an icon only for its
        // floating presentation; its original action and docked look stay intact.
        if (source.isNull() && button->defaultAction()
            && button->defaultAction()->objectName() == QLatin1String("toggle_thumbnailbar"))
            source = QIcon::fromTheme(QStringLiteral("view-preview"), style()->standardIcon(QStyle::SP_FileDialogContentsView));
        if (source.isNull()) continue;
        if (force || !mThemedIcons.contains(button) || source.cacheKey() != mOriginalIcons.value(button).cacheKey()) {
            mOriginalIcons.insert(button, source);
            mThemedIcons.insert(button, QIcon(new ToolbarIconEngine(source, controls)));
        }
        button->setIcon(mThemedIcons.value(button));
        mThemedIconKeys.insert(button, button->icon().cacheKey());
    }
}

bool FloatingViewToolbar::needsBackdrop() const
{
    return !mControlsDocked && isVisible()
        && (mProgress > 0 || mToggle->property("revealOpacity").toReal() > 0);
}

void FloatingViewToolbar::refreshBackdrop()
{
    auto view = qobject_cast<QGraphicsView *>(mViewer);
    if (!view || !needsBackdrop() || mCaptureRect.isEmpty()) return;
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
