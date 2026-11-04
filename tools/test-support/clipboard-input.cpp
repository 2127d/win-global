// SPDX-License-Identifier: MIT
// Test-only pointer input for the private virtual compositor.
#include <libei.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <unistd.h>
#include <cstdlib>
int main(int argc,char**argv){
 QCoreApplication app(argc,argv); if(argc<3 || qEnvironmentVariable("SEQUOIA_CLIPBOARD_TEST_PRIVATE") != "1") return 1;
 auto call=[](const char*m){return QDBusMessage::createMethodCall("org.kde.KWin","/org/kde/KWin/EIS/RemoteDesktop","org.kde.KWin.EIS.RemoteDesktop",m);};
 auto req=call("connectToEIS");req<<2;auto reply=QDBusConnection::sessionBus().call(req);
 if(reply.type()!=QDBusMessage::ReplyMessage||reply.arguments().size()!=2)return 2;
 int cookie=reply.arguments()[1].toInt();auto fd=qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments()[0]);
 ei*ctx=ei_new_sender(nullptr);ei_configure_name(ctx,"Sequoia isolated clipboard test");ei_setup_backend_fd(ctx,dup(fd.fileDescriptor()));
 ei_device *pointer=nullptr,*buttons=nullptr;
 auto dispatch=[&](){ei_dispatch(ctx);while(auto e=ei_get_event(ctx)){
 if(ei_event_get_type(e)==EI_EVENT_SEAT_ADDED)ei_seat_bind_capabilities(ei_event_get_seat(e),EI_DEVICE_CAP_POINTER_ABSOLUTE,EI_DEVICE_CAP_BUTTON,EI_DEVICE_CAP_SCROLL,nullptr);
 if(ei_event_get_type(e)==EI_EVENT_DEVICE_RESUMED){auto d=ei_event_get_device(e);ei_device_start_emulating(d,0);if(ei_device_has_capability(d,EI_DEVICE_CAP_POINTER_ABSOLUTE))pointer=ei_device_ref(d);if(ei_device_has_capability(d,EI_DEVICE_CAP_BUTTON))buttons=ei_device_ref(d);}
 ei_event_unref(e);}};
 for(int i=0;i<100&&(!pointer||!buttons);i++){dispatch();usleep(10000);}
 if(pointer&&buttons){ei_device_pointer_motion_absolute(pointer,atof(argv[1]),atof(argv[2]));ei_device_frame(pointer,ei_now(ctx));dispatch();usleep(100000);if(argc==3){ei_device_button_button(buttons,0x110,true);ei_device_frame(buttons,ei_now(ctx));dispatch();usleep(80000);ei_device_button_button(buttons,0x110,false);ei_device_frame(buttons,ei_now(ctx));dispatch();usleep(100000);}else if(QString(argv[3]).startsWith("scroll")){ei_device_scroll_discrete(pointer,0,QString(argv[3])=="scrollDown"?1200:-1200);ei_device_frame(pointer,ei_now(ctx));dispatch();usleep(200000);}}
 ei_device_unref(pointer);ei_device_unref(buttons);ei_unref(ctx);auto end=call("disconnect");end<<cookie;QDBusConnection::sessionBus().call(end);
 return pointer&&buttons?0:3;
}
