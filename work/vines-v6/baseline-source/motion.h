// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QPointF>
#include <QVector>
struct JunglePose {
    QVector<double> anchors;
    QVector<QPointF> grips;
    QVector<int> route;
    QPointF body;
    QPointF hands[2];
    int heldVines[2]={-1,-1};
    double armFlex[2]={0,0};
    int support=0, supportVine=0, nextVine=0, pass=0;
    double progress=0, rotation=0, localTime=0, journey=0, cycle=0, width=0;
    bool visible=false;
    double deployment(double x) const;
};
JunglePose junglePose(double width,double seconds);
