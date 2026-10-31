// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "dialogshell.h"
#include "annotationtools.h"
#include "languagemenulist.h"
#include <QApplication>
#include <QAbstractButton>
#include <QCursor>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QStackedWidget>
#include <QSplitter>
#include <QStyle>
#include <QScopeGuard>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVariantAnimation>

namespace Gwenview {
// Live dialog/page fades preserve the underlying material and input targets.
// Timing and easing follow the other Weather-style floating surfaces.
class DialogEntrance : public QWidget
{
public:
    explicit DialogEntrance(QWidget *surface, bool page = false) : QWidget(surface), mSurface(surface), mPage(page)
    {
        setObjectName(page ? QStringLiteral("dialogPageTransition") : QStringLiteral("dialogEntrance"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        hide();
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(page ? QStringLiteral("dialogPageAnimation") : QStringLiteral("dialogEntranceAnimation"));
        mAnimation->setDuration(Sequoia::LongDuration);
        mAnimation->setStartValue(0.0); mAnimation->setEndValue(1.0);
        mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mProgress = value.toReal();
            if (mPageEffect) mPageEffect->setOpacity(mPage ? 0.65 + 0.35 * mProgress : mProgress);
            else update();
        });
        connect(mAnimation, &QVariantAnimation::finished, this, [this] { finish(); });
        qApp->installEventFilter(this);
        if (auto stack = qobject_cast<QStackedWidget *>(surface))
            connect(stack, &QStackedWidget::currentChanged, this, [this] { open(); });
    }
    void open()
    {
        finish();
        if (!mSurface->isVisible() || mSurface->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mSurface) == 0) return;
        if (mPage || mSurface->findChild<QObject *>(QStringLiteral("dialogShell"), Qt::FindDirectChildrenOnly)) {
            // Fade the live page over its actual parent material. A cached
            // stack image with a Window-color underlay flashes an opaque pale
            // rectangle over transparent dialogs on every view change.
            auto stack = qobject_cast<QStackedWidget *>(mSurface);
            auto page = stack ? stack->currentWidget() : mSurface;
            if (!page || page->graphicsEffect()) return;
            mPageWidget = page;
            mPageEffect = new QGraphicsOpacityEffect(page);
            mPageEffect->setOpacity(mPage ? 0.65 : 0.0);
            page->setGraphicsEffect(mPageEffect);
            mProgress = 0; mAnimation->start();
            return;
        }
        const int generation = ++mGeneration;
        mPending = true;
        QTimer::singleShot(0, this, [this, generation] {
            if (!mPending || generation != mGeneration || !mSurface->isVisible()) return;
            mPending = false;
            if (mSurface->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mSurface) == 0) return;
            mFrame = mSurface->grab();
            setGeometry(mSurface->rect()); mProgress = 0;
            show(); raise(); mAnimation->start();
        });
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == mSurface && event->type() == QEvent::Show && !mPage) open();
        if ((object == mSurface || object == mSurface->window())
            && (event->type() == QEvent::Hide || event->type() == QEvent::Resize
                || event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            finish();
        }
        if ((isVisible() || mPending || mPageEffect) && (event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::KeyPress || event->type() == QEvent::Wheel || event->type() == QEvent::TouchBegin)) {
            auto widget = qobject_cast<QWidget *>(object);
            if (widget && widget->window() == mSurface->window()) finish();
        }
        return QWidget::eventFilter(object, event);
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), mSurface->palette().color(QPalette::Window));
        painter.setOpacity(0.25 + 0.75 * mProgress);
        painter.drawPixmap(QPoint(0, qRound(6 * (1 - mProgress))), mFrame);
    }
private:
    void finish()
    {
        ++mGeneration; mPending = false; mAnimation->stop(); hide(); mFrame = QPixmap();
        if (mPageWidget && mPageEffect && mPageWidget->graphicsEffect() == mPageEffect)
            mPageWidget->setGraphicsEffect(nullptr);
        mPageEffect = nullptr; mPageWidget = nullptr;
    }
    QWidget *mSurface;
    QVariantAnimation *mAnimation;
    QPixmap mFrame;
    QPointer<QWidget> mPageWidget;
    QPointer<QGraphicsOpacityEffect> mPageEffect;
    qreal mProgress = 0;
    int mGeneration = 0;
    bool mPage;
    bool mPending = false;
};

