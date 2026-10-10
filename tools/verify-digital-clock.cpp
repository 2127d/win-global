// SPDX-License-Identifier: MIT
// Exercise the packaged applet inside plasmawindowed on a private session.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <cstdlib>

static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> result{root};
    for (int i = 0; i < result.size(); ++i) result.append(result[i]->childItems());
    return result;
}
static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::_Exit(2); }
}
static QVariant eval(QObject *object, const QString &code) {
    QQmlExpression expression(qmlContext(object), object, code);
    auto value = expression.evaluate();
    if (expression.hasError()) fprintf(stderr, "%s\n", qPrintable(expression.error().toString()));
    require(!expression.hasError(), "QML expression");
    return value;
}
static QQuickItem *named(QQuickWindow *view, const QString &name) {
    for (auto item : items(view->contentItem())) if (item->objectName() == name) return item;
    return nullptr;
}
static void start() {
    QTimer::singleShot(2200, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto view = qobject_cast<QQuickWindow *>(window);
            if (!view) continue;
            if (auto button = named(view, "digital-clock-compactrepresentation")) {
                eval(button, "Plasmoid.configuration.use24hFormat = 2; Plasmoid.configuration.showSeconds = 2");
                QTest::qWait(100);
                require(eval(button, "timeFormatWithSeconds.indexOf('s') >= 0 && timeFormat.indexOf('AP') < 0").toBool(), "24-hour format and seconds");
                eval(button, "Plasmoid.configuration.use24hFormat = 0");
                QTest::qWait(100);
                require(eval(button, "timeFormat.toLowerCase().indexOf('ap') >= 0").toBool(), "12-hour format");
                eval(button, "Plasmoid.configuration.dateFormat = 'custom'; Plasmoid.configuration.customDateFormat = 'yyyy';");
                require(eval(button, "dateFormatter(new Date(2026, 9, 10)) === '2026'").toBool(), "custom date formatting");
                eval(button, "Plasmoid.configuration.dateFormat = 'shortDate'; Plasmoid.configuration.showSeconds = 1; Plasmoid.configuration.use24hFormat = 1");
                QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
                    button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint());
                return;
            }
        }
        require(false, "compact clock did not load");
    });
    QTimer::singleShot(4500, qApp, [] {
        for (auto window : QGuiApplication::allWindows()) {
            auto view = qobject_cast<QQuickWindow *>(window);
            if (!view) continue;
            auto calendar = named(view, "sequoiaCalendar");
            if (!calendar) continue;
            view->resize(880, 620);
            eval(calendar, "Plasmoid.configuration.selectedTimeZones = ['Local', 'Europe/London', 'Asia/Tokyo']; Plasmoid.configuration.displayTimezoneFormat = 1");
            QTest::qWait(450);
            auto month = named(view, "sequoiaMonthView");
            require(month && month->isVisible(), "calendar visible");
            QQuickItem *today = nullptr;
            for (auto item : items(month)) {
                if (item->objectName().startsWith("calendarCell-") && item->property("today").toBool()) {
                    today = item;
                    break;
                }
            }
            require(today, "today cell exists");
            const auto originalAccent = eval(calendar, "Kirigami.Theme.highlightColor.toString()").toString();
            for (const auto accent : {"#4b97b8", "#e9643a"}) {
                eval(calendar, QStringLiteral("Kirigami.Theme.highlightColor = '%1'").arg(accent));
                QTest::qWait(300);
                require(eval(today, "background.color.toString()").toString() == QString::fromLatin1(accent),
                        "date highlight follows live accent changes across the white surface");
            }
            eval(calendar, QStringLiteral("Kirigami.Theme.highlightColor = '%1'").arg(originalAccent));
            QTest::qWait(300);
            fprintf(stderr, "PASS: calendar date highlight responds to changing desktop accents\n");
            require(eval(calendar, "showClocks").toBool(), "world clocks visible");
            require(!named(view, "sequoiaHeroTime")->property("text").toString().isEmpty(), "live hero time");
            const auto originalMonth = eval(month, "selectedMonth").toString();
            eval(month, "nextView()");
            QTest::qWait(350);
            require(eval(month, "selectedMonth").toString() != originalMonth, "next month navigation");
            eval(month, "previousView()");
            QTest::qWait(350);
            require(eval(month, "selectedMonth").toString() == originalMonth, "previous month navigation");
            eval(month, "showYearView()");
            QTest::qWait(350);
            require(eval(month, "currentIndex").toInt() == 1, "month overview");
            eval(month, "showDecadeView()");
            QTest::qWait(350);
            require(eval(month, "currentIndex").toInt() == 2, "year overview");
            eval(month, "resetToToday()");
            eval(calendar, "Plasmoid.configuration.showWeekNumbers = true; Plasmoid.configuration.firstDayOfWeek = 1");
            QTest::qWait(350);
            require(eval(month, "showWeekNumbers && firstDayOfWeek === 1").toBool(), "calendar preferences");
            eval(month, "viewHeader.pinButton.animateClick()");
            QTest::qWait(250);
            require(eval(calendar, "Plasmoid.configuration.pin").toBool(), "pin popup");
            eval(month, "viewHeader.pinButton.animateClick()");
            QTest::qWait(250);
            require(!eval(calendar, "Plasmoid.configuration.pin").toBool(), "unpin popup");
            eval(calendar, "Plasmoid.configuration.showWeekNumbers = false");
            QTest::qWait(350);
            const auto output = qEnvironmentVariable("SEQUOIA_CLOCK_CAPTURE");
            require(view->grabWindow().save(output + ".png"), "save world clocks preview");
            eval(calendar, "Plasmoid.configuration.selectedTimeZones = ['Local']");
            view->resize(qMax(600, view->minimumWidth()), qMax(620, view->minimumHeight()));
            QTest::qWait(600);
            eval(month, "resetToToday()");
            view->requestUpdate();
            QTest::qWait(400);
            require(!eval(calendar, "showClocks").toBool(), "single timezone compact layout");
            require(view->grabWindow().save(output + "-compact.png"), "save compact preview");
            eval(calendar, "Plasmoid.configuration.enabledCalendarPlugins = ['holidaysevents']");
            QTest::qWait(450);
            require(eval(calendar, "showAgenda").toBool(), "agenda plugin enabled");
            require(view->grabWindow().save(output + "-agenda.png"), "save agenda preview");
            eval(month, "viewHeader.configureButton.animateClick()");
            QTest::qWait(1100);
            bool appearanceLoaded = false;
            for (auto configWindow : QGuiApplication::allWindows()) {
                auto configView = qobject_cast<QQuickWindow *>(configWindow);
                if (!configView || !configView->isVisible()) continue;
                for (auto item : items(configView->contentItem())) {
                    if (auto context = qmlContext(item))
                        appearanceLoaded |= context->baseUrl().toString().endsWith("configAppearance.qml");
                }
            }
            require(appearanceLoaded, "stock appearance settings load");
            fprintf(stderr, "PASS: native popup, 12/24-hour formats, seconds, custom date, live time, world clocks, month navigation, month/year overviews, today reset, week numbers, first weekday, pin, compact/agenda layouts and appearance settings\n");
            std::_Exit(0);
        }
        require(false, "packaged calendar did not load");
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
