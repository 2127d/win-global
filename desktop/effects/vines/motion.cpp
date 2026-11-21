// SPDX-License-Identifier: GPL-2.0-or-later
#include "motion.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr double pi=3.14159265358979323846;
constexpr double spacing=80, stride=spacing*3, stepTime=.8, overlap=.125;
constexpr double vineLength=105, pullAngle=-.22, stretch=10;
double smooth(double u) {u=std::clamp(u,0.0,1.0);return u*u*(3-2*u);}
QPointF rotated(QPointF v,double degrees) {
    const double a=degrees*pi/180;
    return {v.x()*std::cos(a)-v.y()*std::sin(a),v.x()*std::sin(a)+v.y()*std::cos(a)};
}
double rotation(double gait) {return -63.5+51.5*std::cos(pi*(gait-.08*smooth(gait/.15)));}
double hermite(double a,double b,double da,double db,double t) {
    const double t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*a+(t3-2*t2+t)*da+(-2*t3+3*t2)*b+(t3-t2)*db;
}
QPointF gripAt(double anchor,double age,bool entry=false) {
    constexpr double release=1+overlap;
    double angle=0,extension=0;
    if(entry && age<0) {angle=pullAngle;extension=stretch;}
    else if(age>=0 && age<overlap && !entry) {
        // Contact precedes all deflection: the new hand pulls the vine back
        // and down while the trailing hand still supports part of the load.
        const double load=smooth(age/overlap);
        angle=pullAngle*load;extension=stretch*load;
    } else if(age>=0 && age<=release) {
        const double u=entry?age/release:(age-overlap)/(release-overlap);
        const double swing=.5-.5*std::cos(pi*u);
        angle=pullAngle+(.85-pullAngle)*swing;
        extension=stretch-6*smooth(u);
    } else if(age>release) {
        const double seconds=(age-release)*stepTime;
        angle=std::exp(-1.8*seconds)*.85*(std::cos(4*seconds)+.45*std::sin(4*seconds));
        extension=4*std::exp(-4*seconds)*(1+4*seconds);
    }
    const double length=vineLength+extension;
    return {anchor+length*std::sin(angle),length*std::cos(angle)};
}
QPointF lowerIntersection(QPointF back,QPointF front,double backArm,double frontArm) {
    const QPointF delta=front-back;
    const double distance=std::hypot(delta.x(),delta.y());
    const QPointF along=delta/distance,down(-along.y(),along.x());
    const double x=(backArm*backArm-frontArm*frontArm+distance*distance)/(2*distance);
    return back+along*x+down*std::sqrt(std::max(0.0,backArm*backArm-x*x));
}
QPointF transferAt(double anchor,double t,int step) {
    const auto back=gripAt(anchor-stride,t+1,step==1);
    const auto front=gripAt(anchor,t);
    const double u=t/overlap;
    // The trailing arm extends as the leading elbow pulls in. This keeps the
    // shoulder between both grips while load shifts, with forward momentum.
    const double load=u-.1*std::sin(2*pi*u)/(2*pi);
    return lowerIntersection(back,front,140+40*load,135);
}
struct Polar {double angle,radius,angularSpeed,radialSpeed;};
Polar polar(QPointF v,QPointF velocity,bool fromVertical=false) {
    const double r=std::hypot(v.x(),v.y());
    const double spin=(v.x()*velocity.y()-v.y()*velocity.x())/(r*r);
    return {fromVertical?std::atan2(v.x(),v.y()):std::atan2(v.y(),v.x()),r,
        fromVertical?-spin:spin,(v.x()*velocity.x()+v.y()*velocity.y())/r};
}
QPointF suspensionAt(double anchor,double t,int step) {
    if(step>0 && t<=overlap)return transferAt(anchor,t,step);
    constexpr double e=.00001;
    const double start=step==0?0:overlap,duration=1-start;
    const auto grip=[&](double phase){return gripAt(anchor,phase,step==0);};
    const auto startVector=[&](double phase){return transferAt(anchor,phase,step)-grip(phase);};
    const auto endVector=[&](double phase){return transferAt(anchor+stride,phase-1,step+1)-grip(phase);};
    const Polar a=step==0?Polar{-.9,160,0,0}:polar(startVector(start),(startVector(start+e)-startVector(start-e))/(2*e),true);
    const auto b=polar(endVector(1),(endVector(1+e)-endVector(1-e))/(2*e),true);
    const double u=(t-start)/duration;
    const double angle=hermite(a.angle,b.angle,a.angularSpeed*duration,b.angularSpeed*duration,u);
    // Elbow extension lets gravity lengthen the suspension through the low
    // point, then the elbow bends on ascent to prepare the next catch.
    constexpr double low=.48;
    const double radius=u<low?hermite(a.radius,178,a.radialSpeed*duration*low,0,u/low)
        :hermite(178,b.radius,0,b.radialSpeed*duration*(1-low),(u-low)/(1-low));
    return grip(t)+QPointF(radius*std::sin(angle),radius*std::cos(angle));
}
QPointF bodyAt(double anchor,double t,int step) {
    return suspensionAt(anchor,t,step)-rotated(QPointF(10,-10),rotation(step+t));
}
QPointF shoulderAt(double anchor,double t,int hand,int step) {
    return bodyAt(anchor,t,step)+rotated(hand==0?QPointF(2,-14):QPointF(19,-8),rotation(step+t));
}
QPointF circulatingHand(double anchor,double t,int hand,int step) {
    constexpr double e=.00001;
    const double start=step==0?0:overlap,duration=1-start;
    const auto releaseVector=[&](double phase){return gripAt(anchor-stride,phase+1,step==1)-shoulderAt(anchor,phase,hand,step);};
    const auto catchVector=[&](double phase){return QPointF(anchor+stride,vineLength)-shoulderAt(anchor,phase,hand,step);};
    const Polar a=step==0?Polar{2.4,115,-2,0}:polar(releaseVector(start),(releaseVector(start+e)-releaseVector(start-e))/(2*e));
    auto b=polar(catchVector(1),(catchVector(1+e)-catchVector(1-e))/(2*e));
    while(b.angle>a.angle)b.angle-=2*pi;
    const double u=(t-start)/duration;
    // Recover the elbow through a low, forward-reaching pose near the
    // bottom of the body swing, rather than sweeping a large uniform circle.
    constexpr double middle=.40;
    double midAngle=pi/3;
    while(midAngle>a.angle)midAngle-=2*pi;
    const double midVelocity=(b.angle-a.angle)*1.05;
    const double angle=u<middle
        ?hermite(a.angle,midAngle,a.angularSpeed*duration*middle,midVelocity*middle,u/middle)
        :hermite(midAngle,b.angle,midVelocity*(1-middle),b.angularSpeed*duration*(1-middle),(u-middle)/(1-middle));
    constexpr double fold=.18;
    const double radius=u<fold
        ?hermite(a.radius,115,a.radialSpeed*duration*fold,0,u/fold)
        :hermite(115,b.radius,0,b.radialSpeed*duration*(1-fold),(u-fold)/(1-fold));
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
    for(double x=-320;x<pose.width+1100;x+=spacing)pose.anchors.append(x);
    pose.route.append(0);
    while(pose.anchors[pose.route.last()]<pose.width+720)pose.route.append(pose.route.last()+3);
    pose.journey=(pose.route.size()-2)*stepTime;
    pose.cycle=1.8+pose.journey+4.2;
    pose.pass=int(std::max(0.0,seconds)/pose.cycle);
    pose.localTime=std::fmod(std::max(0.0,seconds),pose.cycle);
    const double travel=(pose.localTime-1.8)/stepTime;
    pose.visible=travel>=0 && travel<pose.route.size()-2;
    for(double x:pose.anchors)pose.grips.append(QPointF(x,vineLength));
    // The entry vine was already pulled back offscreen. Every later vine
    // remains upright until the exact frame its hand catches it.
    for(int j=1;j<pose.route.size();++j)
        pose.grips[pose.route[j]]=gripAt(pose.anchors[pose.route[j]],travel-(j-1),j==1);
    if(!pose.visible)return pose;
    const int step=int(travel),j=step+1;
    const double t=travel-step,anchor=pose.anchors[pose.route[j]];
    pose.progress=t;pose.support=step%2;pose.supportVine=pose.route[j];pose.nextVine=pose.route[j+1];
    pose.gait=step+t;pose.rotation=rotation(pose.gait);pose.suspension=suspensionAt(anchor,t,step);pose.body=bodyAt(anchor,t,step);
    pose.hands[pose.support]=pose.grips[pose.supportVine];pose.heldVines[pose.support]=pose.supportVine;
    pose.loadShare[pose.support]=step==0?1:smooth(t/overlap);
    const int free=1-pose.support;
    if(step>0 && t<=overlap) {
        pose.heldVines[free]=pose.route[j-1];pose.hands[free]=pose.grips[pose.route[j-1]];
        pose.loadShare[free]=1-pose.loadShare[pose.support];
    } else {
        pose.hands[free]=circulatingHand(anchor,t,free,step);
        pose.armFlex[free]=std::pow(std::sin(pi*(step==0?t:(t-overlap)/(1-overlap))),2);
    }
    return pose;
}