// Restyle Qt's own overflow buttons without replacing their scrolling,
// auto-repeat, accessibility or right-to-left behavior.
class DialogPillArrow : public QObject
{
public:
    explicit DialogPillArrow(QToolButton *button) : QObject(button), mButton(button)
    {
        setObjectName(QStringLiteral("dialogPillArrow"));
        // Let the dialog material show outside the pill; Qt's tab-scroll
        // buttons otherwise fill their entire rectangular widget bounds.
        button->setAutoFillBackground(false);
        mAnimation = new QVariantAnimation(this);
        mAnimation->setDuration(Sequoia::ShortDuration);
        mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mEmphasis = value.toReal(); mButton->update();
        });
        connect(button, &QAbstractButton::pressed, this, [this] { refresh(); });
        connect(button, &QAbstractButton::released, this, [this] { refresh(); });
        button->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::Enter: mHover = true; refresh(); break;
        case QEvent::Leave: mHover = false; refresh(); break;
        case QEvent::EnabledChange: case QEvent::PaletteChange:
        case QEvent::FocusIn: case QEvent::FocusOut: refresh(); break;
        case QEvent::Hide: mHover = false; mAnimation->stop(); mEmphasis = 0; break;
        case QEvent::Paint: paint(); return true;
        default: break;
        }
        return false;
    }
private:
    void refresh()
    {
        const qreal target = !mButton->isEnabled() ? 0 : mButton->isDown() ? 0.14 : mHover ? 0.08 : 0;
        mAnimation->stop();
        if (!mButton->isVisible() || mButton->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mButton) == 0) {
            mEmphasis = target; mButton->update(); return;
        }
        mAnimation->setStartValue(mEmphasis); mAnimation->setEndValue(target); mAnimation->start();
    }
    void paint()
    {
        QPainter painter(mButton); painter.setRenderHint(QPainter::Antialiasing);
        const auto palette = mButton->palette();
        const QRectF pill = QRectF(mButton->rect()).adjusted(1.5, 3.5, -1.5, -3.5);
        const qreal radius = qMin(pill.width(), pill.height()) / 2;
        QColor border = palette.color(QPalette::WindowText); border.setAlpha(24);
        painter.setPen(border); painter.setBrush(palette.color(QPalette::Button));
        painter.drawRoundedRect(pill, radius, radius);
        QColor emphasis = palette.color(QPalette::WindowText); emphasis.setAlphaF(mEmphasis);
        painter.setPen(Qt::NoPen); painter.setBrush(emphasis);
        painter.drawRoundedRect(pill, radius, radius);
        if (mButton->hasFocus()) {
            painter.setPen(QPen(palette.color(QPalette::Highlight), 1.5)); painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(pill, radius, radius);
        }
        painter.setPen(QPen(palette.color(mButton->isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText),
                            1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF center = pill.center();
        const qreal direction = mButton->arrowType() == Qt::LeftArrow ? -1 : 1;
        painter.drawPolyline(QPolygonF{center + QPointF(-direction * 2, -4), center + QPointF(direction * 2, 0),
                                      center + QPointF(-direction * 2, 4)});
    }
    QToolButton *mButton;
    QVariantAnimation *mAnimation;
    qreal mEmphasis = 0;
    bool mHover = false;
};

// Paint the real QTabBar as one segmented pill. Native tab geometry, mnemonics,
// accessibility, overflow scrolling and keyboard selection remain in charge.
class DialogPill : public QObject
{
public:
    explicit DialogPill(QTabBar *bar) : QObject(bar), mBar(bar)
    {
        setObjectName(QStringLiteral("dialogPill"));
        bar->setProperty("gwenviewDialogPill", true);
        bar->setDrawBase(false);
        bar->setExpanding(true);
        bar->setElideMode(Qt::ElideNone);
        bar->setUsesScrollButtons(true);
        bar->setMouseTracking(true);
        for (auto button : bar->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly))
            new DialogPillArrow(button);
        mAnimation = new QVariantAnimation(this);
        mAnimation->setObjectName(QStringLiteral("dialogPillAnimation"));
        mAnimation->setDuration(Sequoia::LongDuration); mAnimation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(mAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            mSelection = value.toRectF(); mBar->update();
        });
        connect(bar, &QTabBar::currentChanged, this, [this] {
            const QRectF target = selectedRect();
            mAnimation->stop();
            if (!mBar->isVisible() || mSelection.isEmpty()
                || mBar->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, mBar) == 0) {
                mSelection = target; mBar->update(); return;
            }
            mAnimation->setStartValue(mSelection); mAnimation->setEndValue(target); mAnimation->start();
        });
        bar->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::Show: case QEvent::Resize: case QEvent::Hide:
        case QEvent::StyleChange: case QEvent::PaletteChange: case QEvent::LayoutDirectionChange:
            mAnimation->stop(); mSelection = selectedRect(); mBar->update(); break;
        case QEvent::MouseMove: case QEvent::Leave: case QEvent::FocusIn: case QEvent::FocusOut:
            mBar->update(); break;
        case QEvent::Paint: paint(); return true;
        default: break;
        }
        return false;
    }
