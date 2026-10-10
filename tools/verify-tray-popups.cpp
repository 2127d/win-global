// SPDX-License-Identifier: MIT
// Runs only in a private plasmashell: exercise real panel clicks and popup ownership.
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTest>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

static QList<QQuickItem *> items(QQuickItem *root) {
    QList<QQuickItem *> out{root};
    for (qsizetype i=0; i<out.size(); ++i) out.append(out[i]->childItems());
    return out;
}
static void fail(const char *message) {
    fprintf(stderr,"FAIL: %s\n",message); std::_Exit(1);
}
static int popups() {
    int count=0;
    for (auto window : QGuiApplication::allWindows()) {
        if (window->isVisible() && QByteArray(window->metaObject()->className()).contains("AppletPopup")) {
            ++count;
            fprintf(stderr,"POPUP %s %dx%d\n",window->metaObject()->className(),window->width(),window->height());
        }
    }
    return count;
}
static void start() {
    if (!qEnvironmentVariableIsSet("SEQUOIA_TRAY_POPUP_TEST")) return;
    QTimer::singleShot(5000, qApp, [] {
        QQuickItem *tray=nullptr;
        for (auto w : QGuiApplication::allWindows()) {
            if (auto quick=qobject_cast<QQuickWindow *>(w)) {
                for (auto item: items(quick->contentItem())) {
                    if (item->property("systemTrayState").isValid()) tray=item;
                }
            }
        }
        if (!tray) fail("tray did not load in the panel");
        auto state=tray->property("systemTrayState").value<QObject *>();
        auto panel=tray->window();
        if (!state || !panel) fail("missing tray state or panel window");
        for (int round=0; round<3; ++round) {
            for (const auto name: {"org.sequoia.volume","org.sequoia.notifications","org.sequoia.clipboard"}) {
                QQuickItem *icon=nullptr;
                for (auto item: items(tray)) {
                    if (item->property("itemId").toString()==QString::fromLatin1(name)
                        && item->isVisible() && item->width()>0 && item->height()>0) icon=item;
                }
                if (!icon) fail("visible tray icon not found");
                QTest::mouseClick(panel, Qt::LeftButton, Qt::NoModifier,
                                  icon->mapToScene(QPointF(icon->width()/2,icon->height()/2)).toPoint());
                QTest::qWait(900);
                bool expanded=state->property("expanded").toBool();
                auto active=state->property("activeApplet").value<QObject *>();
                auto applet=active ? active->property("_plasma_applet").value<QObject *>() : nullptr;
                auto activeId=applet ? applet->property("pluginName").toString() : QString();
                int count=popups();
                fprintf(stderr,"CLICK %s expanded=%d active=%s windows=%d\n",name,expanded,qPrintable(activeId),count);
                if (!expanded || activeId!=QString::fromLatin1(name) || count!=1)
                    fail("one click must leave exactly one shared popup open on the requested item");
                auto full=active->property("fullRepresentationItem").value<QQuickItem *>();
                if (!full || !full->window() || !full->window()->isVisible()) fail("detail page is not in the visible popup");
                // An in-popup click must not dismiss it; a different activated window must.
                QTest::mouseClick(full->window(),Qt::LeftButton,Qt::NoModifier,QPoint(6,6));
                QTest::qWait(250);
                if (!state->property("expanded").toBool()) fail("popup closed on an inside click");
            }
            QQuickWindow outside;
            outside.setColor(Qt::white);
            outside.resize(180,120); outside.show();
            QTest::qWait(200);
            outside.requestActivate();
            QTest::mouseClick(&outside,Qt::LeftButton,Qt::NoModifier,QPoint(80,60));
            QTest::qWait(700);
            if (state->property("expanded").toBool() || popups()!=0) fail("popup did not close when another window was activated");
            outside.hide();
            QTest::qWait(200);
        }
        // The Control Center expander must use that same single popup.
        QQuickItem *expander=nullptr;
        for (auto item: items(tray)) {
            auto context=QQmlEngine::contextForObject(item);
            if (context && context->baseUrl().fileName()=="ExpanderArrow.qml" && item->isVisible()) expander=item;
        }
        if (!expander) fail("Control Center expander missing");
        QTest::mouseClick(panel,Qt::LeftButton,Qt::NoModifier,
                          expander->mapToScene(QPointF(expander->width()/2,expander->height()/2)).toPoint());
        QTest::qWait(900);
        if (!state->property("expanded").toBool() || popups()!=1) fail("Control Center did not open one popup");
        fprintf(stderr,"PASS: repeated single-click opening, item switching, inside clicks, outside dismissal and one popup surface\n");
        std::_Exit(0);
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
