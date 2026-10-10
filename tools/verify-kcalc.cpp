// SPDX-License-Identifier: MIT
// Exercise the actual application without changing the desktop account.
#include <QBitArray>
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QDateTime>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

static void require(bool ok, const char *message)
{
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void startup()
{
    QTimer::singleShot(600, qApp, [] {
        QMainWindow *window = nullptr;
        for (auto widget : QApplication::topLevelWidgets())
            if (widget->inherits("KCalculator")) window = qobject_cast<QMainWindow *>(widget);
        require(window, "calculator window");
        const QString output = qEnvironmentVariable("SEQUOIA_KCALC_VERIFY");
        auto action = [&](const char *name) {
            auto result = window->findChild<QAction *>(QString::fromLatin1(name));
            if (!result) {
                auto widgets = window->findChildren<QWidget *>(); widgets.prepend(window);
                for (auto widget : widgets)
                    for (auto candidate : widget->actions())
                        if (candidate->objectName() == QLatin1String(name)) result = candidate;
            }
            require(result, name); return result;
        };
        const auto initialMenus = window->menuBar()->actions();
        const auto initialMenuCount = window->findChildren<QMenu *>().size();
        auto checkMenus = [&] {
            require(window->menuBar()->actions() == initialMenus, "top-level menus stay stable across modes");
            require(window->findChildren<QMenu *>().size() == initialMenuCount, "mode changes do not accumulate menus");
            for (auto menu : window->menuBar()->actions()) {
                require(menu->text().remove(QLatin1Char('&')) != QStringLiteral("Constants"), "no top-level Constants menu");
            }
        };
        checkMenus();
        auto mode = [&](const char *name) { action(name)->trigger(); QTest::qWait(100); checkMenus(); };
        auto capture = [&](const char *name) { require(window->grab().save(output + '/' + name + ".png"), "capture"); };
        mode("mode_simple");
        QSize simple = window->size();
        capture("simple");
        auto modes = window->findChild<QComboBox *>("calculatorMode");
        require(modes && modes->currentIndex() == 0, "mode selector follows menu");
        const QList<const char *> advanced{"mode_science", "mode_statistics", "mode_numeral"};
        for (auto name : advanced) {
            mode(name);
            fprintf(stderr, "%s: %dx%d; simple: %dx%d\n", name, window->width(), window->height(), simple.width(), simple.height());
            require(window->width() > simple.width(), "advanced mode grows");
            capture(name);
            mode("mode_simple");
            require(window->size() == simple, "advanced to simple returns to compact size");
        }
        for (int i = 0; i < 3; ++i) {
            window->resize(simple + QSize(220, 150)); QTest::qWait(100);
            require(window->size() == simple + QSize(220, 150), "manual resize remains unrestricted");
            QTest::qWait(150);
            require(window->size() == simple + QSize(220, 150), "manual resize is not undone at idle");
            mode("mode_science"); mode("mode_simple");
            require(window->size() == simple, "manual enlargement does not leak across modes");
        }
        action("mode_science")->trigger(); action("mode_numeral")->trigger(); action("mode_simple")->trigger();
        QTest::qWait(100);
        require(window->size() == simple, "rapid mode changes coalesce");
        modes->setCurrentIndex(1); Q_EMIT modes->activated(1); QTest::qWait(100);
        require(action("mode_science")->isChecked(), "mode selector triggers native action");
        for (auto state : {Qt::WindowMaximized, Qt::WindowFullScreen}) {
            window->setWindowState(state); QTest::qWait(150);
            const QSize expanded = window->size();
            for (auto name : {"mode_simple", "mode_numeral", "mode_science"}) {
                mode(name);
                require(window->windowState().testFlag(state), "window state preserved");
                require(window->size() == expanded, "mode switch preserves fullscreen/maximized geometry");
            }
            window->showNormal(); QTest::qWait(100);
            mode("mode_simple"); require(window->size() == simple, "normal mode refits after leaving fullscreen/maximized");
        }
        // Check the native calculation path and keyboard shortcuts after switching modes.
        auto input = window->findChild<QLineEdit *>("input_display");
        require(input, "input display");
        auto outputDisplay = window->findChild<QWidget *>("calc_display");
        require(outputDisplay, "output display");
        auto resultIs = [&](const char *text) {
            require(outputDisplay->accessibleName() == QString::fromLatin1(text), "visible output matches expected result");
            action("edit_copy")->trigger();
            require(QApplication::clipboard()->text().trimmed() == QString::fromLatin1(text), "copy matches visible output");
        };
        auto click = [&](const char *name) {
            auto button = window->findChild<QPushButton *>(QString::fromLatin1(name));
            require(button && button->isVisible(), name);
            QTest::mouseClick(button, Qt::LeftButton); QTest::qWait(100);
        };
        auto history = window->findChild<QTextEdit *>("calc_history");
        auto liveResult = window->findChild<QWidget *>("calculatorLiveResult");
        auto inputFrame = window->findChild<QWidget *>("calculatorInputFrame");
        require(history && liveResult && inputFrame, "history and inline preview widgets");
        require(history->isVisible() && history->isReadOnly(), "history is visible and read-only");
        require(history->mapTo(window, QPoint(0, history->height())).y() < inputFrame->mapTo(window, QPoint()).y(), "history sits above input");
        require(history->toPlainText().isEmpty() && !liveResult->isVisible(), "clean start has empty history and no preview");
        input->setFocus(); QTest::keyClicks(input, "1 + 2"); QTest::qWait(100);
        resultIs("3");
        require(liveResult->isVisible(), "valid expression shows inline preview");
        require(history->toPlainText().isEmpty(), "typing does not update history");
        capture("live-preview");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200);
        require(input->text() == QStringLiteral("3"), "Enter replaces 1 + 2 with 3");
        require(history->toPlainText().trimmed() == QStringLiteral("1 + 2 = 3"), "Enter adds original equation and answer once");
        require(!liveResult->isVisible(), "Enter hides inline preview");
        capture("history");
        const QString firstHistory = history->toPlainText();
        QTest::keyClicks(input, "+");
        require(!liveResult->isVisible() && history->toPlainText() == firstHistory, "incomplete edit leaves history unchanged and hides preview");
        QTest::keyClicks(input, "4"); resultIs("7");
        require(liveResult->isVisible() && history->toPlainText() == firstHistory, "live result does not change history");
        click("pbEqual");
        require(input->text() == QStringLiteral("7") && !liveResult->isVisible(), "equals updates total and hides preview");
        require(history->toPlainText().contains(QStringLiteral("3+4 = 7")) && history->toPlainText().contains(firstHistory.trimmed()), "history retains successive calculations");
        const QString completedHistory = history->toPlainText();
        mode("mode_science"); mode("mode_simple");
        require(!liveResult->isVisible() && history->toPlainText() == completedHistory, "mode switches retain history and committed preview state");
        input->selectAll(); QTest::keyClick(input, Qt::Key_Backspace);
        require(input->text().isEmpty() && history->toPlainText() == completedHistory && !liveResult->isVisible(), "empty input hides preview without editing history");
        click("pbClear");
        require(input->text().isEmpty() && history->toPlainText().isEmpty() && !liveResult->isVisible(), "C clears entry, history and preview");
        input->setText(QStringLiteral("4+5")); click("pbEqual");
        require(!history->toPlainText().isEmpty(), "new equation recorded after C");
        click("pbAllClear");
        require(history->toPlainText().isEmpty() && !liveResult->isVisible(), "AC clears history and preview");
        input->setFocus(); QTest::keyClicks(input, "2+3*4"); QTest::qWait(100);
        resultIs("14");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200);
        resultIs("14");
        require(input->text() == QStringLiteral("14"), "Enter replaces expression with running total");
        capture("calculation");
        QTest::keyClicks(input, "-"); resultIs("14");
        QTest::keyClicks(input, "4"); resultIs("10");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200); resultIs("10");
        click("pbPlus"); resultIs("10");
        click("pb2"); resultIs("12");
        click("pbEqual"); resultIs("12");
        require(!liveResult->isVisible(), "equals hides preview after continued arithmetic");
        require(input->text() == QStringLiteral("12"), "equals button replaces expression with running total");
        click("pb3"); resultIs("3"); // A digit after equals starts a fresh calculation.
        input->setFocus(); QTest::keyClick(input, Qt::Key_Backspace); resultIs("");
        require(input->text().isEmpty(), "deleting the input clears output");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200); resultIs("");
        input->setFocus(); QTest::keyClicks(input, "7-9"); resultIs("-2");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200); resultIs("-2");
        require(input->text() == QStringLiteral("-2"), "7-9 becomes -2 in the input");
        QTest::keyClicks(input, "+"); resultIs("-2");
        QTest::keyClicks(input, "5"); resultIs("3");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200); resultIs("3");
        require(input->text() == QStringLiteral("3"), "continued arithmetic uses the previous total");
        input->selectAll(); QTest::keyClick(input, Qt::Key_Backspace); resultIs("");
        input->setText(QStringLiteral("   ")); resultIs("");
        input->clear();
        QTest::keyClicks(input, "7"); resultIs("7");
        QTest::keyClicks(input, "/0"); resultIs("7"); // Preserve last successful answer while editing.
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(200);
        require(!outputDisplay->accessibleName().isEmpty() && outputDisplay->accessibleName() != QStringLiteral("7"), "Enter still reports division by zero");
        require(liveResult->isVisible(), "explicit evaluation errors stay visible in input row");
        input->setText(QStringLiteral("9-2")); resultIs("7");
        for (auto name : {"pbClear", "pbAllClear"}) {
            click(name);
            require(input->text().isEmpty(), "clear button empties input");
            resultIs("");
            click("pb8"); resultIs("8");
            click("pbEqual"); resultIs("8");
        }
        // The on-screen Backspace also clears output when the input becomes empty.
        mode("mode_science");
        click("pbBackspace");
        require(input->text().isEmpty(), "on-screen backspace removes input");
        resultIs("");
        click("pbAllClear"); resultIs("");
        mode("mode_simple");
        const QSize beforeHistory = window->size();
        for (int i = 0; i < 30; ++i) {
            input->setText(QStringLiteral("%1 + 1").arg(i));
            require(QMetaObject::invokeMethod(window, "slotEqualclicked"), "commit long history");
        }
        QTest::qWait(100);
        require(history->document()->blockCount() > 1 || history->toPlainText().count(QLatin1Char('\n')) >= 29, "history retains many results");
        require(window->size() == beforeHistory && !liveResult->isVisible(), "long history scrolls without enlarging window or restoring preview");
        capture("history-many");
        fprintf(stderr, "PASS: all modes, repeated/rapid switches, manual resizing, fullscreen, maximized, selector, live results, running totals, history, inline preview, empty input, Enter/equals, clear buttons, errors and stable menus\n");
        checkMenus();
        // Let closing the final window exit normally; forcing exit here would
        // conceal a shutdown regression. The runner also times process teardown.
        fprintf(stderr, "CLOSING: %lld\n", QDateTime::currentMSecsSinceEpoch());
        require(window->close(), "normal close accepted");
        QTimer::singleShot(2000, qApp, [] { require(false, "normal close exits promptly"); });
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
