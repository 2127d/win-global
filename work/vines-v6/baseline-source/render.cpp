// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include <QGuiApplication>
#include <QImage>
#include <QDir>
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    const QString output=argc>1?QString::fromLocal8Bit(argv[1]):".";
    QDir().mkpath(output);
    const double start=argc>2?QString::fromLocal8Bit(argv[2]).toDouble():0;
    const double duration=argc>3?QString::fromLocal8Bit(argv[3]).toDouble():20;
    const int frames=argc>4?QString::fromLocal8Bit(argv[4]).toInt():120;
    for(int i=0;i<frames;++i) {
        QImage image(1600,396,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
        QPainter painter(&image);paintJungle(painter,image.size(),start+i*duration/frames);painter.end();
        if(!image.save(output+QString("/frame-%1.png").arg(i,3,10,QChar('0'))))return 1;
    }
}