private:
    QRectF selectedRect() const { return mBar->currentIndex() < 0 ? QRectF() : QRectF(mBar->tabRect(mBar->currentIndex())).adjusted(3, 3, -3, -3); }
    void paint()
    {
        QPainter painter(mBar); painter.setRenderHint(QPainter::Antialiasing);
        const auto palette = mBar->palette();
        QRectF track = QRectF(mBar->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        // Leave a rounded end beside the separate overflow pills, including
        // when Qt places them at the leading edge in a right-to-left layout.
        for (auto button : mBar->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly)) {
            if (!button->isVisible()) continue;
            if (button->geometry().center().x() > mBar->rect().center().x())
                track.setRight(qMin(track.right(), qreal(button->geometry().left() - 3)));
            else
                track.setLeft(qMax(track.left(), qreal(button->geometry().right() + 3)));
        }
        QColor border = palette.color(QPalette::WindowText); border.setAlpha(24);
        QColor trough = palette.color(QPalette::WindowText); trough.setAlpha(12);
        painter.setPen(border); painter.setBrush(trough);
        painter.drawRoundedRect(track, track.height() / 2, track.height() / 2);
        QPainterPath clip;
        clip.addRoundedRect(track, track.height() / 2, track.height() / 2);
        painter.setClipPath(clip);
        if (mAnimation->state() != QAbstractAnimation::Running) mSelection = selectedRect();
        if (!mSelection.isEmpty()) {
            QColor shadow(Qt::black); shadow.setAlpha(12);
            painter.setPen(Qt::NoPen); painter.setBrush(shadow);
            painter.drawRoundedRect(mSelection.translated(0, 1), mSelection.height()/2, mSelection.height()/2);
            painter.setPen(border); painter.setBrush(palette.color(QPalette::Button));
            painter.drawRoundedRect(mSelection, mSelection.height()/2, mSelection.height()/2);
        }
        const int hover = mBar->underMouse() ? mBar->tabAt(mBar->mapFromGlobal(QCursor::pos())) : -1;
        for (int i = 0; i < mBar->count(); ++i) {
            if (!mBar->isTabVisible(i)) continue;
            const QRect rect = mBar->tabRect(i).adjusted(12, 0, -12, 0);
            const bool current = i == mBar->currentIndex();
            if (hover == i && !current && mBar->isTabEnabled(i)) {
                QColor color = palette.color(QPalette::WindowText); color.setAlpha(9);
                painter.setPen(Qt::NoPen); painter.setBrush(color);
                const QRectF hoverRect = QRectF(mBar->tabRect(i)).adjusted(3, 3, -3, -3);
                painter.drawRoundedRect(hoverRect, hoverRect.height()/2, hoverRect.height()/2);
            }
            // Text-only segments keep the switcher calm; the original icon is
            // retained on the tab for accessibility and other presentations.
            painter.setFont(mBar->font());
            painter.setPen(palette.color(mBar->isTabEnabled(i) ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
            painter.drawText(rect, Qt::AlignCenter | Qt::TextHideMnemonic, mBar->tabText(i));
            if (current && mBar->hasFocus()) {
                QColor focus = palette.color(QPalette::Highlight); focus.setAlpha(180);
                painter.setPen(QPen(focus, 1.5)); painter.setBrush(Qt::NoBrush);
                const QRectF focusRect = QRectF(mBar->tabRect(i)).adjusted(4, 4, -4, -4);
                painter.drawRoundedRect(focusRect, focusRect.height()/2, focusRect.height()/2);
            }
        }
    }
    QTabBar *mBar;
    QVariantAnimation *mAnimation;
    QRectF mSelection;
};

inline void polishDialogPages(QDialog *dialog)
{
    for (auto tabs : dialog->findChildren<QTabWidget *>()) {
        auto bar = tabs->tabBar();
        if (bar->shape() != QTabBar::RoundedNorth || bar->property("gwenviewDialogPill").toBool()) continue;
        tabs->setDocumentMode(false);
        new DialogPill(bar);
        // Only tab-page stacks: internal file-picker and editor stacks keep
        // their own behavior and are not animated a second time.
        auto stack = tabs->findChild<QStackedWidget *>(QString(), Qt::FindDirectChildrenOnly);
        if (stack && !stack->findChild<QWidget *>(QStringLiteral("dialogPageTransition"))) new DialogEntrance(stack, true);
    }
}

inline void polishFilePickerLayout(QDialog *dialog)
{
    for (auto places : dialog->findChildren<QWidget *>()) {
        if (!places->inherits("KFilePlacesView")) continue;
        // The Places view sits inside a dock in KFileWidget's horizontal
        // splitter. Without an explicit stretch policy Qt first scales both
        // panes, then KFileWidget restores the saved sidebar width, producing
        // a visible squeeze/snap during interactive window resizing.
        for (QWidget *pane = places; pane && pane != dialog; pane = pane->parentWidget()) {
            auto splitter = qobject_cast<QSplitter *>(pane->parentWidget());
            if (!splitter || splitter->orientation() != Qt::Horizontal) continue;
            if (!splitter->property("gwenviewStablePlacesWidth").toBool()) {
                splitter->setProperty("gwenviewStablePlacesWidth", true);
                for (int i = 0; i < splitter->count(); ++i)
                    splitter->setStretchFactor(i, splitter->widget(i) == pane ? 0 : 1);
                splitter->setCollapsible(splitter->indexOf(pane), false);
            }
            break;
        }
    }
}

// Keep the native rename editor and its signals, but stop its layout from
// stretching a short filename across an arbitrarily wide Properties window.
class PropertiesFileNameWidth : public QObject
{
public:
    explicit PropertiesFileNameWidth(QLineEdit *edit) : QObject(edit), mEdit(edit)
    {
        setObjectName(QStringLiteral("propertiesFileNameWidth"));
        edit->setMinimumWidth(160);
        edit->setSizePolicy(QSizePolicy::Expanding, edit->sizePolicy().verticalPolicy());
        // Give the editor first claim on the row up to its content width;
        // an expanding trailing spacer takes any unused room. Alignment on
        // QLineEdit itself would pin it to Qt's short default sizeHint.
        auto parent = edit->parentWidget();
        auto row = new QWidget(parent);
        row->setObjectName(QStringLiteral("propertiesFileNameRow"));
        if (auto item = parent->layout() ? parent->layout()->replaceWidget(edit, row) : nullptr) {
            delete item;
            auto layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
            layout->addWidget(edit, 1); layout->addStretch(0);
        } else {
            delete row;
        }
        connect(edit, &QLineEdit::textChanged, this, [this] { refresh(); });
        edit->installEventFilter(this);
        refresh();
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange
            || event->type() == QEvent::ContentsRectChange || event->type() == QEvent::Show)
            refresh();
        return false;
    }
private:
    void refresh()
    {
        const auto margins = mEdit->textMargins();
        int padding = margins.left() + margins.right() + 20
            + 2 * mEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, mEdit);
        if (mEdit->isClearButtonEnabled()) padding += mEdit->style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, mEdit) + 6;
        // A maximum, rather than a fixed width, lets the layout constrain long
        // names in narrow windows while QLineEdit retains native text scrolling.
        mEdit->setMaximumWidth(qMax(160, mEdit->fontMetrics().horizontalAdvance(mEdit->displayText()) + padding));
    }
    QLineEdit *mEdit;
};

