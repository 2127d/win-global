// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "popupsurface.h"
#include "sequoiawidgetstyle.h"
#include "buttonripple.h"
#include "popupmotion.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QAbstractItemView>
#include <QFrame>
#include <QLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QProxyStyle>
#include <QPushButton>
#include <QScopeGuard>
#include <QStyleOption>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <KColorScheme>
#include <KConfigWatcher>
#include <KConfigGroup>

namespace Gwenview {
class FloatingMenuStyle : public SequoiaWidgetStyle
{
public:
    explicit FloatingMenuStyle(const QString &base) : SequoiaWidgetStyle(base) {}
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override
    {
        if (qobject_cast<const QMenu *>(widget)) {
            if (metric == PM_MenuPanelWidth) return 0;
            if (metric == PM_MenuHMargin || metric == PM_MenuVMargin) return 22;
            if (metric == PM_SubMenuOverlap) return 28;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *data = nullptr) const override
    {
        if (qobject_cast<const QMenu *>(widget) && hint == SH_Menu_Mask) return 0;
        return QProxyStyle::styleHint(hint, option, widget, data);
    }
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override
    {
        painter->save();
        const auto restore = qScopeGuard([painter] { painter->restore(); });
        if (qobject_cast<const QMenu *>(widget)) {
            QPainterPath clip; clip.addRoundedRect(popupBodyRect(widget), Sequoia::SurfaceRadius, Sequoia::SurfaceRadius);
            painter->setClipPath(clip, Qt::IntersectClip);
            if (element == CE_MenuItem)
                painter->setOpacity(painter->opacity() * (0.75 + 0.25 * widget->property("gwenviewPopupProgress").toReal()));
        }
        if (element == CE_MenuItem) {
            if (auto item = qstyleoption_cast<const QStyleOptionMenuItem *>(option); item && !item->icon.name().isEmpty()) {
                auto copy = *item;
                const int side = pixelMetric(PM_SmallIconSize, option, widget);
                QPixmap glyph = item->icon.pixmap(QSize(side, side), painter->device()->devicePixelRatioF());
                const QImage pixels = glyph.toImage();
                int colored = 0, visible = 0, darkest = 255, lightest = 0;
                for (int y = 0; y < pixels.height(); ++y) for (int x = 0; x < pixels.width(); ++x) {
                    const QColor color = pixels.pixelColor(x, y);
                    if (color.alpha() < 30) continue;
                    ++visible;
                    darkest = qMin(darkest, color.lightness()); lightest = qMax(lightest, color.lightness());
                    if (qMax(color.red(), qMax(color.green(), color.blue())) - qMin(color.red(), qMin(color.green(), color.blue())) > 35) ++colored;
                }
                // Theme symbolic assets can be black even in dark mode. Tint
                // only monochrome theme glyphs, preserving app/color previews.
                if (visible > 0 && colored * 20 < visible && lightest - darkest < 40) {
                    QPainter tint(&glyph); tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
                    const auto group = item->state & State_Enabled ? QPalette::Active : QPalette::Disabled;
                    tint.fillRect(glyph.rect(), item->palette.color(group, QPalette::WindowText));
                    tint.end(); copy.icon = QIcon(glyph);
                }
                SequoiaWidgetStyle::drawControl(element, &copy, painter, widget); return;
            }
        }
        SequoiaWidgetStyle::drawControl(element, option, painter, widget);
    }
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        if (qobject_cast<const QMenu *>(widget)) {
            if (element == PE_PanelMenu) {
                painter->save();
                painter->setCompositionMode(QPainter::CompositionMode_Source);
                painter->fillRect(widget->rect(), Qt::transparent);
                painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                paintPopupSurface(*painter, widget);
                painter->restore(); return;
            }
            if (element == PE_FrameMenu) return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

// App-local presentation only. The original menus, actions, delegates and popup
// event handlers continue to own keyboard navigation, selection and scrolling.
class FloatingPopups : public QObject
{
public:
    explicit FloatingPopups(QApplication *app) : QObject(app), mPalette(app->palette())
    {
        mStyle = new FloatingMenuStyle(app->style()->objectName()); mStyle->setParent(this);
        app->installEventFilter(this);
        // Plasma may replace kdeglobals before delivering the application
        // palette event. Watch both paths; coalesce changes after style polish.
        mRefresh.setSingleShot(true); mRefresh.setInterval(0);
        connect(&mRefresh, &QTimer::timeout, this, [this] { refreshPopups(); });
        mConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
        mConfigWatcher = KConfigWatcher::create(mConfig);
        connect(mConfigWatcher.data(), &KConfigWatcher::configChanged, this, [this] { reloadMaterial(); });
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        mConfigPath = directory + QStringLiteral("/kdeglobals");
        mFiles.addPath(directory); mFiles.addPath(mConfigPath);
        connect(&mFiles, &QFileSystemWatcher::fileChanged, this, [this] { reloadMaterial(); });
        connect(&mFiles, &QFileSystemWatcher::directoryChanged, this, [this] {
            if (!mFiles.files().contains(mConfigPath)) { mFiles.addPath(mConfigPath); reloadMaterial(); }
        });
        app->setEffectEnabled(Qt::UI_AnimateMenu, false);
        app->setEffectEnabled(Qt::UI_FadeMenu, false);
        app->setEffectEnabled(Qt::UI_AnimateCombo, false);
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == qApp && event->type() == QEvent::ApplicationPaletteChange) {
            mPalette = qApp->palette();
            mRefresh.start();
        }
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget) return false;
        if (event->type() == QEvent::ShortcutOverride && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            for (auto ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
                if (ancestor->inherits("KCommandBar")) {
                    // Find Action is a child overlay. Reserve Escape for its
                    // native close handler instead of Gwenview's Browse action.
                    event->accept(); return true;
                }
            }
        }
        const bool menu = qobject_cast<QMenu *>(widget);
        const bool comboPopup = widget->inherits("QComboBoxPrivateContainer");
        const bool commandPopup = widget->inherits("KCommandBar");
        if ((menu || comboPopup || commandPopup) && widget->property("gwenviewFloatingPopup").toBool()
            && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            syncPalette(widget); mRefresh.start();
        }
        if ((menu || comboPopup || commandPopup) && (event->type() == QEvent::Polish || event->type() == QEvent::Show)) {
            const bool reducedMotion = widget->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, widget) == 0;
            if (!widget->property("gwenviewFloatingPopup").toBool()) {
                widget->setProperty("gwenviewFloatingPopup", true);
                widget->setProperty("_KDE_NET_WM_SKIP_SHADOW", true);
                if (widget->isWindow()) widget->setWindowFlag(Qt::NoDropShadowWindowHint);
                widget->setAttribute(Qt::WA_TranslucentBackground, widget->isWindow());
                if (!widget->isWindow()) widget->setAttribute(Qt::WA_NoSystemBackground, false);
                widget->setAutoFillBackground(false);
                if (menu) widget->setStyle(mStyle);
                else {
                    if (auto frame = qobject_cast<QFrame *>(widget)) frame->setFrameShape(QFrame::NoFrame);
                    if (widget->layout()) widget->layout()->setContentsMargins(22, 22, 22, 22);
                }
            }
            widget->setAutoFillBackground(false);
            widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
            widget->setAttribute(Qt::WA_StyledBackground, false);
            // Annotation's custom QMenus lay child widgets out themselves;
            // their items need the same inset as ordinary native menu rows.
            if (menu && widget->layout()) widget->layout()->setContentsMargins(22, 22, 22, 22);
            if (comboPopup) {
                if (widget->layout()) widget->layout()->setContentsMargins(22, 22, 22, 22);
                if (auto combo = qobject_cast<QComboBox *>(widget->parentWidget())) {
                    // Some delegates report their current viewport width as
                    // a column hint (Print Preview can report hundreds of
                    // pixels for "100%"). Size from the actual option labels.
                    int textWidth = 0;
                    const QFontMetrics metrics(combo->view()->font());
                    for (int row = 0; row < combo->count(); ++row) {
                        const int iconWidth = combo->itemIcon(row).isNull() ? 0 : combo->iconSize().width() + 8;
                        textWidth = qMax(textWidth, metrics.horizontalAdvance(combo->itemText(row)) + iconWidth);
                    }
                    const int width = qMax(combo->width() + 28, textWidth + 64);
                    widget->setFixedWidth(qMin(width, combo->screen()->availableGeometry().width()));
                    widget->setMinimumHeight(qMax(80, combo->view()->sizeHintForRow(0) + 44));
                    // The container supplies the material. Do not stack a second
                    // translucent/opaque list background over it.
                    combo->view()->setAutoFillBackground(false);
                    combo->view()->viewport()->setAutoFillBackground(false);
                    combo->view()->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
                }
            }
            if (commandPopup) for (auto view : widget->findChildren<QAbstractItemView *>()) {
                view->setFrameShape(QFrame::NoFrame);
                view->setAutoFillBackground(false);
                view->viewport()->setAutoFillBackground(false);
                view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
            }
            // Lazy menus may be re-polished when their actions are rebuilt.
            // Restore our surface style on every opening, not only first use.
            if (menu && widget->style() != mStyle) widget->setStyle(mStyle);
            widget->setWindowOpacity(1.0);
            widget->clearMask();
            syncPalette(widget);
            if (event->type() == QEvent::Show) {
                resetDropdownAnchor(widget);
                if (comboPopup) {
                    if (auto field = qobject_cast<QComboBox *>(widget->parentWidget())) positionDropdown(widget, field);
                } else if (auto popup = qobject_cast<QMenu *>(widget)) {
                    bool submenu = false;
                    for (auto object : popup->menuAction()->associatedObjects())
                        if (auto parentMenu = qobject_cast<QMenu *>(object); parentMenu && parentMenu->isVisible()
                            && parentMenu->activeAction() == popup->menuAction()) submenu = true;
                    // Menus can belong to a dialog or action rather than the
                    // button displaying them (notably the folder icon menu).
                    for (auto candidate : qApp->allWidgets()) {
                        if (submenu) break;
                        if (!candidate->isVisible()) continue;
                        auto push = qobject_cast<QPushButton *>(candidate);
                        auto tool = qobject_cast<QToolButton *>(candidate);
                        if ((push && push->menu() == popup) || (tool && tool->menu() == popup)) {
                            positionDropdown(popup, candidate);
                            break;
                        }
                    }
                }
            }
            auto motion = static_cast<PopupMotion *>(widget->findChild<QObject *>(QStringLiteral("popupMotion"), Qt::FindDirectChildrenOnly));
            if (!motion) motion = new PopupMotion(widget);
            if (event->type() == QEvent::Show) motion->open(reducedMotion);
        }
        // The desktop style filters combo-container paints before QFrame can
        // draw them. Paint the outer surface here, leaving its native list alone.
        if ((comboPopup || commandPopup) && widget->property("gwenviewFloatingPopup").toBool() && event->type() == QEvent::Paint) {
            QPainter painter(widget);
            // Child overlays must retain their parent's backing pixels. Source
            // clearing here cuts a transparent hole through the main window.
            if (widget->isWindow()) {
                painter.setCompositionMode(QPainter::CompositionMode_Source); painter.fillRect(widget->rect(), Qt::transparent);
            }
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            paintPopupSurface(painter, widget); return true;
        }
        if (event->type() == QEvent::Show) {
            if (auto button = qobject_cast<QToolButton *>(widget); button && button->defaultAction()
                && button->defaultAction()->objectName() == QLatin1String("toggle_sidebar")
                && !button->findChild<QObject *>(QStringLiteral("sidebarButtonRipple")))
                new SidebarButtonRipple(button);
        }
        return false;
    }
private:
    void reloadMaterial()
    {
        mConfig->reparseConfiguration();
        mPalette = KColorScheme::createApplicationPalette(mConfig);
        mRefresh.start();
    }
    void refreshPopups()
    {
        for (auto popup : qApp->allWidgets()) {
            if (popup->property("gwenviewPolishedDialog").toBool()) {
                QPalette dialogPalette = mPalette;
                for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
                    dialogPalette.setColor(group, QPalette::Mid, Sequoia::alpha(dialogPalette.color(group, QPalette::WindowText), 0.13));
                if (popup->palette() != dialogPalette
                    || popup->property("gwenviewDialogMaterial").value<QPalette>() != dialogPalette) {
                    popup->setProperty("gwenviewDialogMaterial", dialogPalette);
                    // Unpolish first: Qt's stylesheet cache restores its old
                    // palette when cleared, including palettes on child cards.
                    QString sheet = popup->property("gwenviewDialogStyleTemplate").toString();
                    if (sheet.isEmpty()) {
                        sheet = popup->styleSheet();
                        popup->setProperty("gwenviewDialogStyleTemplate", sheet);
                    }
                    popup->setStyleSheet(QString());
                    popup->setPalette(dialogPalette);
                    for (auto child : popup->findChildren<QWidget *>())
                        if (child->window() == popup) child->setPalette(dialogPalette);
                    sheet.replace(QStringLiteral("palette(base)"), dialogPalette.color(QPalette::Base).name(QColor::HexArgb));
                    sheet.replace(QStringLiteral("palette(mid)"), dialogPalette.color(QPalette::Mid).name(QColor::HexArgb));
                    popup->setStyleSheet(sheet);
                }
            }
            if (!popup->property("gwenviewFloatingPopup").toBool()) continue;
            syncPalette(popup);
            // A style repolish can restore a list viewport background. The
            // outer container is the sole material layer, even while open.
            if (popup->inherits("QComboBoxPrivateContainer") || popup->inherits("KCommandBar")) {
                for (auto view : popup->findChildren<QAbstractItemView *>()) {
                    view->setAutoFillBackground(false);
                    view->viewport()->setAutoFillBackground(false);
                    view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
                }
            }
            popup->update();
        }
    }
    void syncPalette(QWidget *widget)
    {
        const QPalette palette = popupMaterialPalette(mPalette);
        if (widget->palette() != palette) widget->setPalette(palette);
    }
    QPalette mPalette;
    QTimer mRefresh;
    QFileSystemWatcher mFiles;
    QString mConfigPath;
    KSharedConfig::Ptr mConfig;
    KConfigWatcher::Ptr mConfigWatcher;
    FloatingMenuStyle *mStyle;
};
}
