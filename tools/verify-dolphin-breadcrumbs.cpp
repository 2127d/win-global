// SPDX-License-Identifier: MIT
// Exercise real KIO breadcrumb widgets through the compiled Sequoia style.
#include <QApplication>
#include <QStyleFactory>
#include <QStyle>
#include <QVBoxLayout>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLineEdit>
#include <QAbstractButton>
#include <QTest>
#include <KUrlNavigator>
#include <cstdio>
#include <cstdlib>
class DolphinUrlNavigator: public KUrlNavigator {
 Q_OBJECT
public: DolphinUrlNavigator(QWidget *parent):KUrlNavigator(nullptr,QUrl::fromLocalFile(qEnvironmentVariable("BREADCRUMB_TEST_PATH")),parent) {setShowFullPath(true);}
};
static void require(bool passed,const char *message) {if(!passed){fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(int argc,char**argv){
 QApplication app(argc,argv);app.setApplicationName("dolphin");app.setStyle(QStyleFactory::create("SequoiaEleven"));
 require(app.style()->objectName().contains("sequoia",Qt::CaseInsensitive),"Sequoia style loaded");
 QFile maps("/proc/self/maps");require(maps.open(QIODevice::ReadOnly),"read loaded libraries");
 require(maps.readAll().contains(qgetenv("BREADCRUMB_EXPECT_PLUGIN")),"expected style binary loaded");
 const bool dark=argc>2 && QByteArray(argv[2])=="dark";
 const int width=argc>3?atoi(argv[3]):700;
 const QColor window(dark?"#24262b":"#f2f3f5"),text(dark?"#f2f3f5":"#24262b");
 QPalette pal;for(auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}){
  pal.setColor(group,QPalette::Window,window);pal.setColor(group,QPalette::Base,dark?QColor("#1d1f23"):Qt::white);
  pal.setColor(group,QPalette::Text,text);pal.setColor(group,QPalette::WindowText,text);pal.setColor(group,QPalette::Button,window);pal.setColor(group,QPalette::ButtonText,text);pal.setColor(group,QPalette::Highlight,QColor("#9f81da"));
 }
 app.setPalette(pal);QWidget host;host.setAutoFillBackground(true);host.setFocusPolicy(Qt::StrongFocus);
 auto *layout=new QVBoxLayout(&host);layout->setContentsMargins(14,14,14,14);
 auto *nav=new DolphinUrlNavigator(&host);layout->addWidget(nav);host.resize(width,68);host.show();host.setFocus();QTest::qWait(60);
 const auto prefix=QString::fromLocal8Bit(argv[1]);
 auto capture=[&](QString suffix){auto image=host.grab().toImage();require(image.save(prefix+suffix+".png"),"save preview");return image;};
 auto checkCorners=[&](const QImage &image){const auto rect=nav->geometry();const qreal scale=image.devicePixelRatio();
  for(auto point:{rect.topLeft(),rect.topRight(),rect.bottomLeft(),rect.bottomRight()}){
   const auto color=image.pixelColor(qRound(point.x()*scale),qRound(point.y()*scale));
   require(color==image.pixelColor(qRound(3*scale),qRound(3*scale)),"corners expose surrounding toolbar");
  }
 };
 auto normal=capture("-breadcrumb");checkCorners(normal);
 // The quiet surface must still be visible when the bar has no focus.
 auto rect=nav->geometry();auto scale=normal.devicePixelRatio();
 require(normal.pixelColor(qRound((rect.right()-35)*scale),qRound(rect.center().y()*scale))!=window,"breadcrumb surface has contrast");
 int hovered=0;for(auto *button:nav->findChildren<QAbstractButton*>())if(button->isVisible()){
  QTest::mouseMove(button,button->rect().center());QTest::qWait(5);checkCorners(host.grab().toImage());++hovered;
 }
 require(hovered>0,"hovered real breadcrumb controls");capture("-hover");
 if(width>=650){bool clicked=false;for(auto *button:nav->findChildren<QAbstractButton*>())if(button->isVisible() && button->text().contains("Documents")){
  QTest::mouseClick(button,Qt::LeftButton);QTest::qWait(20);clicked=true;break;
 }require(clicked && nav->locationUrl().toLocalFile()==QFileInfo(qEnvironmentVariable("BREADCRUMB_TEST_PATH")).dir().absolutePath(),"breadcrumb click still navigates");}
 nav->setUrlEditable(true);QTest::qWait(20);auto *edit=nav->findChild<QLineEdit*>();require(edit && edit->isVisible(),"editable path visible");edit->setFocus();QTest::qWait(20);capture("-editable");
 edit->selectAll();QTest::keyClicks(edit,"/tmp");QTest::keyClick(edit,Qt::Key_Return);QTest::qWait(40);
 require(nav->locationUrl().toLocalFile().startsWith("/tmp"),"keyboard path navigation works");
 nav->setUrlEditable(false);host.setFocus();QTest::qWait(20);checkCorners(host.grab().toImage());
 require(!nav->isUrlEditable(),"return to breadcrumb mode");
 printf("PASS %s width=%d DPR=%.2f: rounded surface, %d hovered controls, editing and navigation\n",dark?"dark":"light",width,scale,hovered);
 return 0;
}
#include "main.moc"
