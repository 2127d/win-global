// SPDX-License-Identifier: MIT
// Test-only startup hook: capture the launched application's own Qt Quick window.
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QApplication>
#include <QQuickWidget>
#include <QTest>

static QList<QQuickItem *> visualItems(QQuickItem *root)
{
    // Popup content is reparented visually into the window overlay while its
    // QObject ownership remains with the drawer, outside contentItem's tree.
    QList<QQuickItem *> items{root};
    for (qsizetype i = 0; i < items.size(); ++i) items.append(items[i]->childItems());
    return items;
}

static bool verifyScrollClipping(QQuickItem *root)
{
    int checked = 0;
    for (auto item : visualItems(root)) {
        if (!item->isVisible() || !item->property("sequoiaInsetContent").toBool()) continue;
        auto viewport = item->property("contentItem").value<QQuickItem *>();
        if (!viewport || !viewport->isVisible() || viewport->width() <= 0 || viewport->height() <= 0) continue;
        const QRectF bounds = viewport->mapRectToItem(item, viewport->boundingRect());
        const QRectF inset(item->property("leftPadding").toReal(), item->property("topPadding").toReal(),
                           item->property("availableWidth").toReal(), item->property("availableHeight").toReal());
        if (!viewport->clip() || !inset.adjusted(-0.5, -0.5, 0.5, 0.5).contains(bounds)) {
            fprintf(stderr, "Scroll content must clip inside its floating surface's padded viewport\n");
            return false;
        }
        ++checked;
    }
    fprintf(stderr, "Verified %d inset scroll viewports clip their contents\n", checked);
    return true;
}

static bool verifyMerkuroIcons(QQuickItem *root)
{
    int checked = 0;
    const QStringList names{"Month View", "Week View", "Schedule View", "Tasks View"};
    for (auto item : visualItems(root)) {
        if (!item->isVisible() || !item->inherits("QQuickAbstractButton")
            || !names.contains(item->property("text").toString())) continue;
        bool found = false;
        for (auto icon : visualItems(item)) {
            if (!icon->isVisible() || QByteArray(icon->metaObject()->className()) != "Icon") continue;
            if (icon->width() < 12 || icon->height() < 12) return false;
            for (auto ancestor = icon->parentItem(); ancestor; ancestor = ancestor->parentItem()) {
                if ((ancestor == item || ancestor->clip())
                    && !ancestor->boundingRect().adjusted(-0.5, -0.5, 0.5, 0.5).contains(
                        icon->mapRectToItem(ancestor, icon->boundingRect()))) return false;
            }
            found = true;
        }
        if (!found) return false;
        ++checked;
    }
    fprintf(stderr, "Verified %d complete Merkuro navigation icons\n", checked);
    return checked == 4;
}

static bool scrollSidebar(QQuickItem *root)
{
    for (auto item : visualItems(root)) {
        if (!item->isVisible() || !item->property("sequoiaInsetContent").toBool()) continue;
        const bool navigation = item->property("sequoiaNavigationPage").toBool();
        const auto parent = item->parentItem();
        const bool discover = parent && parent->parentItem() && parent->parentItem()->objectName() == "DiscoverDrawer";
        if (!navigation && !discover) continue;
        auto viewport = item->property("contentItem").value<QQuickItem *>();
        if (!viewport || viewport->height() <= 0) continue;
        const qreal overflow = viewport->property("contentHeight").toReal() - viewport->height();
        if (overflow <= 0) continue;
        viewport->setProperty("contentY", viewport->property("originY").toReal() + overflow * 0.53);
        fprintf(stderr, "Scrolled sidebar to exercise partially visible rows\n");
        return true;
    }
    return false;
}

static QQuickItem *navigationItem(QQuickItem *item, const QString &text)
{
    auto action = item->property("tAction").value<QObject *>();
    if (item->inherits("QQuickAbstractButton") && action && action->property("text").toString().remove('&') == text && item->isVisible()) return item;
    for (auto child : item->childItems())
        if (auto found = navigationItem(child, text)) return found;
    return nullptr;
}

static void clickNavigation(const QString &text)
{
    for (auto window : QGuiApplication::allWindows()) {
        auto quick = qobject_cast<QQuickWindow *>(window);
        if (!quick || !quick->isVisible()) continue;
        if (auto item = navigationItem(quick->contentItem(), text)) {
            QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                             item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
            QCoreApplication::processEvents();
            if (!item->property("checked").toBool()) {
                fprintf(stderr, "Navigation click did not select the page\n");
                QCoreApplication::exit(4);
            }
            return;
        }
    }
    fprintf(stderr, "Navigation control was not found\n");
    QCoreApplication::exit(4);
}

