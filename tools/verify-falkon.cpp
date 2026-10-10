// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only startup hook. The runner isolates the profile, D-Bus and local fixtures.
#include <QBitArray>
#include "browserwindow.h"
#include "tabwidget.h"
#include "tabsearch.h"
#include "tabbedwebview.h"
#include "webpage.h"
#include "locationbar.h"
#include "loadrequest.h"
#include "sidebar.h"
#include "toolbutton.h"
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QListView>
#include <QToolButton>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QTest>
#include <QElapsedTimer>
#include <QWebEngineScript>
#include <QWebEngineProfile>
#include <cstdio>
#include <cstdlib>

static void require(bool ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static bool until(const std::function<bool()> &condition, int timeout = 15000) {
    QElapsedTimer timer; timer.start();
    while (!condition() && timer.elapsed() < timeout) QTest::qWait(50);
    return condition();
}
static QVariant js(QWebEnginePage *page, const QString &code) {
    QVariant result; bool finished = false;
    page->runJavaScript(code, QWebEngineScript::MainWorld, [&](const QVariant &v) { result = v; finished = true; });
    require(until([&] { return finished; }), "JavaScript callback");
    return result;
}
#include "verify-falkon-blocking.h"

static void startup() {
    if (qEnvironmentVariableIsEmpty("SEQUOIA_FALKON_VERIFY")) return;
    QTimer::singleShot(2000, qApp, [] {
        BrowserWindow *window = nullptr;
        for (auto *widget : QApplication::topLevelWidgets())
            if ((window = qobject_cast<BrowserWindow *>(widget))) break;
        require(window, "native browser window");
        window->resize(1280, 860); window->show(); window->activateWindow();
        QApplication::setActiveWindow(window);
        const QString output = qEnvironmentVariable("SEQUOIA_FALKON_VERIFY");
        auto capture = [&](const QString &name) { QTest::qWait(500); require(window->grab().save(output + "/" + name + ".png"), "capture"); };
        auto *tabs = window->tabWidget();
        auto *view = window->weView();
        require(until([&] { return !view->isLoading() && view->url().toString() == "falkon:speeddial"; }), "start page loaded");
        require(until([&] { return js(view->page(), "document.querySelectorAll('#quickdial .entry').length").toInt() == 4; }), "editable favorites loaded through native bridge");
        require(js(view->page(), "document.querySelector('h1').textContent").toString() == "Where to next?", "new start page");
        require(js(view->page(), "document.body.innerText.includes('%START-')").toBool() == false, "translated labels substituted");
        auto *searchButton = window->findChild<QToolButton *>("tabwidget-button-opentabs");
        require(searchButton && searchButton->isVisible(), "tab search visible with one tab");
        auto *searchBar = window->findChild<QWidget *>("websearchbar");
        require(!searchBar || !searchBar->isVisible(), "unified address bar by default");
        require(window->locationBar()->height() >= 34, "comfortable address target");
        if (qEnvironmentVariableIsSet("SEQUOIA_FALKON_PRIVATE")) {
            auto *badge = window->findChild<QLabel *>("private-browsing-icon");
            require(badge && badge->isVisible() && badge->text() == "Private", "visible private browsing badge");
            require(view->page()->profile()->isOffTheRecord(), "private mode retains off-the-record WebEngine profile");
        }
        capture("start");
        // Open real local documents in the native renderer.
        const QString fixtures = qEnvironmentVariable("SEQUOIA_FALKON_FIXTURES");
        auto *first = qobject_cast<WebTab *>(tabs->widget(tabs->currentIndex()));
        window->addTab(); auto *alpha = qobject_cast<WebTab *>(tabs->widget(tabs->currentIndex()));
        alpha->load(LoadRequest(QUrl::fromLocalFile(fixtures + "/alpha.html")));
        window->addTab(); auto *beta = qobject_cast<WebTab *>(tabs->widget(tabs->currentIndex()));
        beta->load(LoadRequest(QUrl::fromLocalFile(fixtures + "/beta.html")));
        require(until([&] { return alpha->title() == "Design notes" && beta->title() == "Reading list"; }), "local pages rendered");
        alpha->togglePinned(); QTest::qWait(200);
        auto openSearch = [&]() {
            QTest::mouseClick(searchButton, Qt::LeftButton);
            require(until([&] { return window->findChild<TabSearch *>(); }), "tab search opens");
            auto *dialog = window->findChild<TabSearch *>();
            auto *input = dialog->findChild<QLineEdit *>("tab-search-input");
            require(input, "search input");
            return dialog;
        };
        auto *dialog = openSearch();
        auto *input = dialog->findChild<QLineEdit *>("tab-search-input");
        auto *results = dialog->findChild<QListView *>("tab-search-results");
        input->setText("DESIGN"); QTest::qWait(100);
        require(results->model()->rowCount() == 1, "case-insensitive title search includes pinned tabs");
        require(dialog->grab().save(output + "/tab-search.png"), "search capture");
        QTest::keyClick(input, Qt::Key_Return); QTest::qWait(100);
        require(qobject_cast<WebTab *>(tabs->widget(tabs->currentIndex())) == alpha, "Enter activates pinned result");
        dialog = openSearch(); input = dialog->findChild<QLineEdit *>("tab-search-input"); results = dialog->findChild<QListView *>("tab-search-results");
        input->setText("no-such-tab"); QTest::qWait(100);
        require(results->model()->rowCount() == 0, "empty search result");
        QTest::keyClick(input, Qt::Key_Down); QTest::keyClick(input, Qt::Key_Return);
        require(dialog->isVisible(), "empty search does not switch tabs");
        input->setText("beta.html"); QTest::qWait(100);
        require(results->model()->rowCount() == 1, "URL search");
        tabs->requestCloseTab(beta->tabIndex()); QTest::qWait(150);
        require(results->model()->rowCount() == 0, "closed tab removed while search is open");
        QTest::keyClick(input, Qt::Key_Escape); QTest::qWait(150);
        require(!window->findChild<TabSearch *>(), "Escape dismisses and deletes search");
        QApplication::setActiveWindow(window); window->setFocus();
        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier | Qt::ShiftModifier);
        require(until([&] { return window->findChild<TabSearch *>(); }), "keyboard tab-search shortcut");
        window->findChild<TabSearch *>()->reject(); QTest::qWait(150);
        tabs->setCurrentIndex(first->tabIndex());
        auto *library = window->findChild<ToolButton *>("navigation-button-library");
        require(library && library->isVisible() && library->menu(), "Library toolbar button");
        QTest::mouseClick(library, Qt::LeftButton);
        require(until([&] { return library->menu()->isVisible(); }), "Library opens menu");
        library->menu()->actions().first()->trigger(); library->menu()->close();
        require(until([&] { return window->sideBarManager()->activeSideBar() == "Bookmarks"; }), "native bookmarks sidebar");
        capture("bookmarks"); window->sideBarManager()->showSideBar("Bookmarks");
        window->resize(720, 700); QTest::qWait(300);
        require(searchButton->isVisible() && window->locationBar()->width() > 180, "compact toolbar remains usable");
        require(js(first->webView()->page(), "document.documentElement.scrollWidth <= window.innerWidth").toBool(), "responsive start page has no horizontal overflow");
        capture("compact");
        // Real Speed Dial add action opens the existing editor through its isolated script world.
        js(first->webView()->page(), "document.getElementById('button-add-speed-dial').click()");
        require(until([&] { return js(first->webView()->page(), "!!document.getElementById('formUrl')").toBool(); }), "add favorite opens editor");
        capture("favorite-editor");
        js(first->webView()->page(), QString("document.getElementById('formUrl').value='%1'; document.getElementById('formTitle').value='Saved favorite'; document.getElementById('acceptEditBox').click()").arg(QUrl::fromLocalFile(fixtures + "/alpha.html").toString()));
        require(until([&] { return js(first->webView()->page(), "Array.from(document.querySelectorAll('.boxTitle')).some(e=>e.textContent==='Saved favorite')").toBool(); }), "favorite saved");
        first->webView()->reload();
        require(until([&] { return !first->isLoading() && js(first->webView()->page(), "Array.from(document.querySelectorAll('.boxTitle')).some(e=>e.textContent==='Saved favorite')").toBool(); }), "favorite survives page reload");
        fprintf(stderr, "PASS: native rendering, start page, tabs, pinned/title/URL search, live removal, empty results, shortcuts, bookmarks and compact layout\n");
        window->resize(1280, 860);
        verifyBlocking(window, output);
        window->close();
        QCoreApplication::exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(startup)
