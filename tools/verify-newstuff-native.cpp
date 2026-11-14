// SPDX-License-Identifier: MIT
// Test the real application's launcher and Get New action with a local provider.
#include <QApplication>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <QTest>
#include <QSet>
#include <cstdio>
#include <cstdlib>
static QList<QObject*> objects(){
    QList<QObject*> result,pending;QSet<QObject*> seen;
    for(auto*w:QApplication::topLevelWidgets())pending.append(w);
    for(auto*w:QGuiApplication::allWindows())pending.append(w);
    while(!pending.isEmpty()){
        auto*o=pending.takeLast();if(seen.contains(o))continue;seen.insert(o);result.append(o);
        pending.append(o->children());
        if(auto*i=qobject_cast<QQuickItem*>(o))for(auto*c:i->childItems())pending.append(c);
        if(auto*w=qobject_cast<QQuickWidget*>(o))if(w->rootObject())pending.append(w->rootObject());
        if(auto*w=qobject_cast<QQuickWindow*>(o))pending.append(w->contentItem());
    }
    return result;
}
static void start(){
    if(qEnvironmentVariableIsEmpty("NEWSTUFF_NATIVE_CAPTURE"))return;
    QTimer::singleShot(1800,qApp,[]{
        if(qEnvironmentVariableIsSet("NEWSTUFF_FROM_PLASMA")){
            bool found=false;
            for(auto*o:objects())if(o->objectName()=="plasmaStyleGetNew"){
                found=true;QMetaObject::invokeMethod(o,"clicked");break;
            }
            if(!found){fprintf(stderr,"FAIL: Plasma Style Get New button\n");std::_Exit(2);}
        }
        QTimer::singleShot(2000,qApp,[]{
            for(auto*o:objects())if(o->objectName()=="sequoiaNewStuffPage"){
                auto*i=qobject_cast<QQuickItem*>(o);if(!i||!i->window())continue;
                i->window()->grabWindow().save(qEnvironmentVariable("NEWSTUFF_NATIVE_CAPTURE"));
                fprintf(stderr,"PASS: actual native Get New window loads the shared presentation\n");std::_Exit(0);
            }
            fprintf(stderr,"FAIL: native Get New did not load shared page\n");std::_Exit(2);
        });
    });
}
Q_COREAPP_STARTUP_FUNCTION(start)
