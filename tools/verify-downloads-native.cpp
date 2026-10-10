// SPDX-License-Identifier: MIT
// Test-only hook: exercise the visible button, never bypass its input handler.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QQmlExpression>
#include <QQmlContext>
#include <QUrl>
#include <QTest>
#include <cstdlib>
#include <dlfcn.h>

static bool fanBlurSeen = false;
static bool fanBlurEnabled = true;
namespace KWindowEffects {
void enableBlurBehind(QWindow *window, bool enabled, const QRegion &region) {
    using Function = void (*)(QWindow *, bool, const QRegion &);
    static auto original = reinterpret_cast<Function>(dlsym(RTLD_NEXT, "_ZN14KWindowEffects16enableBlurBehindEP7QWindowbRK7QRegion"));
    if (window->property("sequoiaNoBlur").toBool()) {
        fanBlurSeen = true;
        fanBlurEnabled = enabled;
    }
    if (original) original(window, enabled, region);
}
}

static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (int i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}
static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    std::_Exit(2);
}
static void start() {
    if (qEnvironmentVariableIsEmpty("DOWNLOADS_NATIVE_CAPTURE")) return;
    QTimer::singleShot(1200, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto view = qobject_cast<QQuickWindow *>(window);
            if (!view) continue;
            for (auto root : items(view->contentItem())) {
                if (root->objectName() != "sequoiaDownloadsStack") continue;
                QQmlExpression config(qmlContext(root), root,
                    "Plasmoid.configuration.folderUrl = \"" + QUrl::fromLocalFile(qEnvironmentVariable("DOWNLOADS_NATIVE_FOLDER")).toString() + "\"");
                config.evaluate();
                if (config.hasError()) fail("fixture configuration");
                // Only the test ignores unrelated focus changes during capture.
                if (auto dialog = root->findChild<QObject *>("downloadsFanWindow"))
                    dialog->setProperty("closeOnFocusLoss", false);
                view->resize(64, 68);
                QTimer::singleShot(350, qApp, [view] {
                    QQuickItem *button = nullptr;
                    QQuickItem *icon = nullptr;
                    for (auto item : items(view->contentItem())) {
                        if (item->objectName() == "downloadsStackButton") button = item;
                        if (item->objectName() == "downloadsFolderIcon") icon = item;
                    }
                    if (!button || !icon || !button->isVisible() || icon->width() < 24 || icon->height() < 24)
                        fail("visible Downloads folder button missing");
                    auto capture = view->grabWindow();
                    int bluePixels = 0;
                    for (int y = 0; y < capture.height(); ++y)
                        for (int x = 0; x < capture.width(); ++x) {
                            auto c = capture.pixelColor(x, y);
                            if (c.alpha() > 128 && c.blue() > c.red() + 30) ++bluePixels;
                        }
                    if (bluePixels < 40) fail("Downloads folder icon did not render");
                    capture.save(qEnvironmentVariable("DOWNLOADS_NATIVE_CAPTURE") + ".button.png");
                    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
                        button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
                });
                return;
            }
        }
        fail("native applet missing");
    });
    QTimer::singleShot(4000, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto view = qobject_cast<QQuickWindow *>(window);
            if (!view || !view->isVisible() || !view->isExposed()) continue;
            for (auto fan : items(view->contentItem())) {
                if (fan->objectName() != "downloadsFan" || !fan->isVisible()) continue;
                auto model = fan->findChild<QObject *>("downloadsFiles");
                int rows = 0;
                for (auto child : items(fan))
                    if (child->objectName().startsWith("downloadEntry") && child->opacity() > 0.9) ++rows;
                if (!model || model->property("count").toInt() != 3 || rows != 3) fail("file rows missing");
                if (!fanBlurSeen || fanBlurEnabled) fail("fan still requests compositor blur");
                auto capture = view->grabWindow();
                if (capture.isNull() || capture.pixelColor(0, 0).alpha() != 0) fail("fan backdrop is not transparent");
                capture.save(qEnvironmentVariable("DOWNLOADS_NATIVE_CAPTURE"));
                QTest::keyClick(view, Qt::Key_Escape);
                const int duration = fan->property("motionDuration").toInt();
                QTest::qWait(duration / 2);
                const double reveal = fan->property("reveal").toDouble();
                if (!view->isVisible() || reveal <= 0 || reveal >= 1) fail("popup hides before fold animation");
                view->grabWindow().save(qEnvironmentVariable("DOWNLOADS_NATIVE_CAPTURE") + ".folding.png");
                QTest::qWait(duration + 100);
                if (view->isVisible()) fail("Escape did not close popup");
                fprintf(stderr, "PASS: rendered Downloads folder icon, real button click opens fan, visible rows, transparency, compositor blur disabled and Escape finishes folding before hiding\n");
                std::_Exit(0);
            }
        }
        fail("button click did not open fan");
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
