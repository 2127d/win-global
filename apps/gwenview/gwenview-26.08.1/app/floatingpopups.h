// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "popupsurface.h"
#include "buttonripple.h"
#include "popupmotion.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QAbstractItemView>
#include <QFrame>
#include <QLayout>
#include <QMenu>
#include <QProxyStyle>
#include <QStyleOption>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <KColorScheme>
#include <KConfigWatcher>
#include <KConfigGroup>

namespace Gwenview {
class FloatingMenuStyle : public QProxyStyle
{
public:
    explicit FloatingMenuStyle(const QString &base) : QProxyStyle(base) {}
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
                    tint.fillRect(glyph.rect(), item->palette.color(group, item->state & State_Selected ? QPalette::HighlightedText : QPalette::WindowText));
                    tint.end(); copy.icon = QIcon(glyph);
                }
                QProxyStyle::drawControl(element, &copy, painter, widget); return;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        if (qobject_cast<const QMenu *>(widget)) {
            if (element == PE_PanelMenu) { paintPopupSurface(*painter, widget); return; }
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
        const bool menu = qobject_cast<QMenu *>(widget);
        const bool comboPopup = widget->inherits("QComboBoxPrivateContainer");
        if ((menu || comboPopup) && widget->property("gwenviewFloatingPopup").toBool()
            && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)) {
            syncPalette(widget); mRefresh.start();
        }
        if ((menu || comboPopup) && (event->type() == QEvent::Polish || event->type() == QEvent::Show)) {
            const bool reducedMotion = widget->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, widget) == 0;
            if (!widget->property("gwenviewFloatingPopup").toBool()) {
                widget->setProperty("gwenviewFloatingPopup", true);
                widget->setProperty("_KDE_NET_WM_SKIP_SHADOW", true);
                widget->setWindowFlag(Qt::NoDropShadowWindowHint);
                widget->setAttribute(Qt::WA_TranslucentBackground);
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
            if (comboPopup) {
                if (widget->layout()) widget->layout()->setContentsMargins(22, 22, 22, 22);
                if (auto combo = qobject_cast<QComboBox *>(widget->parentWidget())) {
                    widget->setMinimumWidth(qMax(combo->width(), combo->view()->sizeHintForColumn(0) + 20) + 44);
                    // The container supplies the material. Do not stack a second
                    // translucent/opaque list background over it.
                    combo->view()->setAutoFillBackground(false);
                    combo->view()->viewport()->setAutoFillBackground(false);
                    combo->view()->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
                }
            }
            // Lazy menus may be re-polished when their actions are rebuilt.
            // Restore our surface style on every opening, not only first use.
            if (menu && widget->style() != mStyle) widget->setStyle(mStyle);
            widget->setWindowOpacity(1.0);
            widget->clearMask();
            syncPalette(widget);
            auto motion = static_cast<PopupMotion *>(widget->findChild<QObject *>(QStringLiteral("popupMotion"), Qt::FindDirectChildrenOnly));
            if (!motion) motion = new PopupMotion(widget);
            if (event->type() == QEvent::Show) motion->open(reducedMotion);
        }
        // The desktop style filters combo-container paints before QFrame can
        // draw them. Paint the outer surface here, leaving its native list alone.
        if (comboPopup && widget->property("gwenviewFloatingPopup").toBool() && event->type() == QEvent::Paint) {
            QPainter painter(widget);
            painter.setCompositionMode(QPainter::CompositionMode_Source); painter.fillRect(widget->rect(), Qt::transparent);
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
        for (auto popup : qApp->topLevelWidgets()) {
            if (!popup->property("gwenviewFloatingPopup").toBool()) continue;
            syncPalette(popup);
            // A style repolish can restore a list viewport background. The
            // outer container is the sole material layer, even while open.
            if (popup->inherits("QComboBoxPrivateContainer")) {
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
