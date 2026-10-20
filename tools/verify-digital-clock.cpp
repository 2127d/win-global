// SPDX-License-Identifier: MIT
// Exercise the packaged applet inside plasmawindowed on a private session.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlExpression>
#include <QQmlContext>
#include <QTimer>
#include <QTest>
#include <QExposeEvent>
#include <QRegion>
#include <dlfcn.h>
#include <cstdlib>

// Observe and forward the real native request, including the Wayland backend.
namespace KWindowEffects {
void enableBlurBehind(QWindow *window, bool enabled, const QRegion &region) {
    using Function = void (*)(QWindow *, bool, const QRegion &);
    static auto original = reinterpret_cast<Function>(dlsym(RTLD_NEXT, "_ZN14KWindowEffects16enableBlurBehindEP7QWindowbRK7QRegion"));
    for (auto owner = window; owner; owner = owner->transientParent()) {
        if (owner->objectName() == "sequoiaDigitalClockSettings") {
            window->setProperty("testBlurEnabled", enabled);
            window->setProperty("testBlurRequests", window->property("testBlurRequests").toInt() + 1);
            break;
        }
    }
    if (original) original(window, enabled, region);
}
}

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
static void verifyDropdown(QObject *page, const QString &name, QQuickWindow *settingsView, const QString &output) {
    auto combo = qobject_cast<QQuickItem *>(eval(page, name).value<QObject *>());
    require(combo && combo->isEnabled(), "dropdown available");
    const int originalIndex = combo->property("currentIndex").toInt();
    const int selectedIndex = (originalIndex + 1) % combo->property("count").toInt();
    // Mouse selection, keyboard selection, Escape, and clicking outside. Each
    // attempt starts focused, reproducing the focus-out regression on opening.
    const int attempts = name == "showSecondsComboBox" ? 4 : 1;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        settingsView->requestActivate();
        combo->forceActiveFocus();
        QTest::qWait(80);
        if (attempt == 1)
            QTest::keyClick(settingsView, Qt::Key_Space);
        else
            QTest::mouseClick(settingsView, Qt::LeftButton, Qt::NoModifier,
                combo->mapToScene(QPointF(combo->width()/2, combo->height()/2)).toPoint());
        QTest::qWait(450);
        auto popupObject = eval(page, name + ".popup").value<QObject *>();
        auto popup = qobject_cast<QQuickWindow *>(eval(page, name + ".popup.contentItem.Window.window").value<QObject *>());
        require(popup && popup != settingsView && popup->isVisible()
                && popupObject->property("opened").toBool(), "clicked dropdown stays open after focus transfer");
        require(popup->property("testBlurEnabled").toBool(), "theme blurs interactive dropdown, including reopen");
        if (name == "showSecondsComboBox" && attempt == 0)
            require(popup->grabWindow().save(output + "-settings-dropdown.png"), "save dropdown preview");
        int expectedIndex = selectedIndex;
        if (attempt == 0) {
            auto choice = qobject_cast<QQuickItem *>(eval(page, name + QStringLiteral(".popup.itemAt(%1)").arg(selectedIndex)).value<QObject *>());
            require(choice, "dropdown choice available");
            QTest::mouseClick(popup, Qt::LeftButton, Qt::NoModifier,
                choice->mapToScene(QPointF(choice->width()/2, choice->height()/2)).toPoint());
        } else if (attempt == 1) {
            const int previousHighlight = popupObject->property("currentIndex").toInt();
            QTest::keyClick(popup, Qt::Key_Down);
            expectedIndex = popupObject->property("currentIndex").toInt();
            require(expectedIndex >= 0 && expectedIndex != previousHighlight, "arrow key moves menu highlight");
            QTest::keyClick(popup, Qt::Key_Return);
        } else if (attempt == 2) {
            QTest::keyClick(popup, Qt::Key_Escape);
        } else {
            const auto outside = popup->mapFromGlobal(settingsView->mapToGlobal(QPoint(4, 4)));
            QTest::mouseClick(popup, Qt::LeftButton, Qt::NoModifier, outside);
        }
        QTest::qWait(250);
        require(!popupObject->property("visible").toBool(), "selection, Escape, or outside click dismisses menu");
        if (attempt < 2) {
            require(combo->property("currentIndex").toInt() == expectedIndex,
                    "mouse and keyboard selections update combo value");
            eval(page, name + QStringLiteral(".currentIndex = %1; ").arg(originalIndex)
                 + name + QStringLiteral(".activated(%1)").arg(originalIndex));
        }
    }
    fprintf(stderr, "PASS: %s click, focus handoff, blur, selection and dismissal\n", qPrintable(name));
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
            auto surface = named(view, "sequoiaCalendarSurface");
            require(surface, "calendar surface exists");
            const auto originalBackground = eval(calendar, "Kirigami.Theme.backgroundColor.toString()").toString();
            const auto originalText = eval(calendar, "Kirigami.Theme.textColor.toString()").toString();
            require(surface->property("color") == calendar->property("cardColor"), "calendar uses theme card color at startup");
            require(eval(today, "Kirigami.Theme.textColor.toString()").toString() == originalText,
                    "calendar dates use theme text color at startup");
            for (const auto background : {"#24262b", "#ffffff"}) {
                const auto text = QString::fromLatin1(background) == "#ffffff" ? "#24262b" : "#ffffff";
                eval(calendar, QStringLiteral("Kirigami.Theme.backgroundColor = '%1'; Kirigami.Theme.textColor = '%2'")
                     .arg(background, text));
                QTest::qWait(300);
                require(surface->property("color") == calendar->property("cardColor"),
                        "calendar surface follows live light/dark palette changes");
                require(eval(today, "Kirigami.Theme.textColor.toString()").toString() == QString::fromLatin1(text),
                        "calendar dates follow live light/dark text changes");
            }
            eval(calendar, QStringLiteral("Kirigami.Theme.backgroundColor = '%1'; Kirigami.Theme.textColor = '%2'")
                 .arg(originalBackground, originalText));
            QTest::qWait(300);
            fprintf(stderr, "PASS: calendar surface and date text follow light/dark palettes at startup and while open\n");
            const auto originalAccent = eval(calendar, "Kirigami.Theme.highlightColor.toString()").toString();
            for (const auto accent : {"#4b97b8", "#e9643a"}) {
                eval(calendar, QStringLiteral("Kirigami.Theme.highlightColor = '%1'").arg(accent));
                QTest::qWait(300);
                require(eval(today, "background.color.toString()").toString() == QString::fromLatin1(accent),
                        "date highlight follows live accent changes across the calendar surface");
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
            QQuickWindow *settingsView = nullptr;
            QQuickItem *settings = nullptr;
            for (auto window : QGuiApplication::allWindows()) {
                auto candidate = qobject_cast<QQuickWindow *>(window);
                if (candidate && (settings = named(candidate, "sequoiaClockSettings"))) {
                    settingsView = candidate;
                    break;
                }
            }
            require(settingsView && settings, "Sequoia settings frame installed");
            const auto windowMaterial = eval(settings, "Kirigami.Theme.backgroundColor").value<QColor>();
            require(settingsView->color().alpha() == 0 && windowMaterial.alpha() > 0
                    && settings->property("color").value<QColor>() == windowMaterial,
                    "outer canvas paints the decoration's Window material exactly once");
            eval(settings, "Kirigami.Theme.backgroundColor = '#80314253'");
            require(settings->property("color").value<QColor>() == QColor("#80314253"), "outer canvas follows live palette color and opacity");
            eval(settings, QStringLiteral("Kirigami.Theme.backgroundColor = '%1'").arg(windowMaterial.name(QColor::HexArgb)));
            QTest::qWait(100);
            const auto canvasCapture = settingsView->grabWindow();
            const auto canvasPixel = canvasCapture.pixelColor(canvasCapture.width() / 2, 0);
            require(qAbs(canvasPixel.alpha() - windowMaterial.alpha()) <= 1
                    && qAbs(canvasPixel.red() - windowMaterial.red()) <= 1
                    && qAbs(canvasPixel.green() - windowMaterial.green()) <= 1
                    && qAbs(canvasPixel.blue() - windowMaterial.blue()) <= 1,
                    "outer margin renders the configured window color and opacity");
            auto sidebar = named(settingsView, "sequoiaClockSettingsSidebar");
            auto content = named(settingsView, "sequoiaClockSettingsContentSurface");
            require(sidebar && content && sidebar->x() > 0 && content->x() > sidebar->x() + sidebar->width(),
                    "settings panels float with outer margins and a gap");
            require(content->property("color").value<QColor>() == eval(settings, "Kirigami.Theme.backgroundColor").value<QColor>(),
                    "content panel follows the configured opaque or translucent palette");
            if (qEnvironmentVariable("QT_STYLE_OVERRIDE") == "SequoiaEleven") {
                require(settingsView->property("testBlurEnabled").toBool(), "native settings background blur enabled");
                const int blurRequests = settingsView->property("testBlurRequests").toInt();
                QExposeEvent expose(QRegion(settingsView->geometry()));
                QCoreApplication::sendEvent(settingsView, &expose);
                QTest::qWait(150);
                require(settingsView->property("testBlurRequests").toInt() > blurRequests,
                        "settings blur reapplied on exposure");
                auto page = eval(settings, "applicationWindow().pageStack.currentItem").value<QObject *>();
                require(page, "appearance page available for dropdown check");
                require(eval(page, "showSecondsComboBox.popup.popupType === QQC2.Popup.Window").toBool(),
                        "dropdown requests a compositor popup surface");
                // QTest has no Wayland input serial, so native grabs require
                // real compositor input. Exercise click/focus behavior offscreen.
                if (QGuiApplication::platformName() != "wayland") {
                    for (const auto name : {"dateDisplayFormat", "showSecondsComboBox", "displayTimeZoneFormat", "use24hFormat", "dateFormat"})
                        verifyDropdown(page, name, settingsView, output);
                }
                fprintf(stderr, "PASS: theme supplies settings blur and dropdowns request popup windows\n");
            }
            if (qEnvironmentVariable("QT_QUICK_BACKEND") != "software") {
                require(named(settingsView, "sequoiaClockSettingsPageMask"), "rounded page mask active");
                const auto capture = settingsView->grabWindow();
                const auto origin = eval(settings, "applicationWindow().pageStack.mapToItem(null, 0, 0)").toPointF();
                const qreal dpr = capture.devicePixelRatio();
                const auto corner = capture.pixelColor(qRound((origin.x() + 1) * dpr), qRound((origin.y() + 1) * dpr));
                const auto edge = capture.pixelColor(qRound((origin.x() + 22) * dpr), qRound((origin.y() + 1) * dpr));
                if (content->property("color").value<QColor>().alpha() < 255)
                    require(corner.alpha() < edge.alpha(), "page corner reveals the translucent card instead of square page paint");
            }
            require(settingsView->grabWindow().save(output + "-settings-appearance.png"), "save appearance settings preview");
            eval(settings, "applicationWindow().pageStack.currentItem.cfg_showDate = !Plasmoid.configuration.showDate");
            require(eval(settings, "isConfigurationChanged()").toBool(), "settings edits detected");
            eval(settings, "applyAction.trigger()");
            require(!eval(settings, "isConfigurationChanged()").toBool(), "Apply saves settings");
            // Cancel a navigation attempt with unsaved edits, then discard them.
            eval(settings, "applicationWindow().pageStack.currentItem.cfg_showDate = !Plasmoid.configuration.showDate; categories.openCategory(configDialog.configModel.get(1))");
            require(eval(settings, "messageDialog.visible").toBool(), "navigation prompts for unsaved edits");
            eval(settings, "messageDialog.close()");
            require(eval(settings, "isConfigurationChanged()").toBool(), "cancel keeps unsaved page");
            eval(settings, "categories.openCategory(configDialog.configModel.get(1)); messageDialog.discarded()");
            QTest::qWait(350);
            require(eval(settings, "'cfg_showWeekNumbers' in applicationWindow().pageStack.currentItem").toBool(), "Calendar settings load after discard");
            if (qEnvironmentVariable("QT_STYLE_OVERRIDE") == "SequoiaEleven" && QGuiApplication::platformName() != "wayland")
                verifyDropdown(eval(settings, "applicationWindow().pageStack.currentItem").value<QObject *>(),
                               "firstDayOfWeekCombo", settingsView, output);
            require(settingsView->grabWindow().save(output + "-settings-calendar.png"), "save calendar settings preview");
            eval(settings, "open(configDialog.configModel.get(2))");
            QTest::qWait(350);
            require(eval(settings, "'cfg_selectedTimeZones' in applicationWindow().pageStack.currentItem").toBool(), "Time Zones settings load");
            require(settingsView->grabWindow().save(output + "-settings-timezones.png"), "save timezone settings preview");
            eval(settings, "open(globalConfigModel.get(0))");
            QTest::qWait(350);
            require(settingsView->grabWindow().save(output + "-settings-shortcuts.png"), "save shortcuts settings preview");
            fprintf(stderr, "PASS: floating settings panels, rounded page mask, transparency, Appearance/Calendar/Time Zones/Shortcuts, Apply and unsaved-change cancel/discard\n");
            fprintf(stderr, "PASS: native popup, 12/24-hour formats, seconds, custom date, live time, world clocks, month navigation, month/year overviews, today reset, week numbers, first weekday, pin, compact/agenda layouts and appearance settings\n");
            std::_Exit(0);
        }
        require(false, "packaged calendar did not load");
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
