// SPDX-License-Identifier: MIT
// Real NewStuff engine and controls, local provider, isolated XDG roots.
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QPalette>
#include <QQuickStyle>
#include <QImage>
#include <QTest>
#include <KLocalizedContext>
#include <QFile>
#include <QSet>
#include <cstdio>

static QObject *find(QObject *root,const QString &name) {
    QList<QObject*> pending{root};QSet<QObject*> seen;
    while(!pending.isEmpty()){
        auto*o=pending.takeLast();if(seen.contains(o))continue;seen.insert(o);
        if(o->objectName()==name)return o;
        pending.append(o->children());
        if(auto*i=qobject_cast<QQuickItem*>(o))for(auto*c:i->childItems())pending.append(c);
    }
    return nullptr;
}
static void require(bool ok,const char*what){if(!ok){fprintf(stderr,"FAIL: %s\n",what);exit(1);}}
int main(int argc,char **argv){
    QApplication app(argc,argv);
    app.setQuitOnLastWindowClosed(false);
    QPalette p=app.palette();const bool dark=qEnvironmentVariableIsSet("TEST_DARK");
    const bool glass=qEnvironmentVariableIsSet("TEST_GLASS");
    QColor bg=dark?QColor(37,38,41):QColor(242,243,245);if(glass)bg.setAlpha(168);
    p.setColor(QPalette::Window,bg);p.setColor(QPalette::Base,dark?QColor("#252629"):QColor("#ffffff"));
    p.setColor(QPalette::WindowText,dark?Qt::white:Qt::black);p.setColor(QPalette::Text,dark?Qt::white:Qt::black);
    p.setColor(QPalette::Button,dark?QColor("#303135"):QColor("#ffffff"));p.setColor(QPalette::ButtonText,dark?Qt::white:Qt::black);app.setPalette(p);
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl::fromLocalFile(argv[1]));require(!engine.rootObjects().isEmpty(),"load native NewStuff dialog");
    QObject *root=engine.rootObjects()[0];QObject *page=nullptr;
    for(int n=0;n<100&&!page;++n){QTest::qWait(50);page=find(root,"sequoiaNewStuffPage");}
    require(page,"shared NewStuff page is active");
    engine.rootContext()->setContextProperty("testPage",page);
    engine.rootContext()->setContextProperty("testDialog",root);
    auto js=[&](const QString &code){QQmlExpression e(engine.rootContext(),root,code);QVariant v=e.evaluate();if(e.hasError()){fprintf(stderr,"%s\n",qPrintable(e.error().toString()));exit(1);}return v;};
    auto wait=[&](const QString &code){for(int n=0;n<120;++n){if(js(code).toBool())return true;QTest::qWait(50);}return false;};
    require(wait("testPage.view.count === 3"),"local catalog loads all entries");
    require(wait("testPage.view.itemAtIndex(0) !== null"),"native entry delegate loads");
    js("testPage.engine.searchTerm = 'Sequoia Sample 2'");
    require(wait("testPage.view.count === 1"),"native search filters catalog");
    js("testPage.engine.searchTerm = ''");require(wait("testPage.view.count === 3"),"search clears");
    QTest::qWait(350);
    auto *window=qobject_cast<QQuickWindow*>(root);require(window,"native Quick window");
    window->grabWindow().save(QString::fromLocal8Bit(argv[2]));
    auto *filter=qobject_cast<QQuickItem*>(find(root,"newStuffAction1"));
    require(filter,"shared filter pill");
    const QPoint point=filter->mapToScene(QPointF(filter->width()/2,filter->height()/2)).toPoint();
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point);QTest::qWait(250);
    require(filter->property("menuOpen").toBool(),"filter menu opens on click");
    window->grabWindow().save(QString::fromLocal8Bit(argv[2])+".menu.png");
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point);QTest::qWait(300);
    require(!filter->property("menuOpen").toBool(),"second click closes without reopening");
    require(js("testPage.view.itemAtIndex(0).x < testPage.view.cellWidth").toBool(),"first card has no empty leading cells");
    // Use the real control's clicked signal and native action groups.
    require(js("testPage.actions[1].children.length === 3").toBool(),"native filter actions retained");
    js("testPage.actions[1].children[1].trigger()");
    require(wait("testPage.engine.filter === 1 && testPage.view.count === 0"),"installed filter works");
    js("testPage.actions[1].children[0].trigger()");require(wait("testPage.view.count === 3"),"all filter restores catalog");
    require(wait("testPage.view.itemAtIndex(0) !== null"),"delegate restored after filtering");
    js("testPage.view.itemAtIndex(0).showDetails()");QTest::qWait(350);
    QObject *back=nullptr;
    for(auto*c:root->findChildren<QObject*>("newStuffBack")){auto*i=qobject_cast<QQuickItem*>(c);if(i&&i->isVisible())back=c;}
    require(back,"circular back button appears in entry details");
    QMetaObject::invokeMethod(back,"clicked");QTest::qWait(350);
    // Install/remove only our plain local payload under the temporary data root.
    js("testPage.engine.installLatest(testPage.view.itemAtIndex(0).entry)");
    require(wait("testPage.view.itemAtIndex(0).entry.status === testDialog.installedStatus"),"native local download installs");
    js("testPage.engine.uninstall(testPage.view.itemAtIndex(0).entry)");
    require(wait("testPage.view.itemAtIndex(0).entry.status === testDialog.deletedStatus"),"native removal completes");
    const QString payload=qEnvironmentVariable("XDG_DATA_HOME")+"/sequoia-test-only/payload.txt";
    for(int n=0;n<120 && QFile::exists(payload);++n)QTest::qWait(50);
    fprintf(stderr,"Removed status: %d\n",js("testPage.view.itemAtIndex(0).entry.status").toInt());
    require(!QFile::exists(payload),"installed payload removed");
    fprintf(stderr,"PASS: shared NewStuff page, native search/filter, cards, details/back, local install/removal\n");
    return 0;
}
