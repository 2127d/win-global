// SPDX-License-Identifier: MIT
// Test-only startup hook for the real application, on a private session bus.
#include <QBitArray>
#include <QApplication>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QTest>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDialog>
#include <QAbstractButton>
#include <QSet>
#include <cstdio>
#include <cstdlib>

static void require(bool condition, const char *message)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static QList<QQuickItem *> items(QQuickItem *root)
{
    QList<QQuickItem *> list{root};
    for (int i = 0; i < list.size(); ++i) list.append(list[i]->childItems());
    return list;
}
static bool call(QObject *nav, const char *method, const QString &value)
{
    bool ok = false;
    require(QMetaObject::invokeMethod(nav, method, Qt::DirectConnection, Q_RETURN_ARG(bool, ok), Q_ARG(QString, value)), "Missing navigation method");
    QTest::qWait(100);
    return ok;
}
static bool back(QObject *nav)
{
    bool ok = false;
    require(QMetaObject::invokeMethod(nav, "back", Qt::DirectConnection, Q_RETURN_ARG(bool, ok)), "Missing back method");
    QTest::qWait(100);
    return ok;
}
static void open(QObject *nav, const QString &id)
{
    require(QMetaObject::invokeMethod(nav, "open", Qt::DirectConnection, Q_ARG(QString, id)), "Missing open method");
    QTest::qWait(600);
}
static void startup()
{
    if (qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY").isEmpty()) return;
    // Native modules may launch Qt helpers that inherit LD_PRELOAD.
    if (QFileInfo(QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget()).fileName() != QStringLiteral("systemsettings")) return;
    QTimer::singleShot(1000, qApp, [] {
        QWidget *window = nullptr;
        for (auto candidate : QApplication::topLevelWidgets()) {
            if (candidate->inherits("SettingsBase")) window = candidate;
        }
        require(window, "Missing System Settings window");
        auto nav = window->findChild<QObject *>(QStringLiteral("settingsNavigation"));
        require(nav, "Missing navigation controller");
        if (qEnvironmentVariableIsSet("SEQUOIA_SETTINGS_RTL")) qApp->setLayoutDirection(Qt::RightToLeft);
        window->resize(1100, 790);
        QTest::qWait(200);
        auto capture = [window](const QString &name) {
            const auto directory = qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY");
            require(window->grab().save(directory + QLatin1Char('/') + name + QStringLiteral(".png")), "Screenshot failed");
        };
        for (auto quick : window->findChildren<QQuickWidget *>()) require(quick->status() != QQuickWidget::Error && quick->rootObject(), "QML did not load");
        if (qEnvironmentVariableIsSet("SEQUOIA_SETTINGS_STARTUP")) {
            require(nav->property("moduleOpen").toBool(), "Startup deep link did not open a module");
            capture(QStringLiteral("startup"));
            require(back(nav), "Cannot leave startup module");
            require(call(nav, "browse", QStringLiteral("home")), "Cannot reach home after startup deep link");
        }
        require(nav->property("home").toBool(), "Application must start on the category overview");
        capture(QStringLiteral("home"));
        QQuickItem *searchField = nullptr;
        for (auto quick : window->findChildren<QQuickWidget *>()) {
            if (quick->rootObject()) {
                if (auto field = quick->rootObject()->findChild<QQuickItem *>(QStringLiteral("settingsSearch"))) searchField = field;
            }
        }
        require(searchField, "Search field missing");
        searchField->setProperty("text", QStringLiteral("keyboard"));
        QTest::qWait(100);
        require(nav->property("query").toString() == QStringLiteral("keyboard"), "Search field not connected");
        searchField->setProperty("text", QString());
        QTest::qWait(100);
        require(nav->property("query").toString().isEmpty(), "Clearing search did not reset results");
        const auto catalog = nav->property("catalog").toList();
        require(catalog.size() > 40, "Too few discovered settings modules");
        QFile inventory(qEnvironmentVariable("SEQUOIA_SETTINGS_VERIFY") + QStringLiteral("/inventory.json"));
        require(inventory.open(QIODevice::WriteOnly), "Cannot write inventory");
        inventory.write(QJsonDocument(QJsonArray::fromVariantList(catalog)).toJson());
        inventory.close();
        QSet<QString> all, grouped;
        for (const auto &entry : catalog) all.insert(entry.toMap().value(QStringLiteral("id")).toString());
        require(all.size() == catalog.size(), "Duplicate modules in catalog");
        require(call(nav, "browse", QStringLiteral("all")), "All Settings navigation failed");
        require(nav->property("entries").toList().size() == catalog.size(), "All Settings omits modules");
        capture(QStringLiteral("all"));
        if (all.contains(QStringLiteral("sequoia-test-extra"))) {
            open(nav, QStringLiteral("sequoia-test-extra"));
            require(nav->property("moduleOpen").toBool(), "Uncategorized third-party module did not open");
            require(back(nav), "Cannot return from third-party module");
        }
        for (const auto &group : nav->property("groups").toList()) {
            require(call(nav, "browse", group.toMap().value(QStringLiteral("id")).toString()), "Category navigation failed");
            for (const auto &entry : nav->property("entries").toList()) grouped.insert(entry.toMap().value(QStringLiteral("id")).toString());
        }
        require(grouped == all, "Category union does not cover All Settings");
        require(!call(nav, "browse", QStringLiteral("unknown-category")), "Invalid category was accepted");
        require(call(nav, "browse", QStringLiteral("appearance")), "Appearance navigation failed");
        capture(QStringLiteral("appearance"));
        // Every module is searchable by stable ID, including category owners.
        for (const auto &id : all) {
            require(call(nav, "search", id), "Search failed");
            bool found = false;
            for (const auto &entry : nav->property("entries").toList()) found |= entry.toMap().value(QStringLiteral("id")).toString() == id;
            require(found, "Search lost an installed module");
        }
        require(call(nav, "search", QStringLiteral("[.* impossible-query")), "Literal search failed");
        require(nav->property("entries").toList().isEmpty(), "Search unexpectedly treats text as a regular expression");
        capture(QStringLiteral("empty"));
        require(call(nav, "search", QStringLiteral("color")), "Keyword search failed");
        capture(QStringLiteral("search"));
        open(nav, QStringLiteral("kcm_colors"));
        require(nav->property("moduleOpen").toBool(), "Native Colors module did not open");
        require(!window->findChildren<QQuickWidget *>().isEmpty(), "Missing native module host");
        capture(QStringLiteral("colors"));
        QObject *kcm = nullptr;
        for (auto candidate : window->findChildren<QObject *>()) {
            if (candidate->inherits("KCModule")) { kcm = candidate; break; }
        }
        require(kcm, "KDE KCModule was not loaded");
        require(kcm->setProperty("needsSave", true), "Cannot exercise unsaved state");
        auto cancel = [] {
            auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            require(dialog, "KDE unsaved-change dialog did not appear");
            for (auto button : dialog->findChildren<QAbstractButton *>()) {
                if (button->text().remove(QLatin1Char('&')) == QStringLiteral("Cancel")) { button->click(); return; }
            }
            require(false, "Cancel action missing");
        };
        QTimer::singleShot(100, qApp, cancel);
        require(!back(nav), "Back discarded unsaved changes after Cancel");
        require(nav->property("moduleOpen").toBool(), "Cancel lost the current page");
        QTimer::singleShot(100, qApp, cancel);
        require(!call(nav, "browse", QStringLiteral("devices")), "Category switch ignored Cancel");
        QTimer::singleShot(100, qApp, cancel);
        require(!call(nav, "search", QStringLiteral("keyboard")), "Search ignored Cancel");
        require(nav->property("query").toString() == QStringLiteral("color"), "Canceled search mutated navigation");
        // Reset only the test flag; no settings are saved.
        kcm->setProperty("needsSave", false);
        require(back(nav), "Return to search failed");
        require(nav->property("query").toString() == QStringLiteral("color"), "Back lost search context");
        require(call(nav, "browse", QStringLiteral("devices")), "Devices navigation failed");
        open(nav, QStringLiteral("kcm_keyboard"));
        require(nav->property("moduleOpen").toBool(), "Native QWidget keyboard module did not open");
        capture(QStringLiteral("keyboard"));
        require(back(nav), "Back from keyboard failed");
        const auto captureGroup = qEnvironmentVariable("SEQUOIA_SETTINGS_CAPTURE_GROUP");
        if (!captureGroup.isEmpty()) {
            require(call(nav, "browse", captureGroup), "Capture category missing");
            const auto entries = nav->property("entries").toList();
            require(!entries.isEmpty(), "Capture category is empty");
            for (const auto &entry : entries) {
                const auto id = entry.toMap().value(QStringLiteral("id")).toString();
                fprintf(stderr, "Opening %s\n", qPrintable(id));
                fflush(stderr);
                open(nav, id);
                // KDE's external-app host has no KCModule metadata; the
                // navigation controller reports it as open with an empty ID.
                require(nav->property("moduleOpen").toBool(), "Category page did not open");
                if (id != QStringLiteral("sequoia-test-extra")) {
                    require(nav->property("currentModule").toString() == id, "Wrong category page opened");
                }
                for (auto quick : window->findChildren<QQuickWidget *>()) {
                    if (quick->isVisible()) require(quick->status() != QQuickWidget::Error && quick->rootObject(), "Category QML page failed to load");
                }
                capture(id);
                if (id == QStringLiteral("kcm_kwindecoration")) {
                    bool clicked = false;
                    for (auto quick : window->findChildren<QQuickWidget *>()) {
                        if (!quick->isVisible() || !quick->rootObject()) continue;
                        for (auto item : items(quick->rootObject())) {
                            if (item->isVisible() && item->inherits("QQuickAbstractButton")
                                && item->property("text").toString().contains(QStringLiteral("Configure Titlebar Buttons"))) {
                                QTest::mouseClick(quick->quickWindow(), Qt::LeftButton, Qt::NoModifier,
                                    item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
                                clicked = true;
                                break;
                            }
                        }
                        if (clicked) break;
                    }
                    require(clicked, "Cannot open titlebar button subpage");
                    QTest::qWait(600);
                    capture(QStringLiteral("appearance-titlebar-buttons"));
                }
                if (id == QStringLiteral("kcm_fonts")) {
                    window->setMinimumSize(0, 0);
                    window->resize(800, 600);
                    QTest::qWait(200);
                    capture(QStringLiteral("appearance-fonts-compact"));
                    window->resize(1100, 790);
                    QTest::qWait(200);
                }
                // Some native pages mark defaults dirty during initialization
                // in an empty profile (for example Spell Check). Discard those
                // changes through the real dialog; never apply them.
                QTimer::singleShot(100, qApp, [] {
                    auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                    if (!dialog) return;
                    for (auto button : dialog->findChildren<QAbstractButton *>()) {
                        if (button->text().remove(QLatin1Char('&')) == QStringLiteral("Discard")) {
                            button->click();
                            return;
                        }
                    }
                    require(false, "Unexpected dialog while leaving captured page");
                });
                require(back(nav), "Cannot leave category page");
            }
            fprintf(stderr, "PASS: opened and captured %lld pages in %s\n", static_cast<long long>(entries.size()), qPrintable(captureGroup));
        }
        require(call(nav, "browse", QStringLiteral("home")), "Home navigation failed");
        // Exercise a real card click; QML delegates must be wired to the backend.
        QQuickWidget *overview = nullptr;
        QQuickItem *card = nullptr;
        for (auto quick : window->findChildren<QQuickWidget *>()) {
            if (!quick->isVisible() || !quick->rootObject()) continue;
            for (auto item : items(quick->rootObject())) {
                if (item->property("modelData").toMap().value(QStringLiteral("id")) == QStringLiteral("appearance")
                    && item->inherits("QQuickAbstractButton") && item->width() > 250) { overview = quick; card = item; }
            }
        }
        require(overview && card, "Appearance card missing");
        QTest::mouseClick(overview->quickWindow(), Qt::LeftButton, Qt::NoModifier,
                         card->mapToScene(QPointF(card->width()/2, card->height()/2)).toPoint());
        QTest::qWait(200);
        require(nav->property("selectedGroup").toString() == QStringLiteral("appearance"), "Card click did not navigate");
        require(back(nav), "Return home failed");
        // The command-line and D-Bus entry path remains the upstream path.
        QObject *sidebar = nav->parent();
        require(QMetaObject::invokeMethod(sidebar, "loadModule", Qt::DirectConnection, Q_ARG(QModelIndex, QModelIndex()), Q_ARG(QStringList, QStringList())), "Native loadModule entry point missing");
        require(nav->property("home").toBool(), "Invalid module changed the overview");
        window->setMinimumSize(0, 0);
        window->resize(800, 600);
        QTest::qWait(200);
        capture(QStringLiteral("compact"));
        fprintf(stderr, "PASS: %lld unique modules; complete category and search coverage; native QML and QWidget pages; Cancel preserves pending changes; real card navigation\n", static_cast<long long>(catalog.size()));
        QCoreApplication::exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
