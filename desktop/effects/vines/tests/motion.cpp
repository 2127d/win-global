// SPDX-License-Identifier: GPL-2.0-or-later
#include "../motion.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
static void require(bool value,const char *message) {
    if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static double distance(QPointF a,QPointF b) {return std::hypot(a.x()-b.x(),a.y()-b.y());}
static QPointF shoulder(const JunglePose &p,int hand) {
    const auto v=hand==0?QPointF(2,-14):QPointF(19,-8);const double r=p.rotation*std::acos(-1.)/180;
    return p.body+QPointF(v.x()*std::cos(r)-v.y()*std::sin(r),v.x()*std::sin(r)+v.y()*std::cos(r));
}
int main() {
    for(double speed:{.25,1.,2.,3.}) {
        const auto base=junglePose(1600,0), timed=junglePose(1600,0,speed,60);
        require(std::abs(timed.cycle-(1.8+base.journey/speed+1.6+60))<1e-7,"speed and pause independently set visit cadence");
        for(double t=0;t<base.journey;t+=.031) {
            const auto expected=junglePose(1600,1.8+t);
            const auto actual=junglePose(1600,1.8+t/speed,speed,60);
            require(distance(expected.body,actual.body)<1e-6,"speed preserves swing geometry and catches");
        }
        const auto resting=junglePose(1600,timed.cycle-30,speed,60);
        require(!resting.visible && resting.deployment(1600)==0,"long pause keeps monkey and vines fully retracted");
        require(junglePose(1600,timed.cycle+1.9,speed,60).visible,"next visit starts after configured pause");
    }
    double reach=0,minTurn=0,maxTurn=-1e9;
    const auto back=junglePose(1600,1.8),bottom=junglePose(1600,2.2),forward=junglePose(1600,2.6),repeat=junglePose(1600,3.4);
    require(back.rotation>-20 && forward.rotation<-100,"hips and feet alternate between trailing and leading");
    require(std::abs(repeat.rotation-junglePose(1600,5.0).rotation)<.001,"body pose repeats after two catches");
    require(bottom.body.y()-back.body.y()>60,"deep suspension through the middle of the swing");
    for(double width:{800.,1600.,2560.,5120.}) {
        const auto initial=junglePose(width,0),entering=junglePose(width,1.8);
        require(!initial.visible && initial.deployment(width/2)==0,"canopy begins retracted");
        require(entering.visible && entering.deployment(width)==1,"canopy descends before entry");
        require(initial.journey<(width+1100)/200,"faster traversal cadence");
        require(entering.grips[entering.supportVine].x()<entering.anchors[entering.supportVine]-20,"entry vine starts already pulled back");
        require(entering.heldVines[1-entering.support]<0,"entry begins suspended from one hand");
        const auto caught=junglePose(width,2.6),loaded=junglePose(width,2.6+.1);
        require(distance(caught.grips[caught.supportVine],QPointF(caught.anchors[caught.supportVine],105))<.0001,"new vine is upright at first contact");
        require(loaded.grips[loaded.supportVine].x()<loaded.anchors[loaded.supportVine]-15 && loaded.grips[loaded.supportVine].y()>110,"hand pulls its new vine toward the body and down after contact");
        auto last=entering;int catches=0,overlaps=0,circles=0;
        for(double time=1.801;time<1.8+initial.journey;time+=.001) {
            const auto pose=junglePose(width,time);
            require(pose.nextVine-pose.supportVine==3,"every catch skips exactly two intervening vines");
            require(pose.grips[pose.nextVine]==QPointF(pose.anchors[pose.nextVine],105),"next vine stays vertical until touched");
            for(int hand=0;hand<2;++hand) {
                reach=std::max(reach,distance(pose.hands[hand],shoulder(pose,hand)));
                if(pose.heldVines[hand]>=0)
                    require(distance(pose.hands[hand],pose.grips[pose.heldVines[hand]])<1e-7,"held hand remains attached to its vine");
                else if(last.heldVines[hand]<0) {
                    const auto a=last.hands[hand]-shoulder(last,hand),b=pose.hands[hand]-shoulder(pose,hand);
                    const double turn=std::atan2(a.x()*b.y()-a.y()*b.x(),a.x()*b.x()+a.y()*b.y());
                    minTurn=std::min(minTurn,turn);maxTurn=std::max(maxTurn,turn);
                    require(pose.progress<.20 || turn<0,"free arm sweeps counterclockwise after releasing the trailing vine momentum");++circles;
                }
                require(distance(pose.hands[hand],last.hands[hand])<3,"hands do not jump at catch or release");
            }
            require(std::abs(pose.loadShare[0]+pose.loadShare[1]-1)<1e-6,"weight is carried by the gripping hands");
            if(last.heldVines[1-pose.support]>=0 && pose.heldVines[1-pose.support]<0) {
                const double boundary=time-(pose.progress-.125)*.8,e=.00001;
                const auto a=junglePose(width,boundary-e),b=junglePose(width,boundary),c=junglePose(width,boundary+e);
                const int h=1-pose.support;
                require(distance((b.hands[h]-a.hands[h])/e,(c.hands[h]-b.hands[h])/e)<2,"hand velocity is continuous at release");
                require(a.armFlex[h]<.0001 && c.armFlex[h]<.0001,"elbow does not pop on release");
            }
            if(pose.heldVines[0]>=0 && pose.heldVines[1]>=0) {
                ++overlaps;
                require(pose.suspension.x()>std::min(pose.hands[0].x(),pose.hands[1].x()) && pose.suspension.x()<std::max(pose.hands[0].x(),pose.hands[1].x()),"body stays between the two grips during transfer");
            }
            for(int vine=0;vine<pose.grips.size();++vine) {
                const auto length=distance(pose.grips[vine],QPointF(pose.anchors[vine],0));
                require(length>=105-1e-6 && length<=115+1e-6,"vine stretches only slightly under load");
                require(distance(pose.grips[vine],last.grips[vine])<2,"vines do not jump at catch or release");
            }
            require(pose.body.x()>last.body.x() && distance(pose.body,last.body)<1,"body keeps flowing forward");
            if(pose.supportVine!=last.supportVine) {
                require(pose.support!=last.support,"hands alternate");++catches;
                // Compare velocities across the handoff at its exact boundary.
                const double boundary=time-pose.progress*.8,e=.00001;
                const auto a=junglePose(width,boundary-e),b=junglePose(width,boundary),c=junglePose(width,boundary+e);
                require(distance((b.body-a.body)/e,(c.body-b.body)/e)<1,"body velocity is continuous at handoff");
                require(std::abs((b.rotation-a.rotation)/e-(c.rotation-b.rotation)/e)<1,"body pitch velocity is continuous at handoff");
                for(int h=0;h<2;++h)
                    require(distance((b.hands[h]-a.hands[h])/e,(c.hands[h]-b.hands[h])/e)<2,"hand velocity is continuous at catch");
            }
            last=pose;
        }
        require(catches>2 && overlaps>100 && circles>100,"repeated circles with a two-hand transfer");
        require(last.body.x()>width+200,"monkey clears screen before retracting");
        const auto retract=junglePose(width,1.8+initial.journey+.7);
        require(!retract.visible && retract.deployment(width/2)>0 && retract.deployment(width/2)<1,"retract after exit");
        const auto quiet=junglePose(width,initial.cycle-.5);
        require(!quiet.visible && quiet.deployment(0)==0 && quiet.deployment(width)==0,"fully retracted between passes");
        const auto next=junglePose(width,initial.cycle+.8);
        require(!next.visible && next.deployment(0)>0 && next.deployment(width)<1,"descend before next pass");
        require(next.anchors==initial.anchors && next.route==initial.route,"fixed spacing and route on every pass");
    }
    std::printf("Maximum arm reach %.2f, free arm angular increments %.5f .. %.5f\n",reach,minTurn,maxTurn);
    require(reach<190,"every catch and circular reach stays within arm length");
    std::puts("PASS: counterclockwise recovery sweeps, overlapping catches, upright untouched vines, fixed two-vine skips, smooth body/hand velocities and canopy lifecycle");
}
