#include "motion.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
QPointF shoulder(const JunglePose &p,int h){auto v=h?QPointF(19,-8):QPointF(2,-14);double r=p.rotation*std::acos(-1)/180;return p.body+QPointF(v.x()*cos(r)-v.y()*sin(r),v.x()*sin(r)+v.y()*cos(r));}
int main(){double reach=0,minvx=1e9,maxstep=0;auto prev=junglePose(1600,1.8);for(int i=1;i<=2400;++i){auto p=junglePose(1600,1.8+i*.001);minvx=std::min(minvx,(p.body.x()-prev.body.x())*1000);maxstep=std::max(maxstep,std::hypot(p.body.x()-prev.body.x(),p.body.y()-prev.body.y()));for(int h=0;h<2;++h){auto v=p.hands[h]-shoulder(p,h);if(std::hypot(v.x(),v.y())>reach){reach=std::hypot(v.x(),v.y());if(reach>190)printf("REACH %.3f gait %.4f hand %d held %d\n",reach,p.gait,h,p.heldVines[h]);}}if(i%100==0)printf("gait %.3f body %.1f %.1f grip %.1f %.1f old %d\n",p.gait,p.body.x(),p.body.y(),p.grips[p.supportVine].x(),p.grips[p.supportVine].y(),p.heldVines[1-p.support]);prev=p;}printf("MAXREACH %.2f MINVX %.2f MAXSTEP %.2f\n",reach,minvx,maxstep);}
