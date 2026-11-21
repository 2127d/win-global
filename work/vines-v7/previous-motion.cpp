// SPDX-License-Identifier: GPL-2.0-or-later
#include "motion.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr double pi=3.14159265358979323846;
constexpr double spacing=80, stride=spacing*3, stepTime=.8, overlap=.14;
constexpr double vineLength=120, catchOffset=-90;
double smooth(double u) {u=std::clamp(u,0.0,1.0);return u*u*(3-2*u);}
QPointF rotated(QPointF v,double degrees) {
    const double a=degrees*pi/180;
    return QPointF(v.x()*std::cos(a)-v.y()*std::sin(a),v.x()*std::sin(a)+v.y()*std::cos(a));
}
double rotation(double gait) {return -63.5+51.5*std::cos(pi*gait);}
QPointF bodyAt(double anchor,double t,int step) {
    // One complete body pendulum takes TWO catches. Hips lead on the first
    // ascent and trail on the return; descent is deep and fastest mid-swing.
    // Position and velocity remain continuous at each transfer.
    return QPointF(anchor+catchOffset+stride*(t-.25*std::sin(2*pi*t)/(2*pi))+25*(1-std::cos(pi*(step+t))),
                   160+115*std::pow(std::sin(pi*t),2));
}
QPointF shoulderAt(double anchor,double t,int hand,int step) {
    return bodyAt(anchor,t,step)+rotated(hand==0?QPointF(2,-14):QPointF(19,-8),rotation(step+t));
}
QPointF gripAt(double anchor,double age) {
    constexpr double release=1+overlap, ramp=.09;
    constexpr double speed=.9/(release-ramp/2);
    double angle=0;
    if(age>0 && age<=release)
        angle=speed*(age<ramp?age*age/(2*ramp):age-ramp/2);
    else if(age>release) {
        const double seconds=(age-release)*stepTime;
        angle=std::exp(-1.6*seconds)*(.9*std::cos(4*seconds)+(speed/stepTime+1.6*.9)/4*std::sin(4*seconds));
    }
    return QPointF(anchor+vineLength*std::sin(angle),vineLength*std::cos(angle));
}
double hermite(double a,double b,double da,double db,double t) {
    const double t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*a+(t3-2*t2+t)*da+(-2*t3+3*t2)*b+(t3-t2)*db;
}
struct Polar {double angle,radius,angularSpeed,radialSpeed;};
Polar polar(QPointF vector,QPointF velocity) {
    const double r=std::hypot(vector.x(),vector.y());
    return {std::atan2(vector.y(),vector.x()),r,
            (vector.x()*velocity.y()-vector.y()*velocity.x())/(r*r),
            (vector.x()*velocity.x()+vector.y()*velocity.y())/r};
}
QPointF circulatingHand(double anchor,double t,int hand,int step) {
    // From the trailing grip: behind -> down -> forward -> up. In screen
    // coordinates decreasing polar angle traces the counterclockwise loop.
    const auto releaseVector=[&](double phase) {return gripAt(anchor-stride,phase+1)-shoulderAt(anchor,phase,hand,step);};
    const auto catchVector=[&](double phase) {return QPointF(anchor+stride,vineLength)-shoulderAt(anchor,phase,hand,step);};
    constexpr double e=.00001, duration=1-overlap;
    const auto a=polar(releaseVector(overlap),(releaseVector(overlap+e)-releaseVector(overlap-e))/(2*e));
    auto b=polar(catchVector(1),(catchVector(1+e)-catchVector(1-e))/(2*e));
    while(b.angle>a.angle)b.angle-=2*pi;
    const double u=(t-overlap)/duration;
    const double angle=hermite(a.angle,b.angle,a.angularSpeed*duration,b.angularSpeed*duration,u);
    const double radius=hermite(a.radius,b.radius,a.radialSpeed*duration,b.radialSpeed*duration,u)
                        -28*std::pow(std::sin(pi*u),2);
    return shoulderAt(anchor,t,hand,step)+QPointF(radius*std::cos(angle),radius*std::sin(angle));
}
}

double JunglePose::deployment(double x) const {
    const double delay=std::clamp(x/width,0.0,1.0)*.4;
    const double down=smooth((localTime-delay)/1.1);
    const double up=smooth((localTime-(1.8+journey+.2)-delay)/1.0);
    return down*(1-up);
}

JunglePose junglePose(double width,double seconds) {
    JunglePose pose;pose.width=std::max(1.0,width);
    for(double x=-480;x<pose.width+1100;x+=spacing)pose.anchors.append(x);
    // Always pass two intervening vines, then catch the third. Identical
    // spacing and cadence on every pass replace the random route entirely.
    pose.route.append(0);
    while(pose.anchors[pose.route.last()]<pose.width+720)pose.route.append(pose.route.last()+3);
    pose.journey=(pose.route.size()-2)*stepTime;
    pose.cycle=1.8+pose.journey+4.2;
    pose.pass=int(std::max(0.0,seconds)/pose.cycle);
    pose.localTime=std::fmod(std::max(0.0,seconds),pose.cycle);
    const double travel=(pose.localTime-1.8)/stepTime;
    pose.visible=travel>=0 && travel<pose.route.size()-2;
    for(double x:pose.anchors)pose.grips.append(QPointF(x,vineLength));
    for(int j=0;j<pose.route.size();++j)
        pose.grips[pose.route[j]]=gripAt(pose.anchors[pose.route[j]],travel-(j-1));
    if(!pose.visible)return pose;
    const int step=int(travel),j=step+1;
    const double t=travel-step,anchor=pose.anchors[pose.route[j]];
    pose.progress=t;pose.support=step%2;pose.supportVine=pose.route[j];pose.nextVine=pose.route[j+1];
    pose.gait=step+t;pose.rotation=rotation(pose.gait);pose.body=bodyAt(anchor,t,step);
    pose.hands[pose.support]=pose.grips[pose.supportVine];
    pose.heldVines[pose.support]=pose.supportVine;
    const int free=1-pose.support;
    if(t<=overlap) {
        pose.heldVines[free]=pose.route[j-1];
        pose.hands[free]=pose.grips[pose.route[j-1]];
    } else {
        pose.hands[free]=circulatingHand(anchor,t,free,step);
        pose.armFlex[free]=std::pow(std::sin(pi*(t-overlap)/(1-overlap)),2);
    }
    return pose;
}