inline void polishDialogFooters(QDialog *dialog)
{
    for (auto box : dialog->findChildren<QDialogButtonBox *>()) {
        box->setProperty("_breeze_force_frame", false);
        auto parent = box->parentWidget();
        if (!parent || !parent->inherits("KPageView") || !parent->layout()) continue;
        // KPageView's native style draws a separator solely from the direct
        // parent type. A neutral layout host preserves the actual button box,
        // its actions and spacing while opting out of that decoration.
        auto footer = new QWidget(parent); footer->setObjectName(QStringLiteral("gwenviewDialogFooter"));
        if (auto item = parent->layout()->replaceWidget(box, footer)) {
            delete item;
            auto row = new QHBoxLayout(footer); row->setContentsMargins(0,0,0,0); row->addWidget(box);
        } else delete footer;
    }
}

inline void polishDialog(QDialog *dialog)
{
    // Late-created native layouts can show again while the shell changes
    // window flags. Do not attach a second entrance effect during that Show.
    if (dialog->property("gwenviewDialogPolishing").toBool()) return;
    dialog->setProperty("gwenviewDialogPolishing", true);
    const auto polishing = qScopeGuard([dialog] { dialog->setProperty("gwenviewDialogPolishing", false); });
    polishDialogFooters(dialog);
    polishFilePickerLayout(dialog);
    if (dialog->layout() && !dialog->findChild<QObject *>(QStringLiteral("dialogShell"), Qt::FindDirectChildrenOnly)) new DialogShell(dialog);
    if (dialog->objectName() == QLatin1String("gwenviewPropertiesDialog")) {
        if (auto edit = dialog->findChild<QLineEdit *>(QStringLiteral("fileNameLineEdit"));
            edit && !edit->findChild<QObject *>(QStringLiteral("propertiesFileNameWidth")))
            new PropertiesFileNameWidth(edit);
    }
    if (dialog->property("gwenviewPolishedDialog").toBool()) { polishDialogPages(dialog); return; }
    dialog->setProperty("gwenviewPolishedDialog", true);
    // Keep object names intact: KConfigDialog uses its name to find an existing
    // settings window. Palette roles and native controls follow the live theme.
    dialog->setStyleSheet(dialog->styleSheet() + QStringLiteral(R"(
        QTabWidget::pane { border: 0; padding-top: 14px; }
        QDialog#gwenviewPropertiesDialog QTabWidget::pane { border: 0; padding: 0; padding-top: 16px; }
        QTabWidget::tab-bar { alignment: center; }
        QTabBar::tab { min-height: 22px; padding: 7px 18px; margin: 0; border: 0; }
        QGroupBox { background: palette(base); border: 1px solid palette(mid); border-radius: 22px; margin-top: 28px; padding: 18px; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 5px; }
        QFrame[gwenviewSettingsCard="true"] { background: palette(base); border: 1px solid palette(mid); border-radius: 22px; }
        QLabel[gwenviewSectionTitle="true"] { font-weight: 600; background: transparent; }
        QScrollArea#gwenviewSettingsScroll { border: 0; background: transparent; }
    )"));
    auto content = dialog->findChild<QWidget *>(QStringLiteral("dialogShellContent"), Qt::FindDirectChildrenOnly);
    auto layout = content ? content->layout() : dialog->layout();
    if (layout) { layout->setContentsMargins(16, 12, 16, 16); layout->setSpacing(16); }
    new DialogEntrance(dialog);
    polishDialogPages(dialog);
}

// Some theme glyphs are fixed black SVGs. Recolor only flat monochrome
// command icons, keeping photographs and colored application artwork intact.
inline void polishDialogIcon(QAbstractButton *button)
{
    QIcon source = button->icon();
    const auto last = button->property("gwenviewTintedIcon").toULongLong();
    if (last && source.cacheKey() == qint64(last)) source = button->property("gwenviewSourceIcon").value<QIcon>();
    if (source.isNull() || source.name().isEmpty()) return;
    // Theme engines can use a symbolic glyph at 24px and colored artwork at
    // 32px. Inspect the actual displayed size before deciding to recolor it.
    const QImage sample = source.pixmap(button->iconSize(), button->devicePixelRatioF()).toImage();
    int visible = 0, colored = 0, darkest = 255, lightest = 0;
    for (int y = 0; y < sample.height(); ++y) for (int x = 0; x < sample.width(); ++x) {
        const QColor color = sample.pixelColor(x, y);
        if (color.alpha() < 30) continue;
        ++visible; darkest = qMin(darkest, color.lightness()); lightest = qMax(lightest, color.lightness());
        if (qMax(color.red(), qMax(color.green(), color.blue())) - qMin(color.red(), qMin(color.green(), color.blue())) > 35) ++colored;
    }
    if (!visible || colored * 20 >= visible || lightest - darkest >= 40) {
        if (last && button->icon().cacheKey() == qint64(last)) button->setIcon(source);
        return;
    }
    const auto role = QPalette::ButtonText;
    const QString key = QString::number(source.cacheKey()) + '/' + QString::number(button->palette().cacheKey())
        + '/' + QString::number(role) + '/' + QString::number(button->devicePixelRatioF());
    if (last && button->icon().cacheKey() == qint64(last) && button->property("gwenviewIconColors").toString() == key) return;
    QIcon tinted;
    for (const int side : {16, 22, 32, 48}) {
        for (const auto mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected}) {
            QPixmap glyph = source.pixmap(QSize(side, side), button->devicePixelRatioF());
            QPainter painter(&glyph); painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(glyph.rect(), button->palette().color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active, role));
            painter.end(); tinted.addPixmap(glyph, mode);
        }
    }
    button->setProperty("gwenviewSourceIcon", source);
    button->setProperty("gwenviewIconColors", key);
    button->setIcon(tinted);
    button->setProperty("gwenviewTintedIcon", qulonglong(tinted.cacheKey()));
}

