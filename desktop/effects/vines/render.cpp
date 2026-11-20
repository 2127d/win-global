// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include <QGuiApplication>
#include <QImage>
#include <QDir>
int main(int argc,char **argv) {
    QGuiApplication app(argc,argv);
    const QString output=argc>1?QString::fromLocal8Bit(argv[1]):".";
    QDir().mkpath(output);
    for(int i=0;i<90;++i) {
        QImage image(1380,594,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
        QPainter painter(&image);paintJungle(painter,image.size(),i*5.8/90);painter.end();
        if(!image.save(output+QString("/frame-%1.png").arg(i,3,10,QChar('0'))))return 1;
    }
}