static void describe(QQuickItem *item, QTextStream &out, int depth = 0)
{
    if (!item) return;
    out << QString(depth, ' ') << item->metaObject()->className() << " " << item->objectName()
        << " " << item->x() << "," << item->y() << " " << item->width() << "x" << item->height() << "\n";
    for (auto child : item->childItems()) describe(child, out, depth + 1);
}
static void describeWidget(QWidget *widget, QTextStream &out, int depth = 0)
{
    if (!widget->isVisible()) return;
    out << QString(depth, ' ') << widget->metaObject()->className() << " " << widget->objectName()
        << " " << widget->x() << "," << widget->y() << " " << widget->width() << "x" << widget->height() << "\n";
    for (auto child : widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        describeWidget(child, out, depth + 1);
}
static void captureStartup()
{
    const auto output = qEnvironmentVariable("SEQUOIA_CAPTURE_PATH");
    if (output.isEmpty()) return;
    const auto expectedApp = qEnvironmentVariable("SEQUOIA_CAPTURE_APP");
    if (!expectedApp.isEmpty() && QFileInfo(QCoreApplication::applicationFilePath()).fileName() != expectedApp) return;
    if (qEnvironmentVariableIsSet("SEQUOIA_COLLAPSE_MERKURO")) {
        QTimer::singleShot(2500, qApp, [] {
            for (auto window : QGuiApplication::allWindows()) {
                auto quick = qobject_cast<QQuickWindow *>(window);
                if (!quick || !quick->isVisible()) continue;
                for (auto item : visualItems(quick->contentItem())) {
                    if (!item->isVisible() || !QByteArray(item->metaObject()->className()).startsWith("MainDrawerToggleButton")) continue;
                    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                                     item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
                    return;
                }
            }
            fprintf(stderr, "Merkuro sidebar toggle was not found\n");
            QCoreApplication::exit(8);
        });
    }
    if (qEnvironmentVariableIsSet("SEQUOIA_MERKURO_VIEW")) {
        QTimer::singleShot(3500, qApp, [] {
            const QString name = qEnvironmentVariable("SEQUOIA_MERKURO_VIEW");
            const QString text = name == "week" ? "Week View" : name == "schedule" ? "Schedule View"
                : name == "tasks" ? "Tasks View" : "Month View";
            for (auto window : QGuiApplication::allWindows()) {
                auto quick = qobject_cast<QQuickWindow *>(window);
                if (!quick || !quick->isVisible()) continue;
                quick->resize(1200, qEnvironmentVariableIntValue("SEQUOIA_CAPTURE_HEIGHT"));
                for (auto item : visualItems(quick->contentItem())) {
                    if (!item->isVisible() || !item->inherits("QQuickAbstractButton") || item->property("text").toString() != text) continue;
                    QTest::mouseClick(quick, Qt::LeftButton, Qt::NoModifier,
                                     item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
                    return;
                }
            }
            fprintf(stderr, "Merkuro navigation target was not found\n");
            QCoreApplication::exit(8);
        });
    }
    if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_NAVIGATION")) {
        QTimer::singleShot(2000, qApp, [] { clickNavigation(QStringLiteral("Applications")); });
        QTimer::singleShot(3500, qApp, [] { clickNavigation(QStringLiteral("Overview")); });
    }
    QTimer::singleShot(1000, qApp, [] {
        const int height = qEnvironmentVariableIntValue("SEQUOIA_CAPTURE_HEIGHT");
        for (auto window : QGuiApplication::topLevelWindows())
            if (window->isVisible() && window->width() > 300) window->resize(1200, height > 0 ? height : 850);
    });
    if (qEnvironmentVariableIsSet("SEQUOIA_SCROLL_SIDEBAR")) {
        QTimer::singleShot(3000, qApp, [] {
            if (qobject_cast<QApplication *>(qApp)) {
                for (auto widget : QApplication::allWidgets()) {
                    auto quick = qobject_cast<QQuickWidget *>(widget);
                    if (quick && quick->isVisible() && quick->rootObject() && scrollSidebar(quick->rootObject())) return;
                }
            }
            for (auto window : QGuiApplication::allWindows()) {
                auto quick = qobject_cast<QQuickWindow *>(window);
                if (quick && quick->isVisible() && scrollSidebar(quick->contentItem())) return;
            }
            fprintf(stderr, "No scrollable sidebar was found\n");
            QCoreApplication::exit(7);
        });
    }
    QTimer::singleShot(qEnvironmentVariableIsSet("SEQUOIA_MERKURO_VIEW") ? 7000 : 5000, qApp, [output] {
        if (qobject_cast<QApplication *>(qApp)) {
            for (auto widget : QApplication::topLevelWidgets()) {
                if (!widget->isVisible() || widget->width() < 300) continue;
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_SCROLL_CLIPPING")) {
                    for (auto quick : widget->findChildren<QQuickWidget *>()) {
                        if (quick->rootObject() && !verifyScrollClipping(quick->rootObject())) {
                            QCoreApplication::exit(7);
                            return;
                        }
                    }
                }
                if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_SETTINGS_SIDEBAR")) {
                    int sidebars = 0;
                    for (auto quick : widget->findChildren<QQuickWidget *>()) {
                        for (auto item : quick->rootObject()->findChildren<QQuickItem *>(QStringLiteral("sequoiaSettingsSidebar"))) {
                            if (!item->isVisible()) continue;
                            auto canvas = item->parentItem()->findChild<QQuickItem *>(QStringLiteral("sequoiaSettingsSidebarCanvas"));
                            if (!canvas || !canvas->isVisible()
                                || canvas->size() != item->size()
                                || canvas->property("color").value<QColor>() != widget->palette().color(QPalette::Window)) {
                                fprintf(stderr, "Sidebar backing must retain the window palette color and alpha\n");
                                QCoreApplication::exit(6);
                                return;
                            }
                            if (item->width() < 150 || item->width() > 500
                                || item->height() < widget->height() - 60
                                || item->property("surfaceColor").value<QColor>().alpha() != 255
                                || item->property("radius").toReal() != 14
                                || item->property("inset").toReal() != 5) {
                                fprintf(stderr, "Settings sidebar must use a full-height opaque Discover island\n");
                                QCoreApplication::exit(6);
                                return;
                            }
                            ++sidebars;
                        }
                    }
                    if (sidebars != 1) {
                        fprintf(stderr, "Expected one visible full-height settings sidebar, found %d\n", sidebars);
                        QCoreApplication::exit(6);
                        return;
                    }
                    fprintf(stderr, "Verified full-height Discover-style settings sidebar\n");
                }
                QFile tree(output + ".tree");
                if (tree.open(QIODevice::WriteOnly)) {
                    QTextStream stream(&tree);
                    if (qEnvironmentVariableIsSet("SEQUOIA_CAPTURE_WIDGET_TREE")) describeWidget(widget, stream);
                    for (auto quick : widget->findChildren<QQuickWidget *>()) describe(quick->rootObject(), stream);
                }
                QCoreApplication::exit(widget->grab().save(output) ? 0 : 2);
                return;
            }
        }
        for (auto window : QGuiApplication::allWindows()) {
            auto quick = qobject_cast<QQuickWindow *>(window);
            if (!quick || !quick->isVisible() || quick->width() < 300) continue;
            if (qEnvironmentVariableIsSet("SEQUOIA_COLLAPSE_MERKURO") && !verifyMerkuroIcons(quick->contentItem())) {
                fprintf(stderr, "Collapsed Merkuro navigation icons are clipped or undersized\n");
                QCoreApplication::exit(8);
                return;
            }
            if (qEnvironmentVariableIsSet("SEQUOIA_VERIFY_SCROLL_CLIPPING") && !verifyScrollClipping(quick->contentItem())) {
                QCoreApplication::exit(7);
                return;
            }
            const int expectedTint = qEnvironmentVariableIntValue("SEQUOIA_EXPECT_WINDOW_TINT");
            if (expectedTint > 0) {
                const auto tint = quick->findChild<QQuickItem *>(QStringLiteral("sequoiaWindowTint"));
                const int clearAlpha = quick->color().alpha();
                if (!tint || tint->property("color").value<QColor>().alpha() != expectedTint
                    || clearAlpha != (expectedTint < 255 ? 0 : 255)
                    || tint->width() < quick->width() || tint->height() < quick->height()) {
                    fprintf(stderr, "Window tint must cover the canvas and avoid a translucent native clear color\n");
                    QCoreApplication::exit(5);
                    return;
                }
                fprintf(stderr, "Window tint alpha: %d; native clear alpha: %d\n", expectedTint, clearAlpha);
            }
            const auto image = quick->grabWindow();
            QFile tree(output + ".tree");
            if (tree.open(QIODevice::WriteOnly)) { QTextStream stream(&tree); describe(quick->contentItem(), stream); }
            QCoreApplication::exit(image.save(output) ? 0 : 2);
            return;
        }
        QCoreApplication::exit(3);
    });
}
Q_COREAPP_STARTUP_FUNCTION(captureStartup)