// Also reaches KDE-created message boxes, file pickers, shortcut/toolbar editors,
// print dialogs, permission pages and lazy plugin dialogs opened from any menu.
class PolishedDialogs : public QObject
{
public:
    explicit PolishedDialogs(QApplication *app) : QObject(app) { app->installEventFilter(this); }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() != QEvent::Polish && event->type() != QEvent::Show
            && event->type() != QEvent::PaletteChange && event->type() != QEvent::FocusIn && event->type() != QEvent::FocusOut) return false;
        if (auto button = qobject_cast<QAbstractButton *>(object); button && button->window()->property("gwenviewPolishedDialog").toBool())
            polishDialogIcon(button);
        auto widget = qobject_cast<QWidget *>(object);
        if (auto dock = qobject_cast<QDockWidget *>(widget); dock && dock->inherits("kImageAnnotator::AnnotationDockWidget")
            && !dock->findChild<QObject *>(QStringLiteral("annotationToolsPolish"), Qt::FindDirectChildrenOnly))
            new AnnotationTools(dock);
        if (widget && widget->inherits("KLanguageButton") && event->type() == QEvent::Show) {
            for (auto button : widget->findChildren<QPushButton *>())
                if (button->menu() && !button->findChild<QObject *>(QStringLiteral("languageMenuList"))) new LanguageMenuList(button);
        }
        if (widget && widget->property("gwenviewSettingsCard").toBool()) {
            auto shadow = qobject_cast<QGraphicsDropShadowEffect *>(widget->graphicsEffect());
            if (!shadow && !widget->graphicsEffect()) {
                shadow = new QGraphicsDropShadowEffect(widget);
                shadow->setBlurRadius(12); shadow->setOffset(0, 0); widget->setGraphicsEffect(shadow);
            }
            if (shadow) shadow->setColor(Sequoia::alpha(widget->palette().color(QPalette::WindowText), 0.10));
        }
        if (widget && widget->window()->property("gwenviewPolishedDialog").toBool()) {
            if (auto frame = qobject_cast<QFrame *>(widget); frame && frame->frameShape() == QFrame::HLine) {
                frame->setFrameShape(QFrame::NoFrame); frame->setFixedHeight(0); frame->hide();
            }
        }
        auto dialog = qobject_cast<QDialog *>(object);
        // QFileDialog is the platform bridge, not the visible KDE picker.
        // Changing its window flags makes Qt abandon the native helper. The
        // KDEPlatformFileDialog receives the shell through this same filter.
        if (dialog && dialog->inherits("QFileDialog")) return false;
        if (dialog) {
            QPalette palette = dialog->palette();
            for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
                palette.setColor(group, QPalette::Mid, Sequoia::alpha(palette.color(group, QPalette::WindowText), 0.13));
            if (palette != dialog->palette()) dialog->setPalette(palette);
        }
        if (dialog && (event->type() == QEvent::Polish || event->type() == QEvent::Show)) {
            polishDialog(dialog);
            for (auto button : dialog->findChildren<QAbstractButton *>()) polishDialogIcon(button);
            if (event->type() == QEvent::Show) {
                auto entrance = static_cast<DialogEntrance *>(dialog->findChild<QWidget *>(QStringLiteral("dialogEntrance"), Qt::FindDirectChildrenOnly));
                if (entrance) entrance->open();
                QTimer::singleShot(0, dialog, [dialog] { polishDialogPages(dialog); });
            }
        }
        return false;
    }
};
}
