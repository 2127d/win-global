// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
#include "motion.h"
#include <QPainterPath>
#include <cmath>
#include <algorithm>

namespace {
constexpr double pi = 3.141592653589793;
void stroke(QPainter &p, const QPainterPath &path, QColor color, double width) {
    p.setBrush(Qt::NoBrush); p.setPen(QPen(color,width,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin)); p.drawPath(path);
}
void oval(QPainter &p, const QRectF &rect, QColor color) { p.setPen(Qt::NoPen); p.setBrush(color); p.drawEllipse(rect); }
void leaf(QPainter &p, QPointF at, double angle, double length, QColor color) {
    p.save(); p.translate(at); p.rotate(angle);
    QPainterPath shape; shape.moveTo(0,0); shape.cubicTo(length*.22,-length*.65,length*.94,-length*.42,length,0);
    shape.cubicTo(length*.65,length*.30,length*.20,length*.24,0,0);
    p.setPen(Qt::NoPen); p.setBrush(color); p.drawPath(shape);
    QPainterPath vein; vein.moveTo(3,0); vein.quadTo(length*.45,-length*.07,length*.84,-1);
    stroke(p,vein,QColor(255,255,203,48),.9); p.restore();
}
void vine(QPainter &p, double x, QPointF end, double sway, QColor color, double width, int count) {
    const QPointF start(x,-12), delta=end-start;
    QPainterPath stem; stem.moveTo(start);
    stem.cubicTo(start+delta/3+QPointF(sway,0),start+delta*2/3-QPointF(sway,0),end);
    stroke(p,stem,QColor(13,41,22,38),width+2);
    stroke(p,stem,color,width);
    for(int i=0;i<count;++i) {
        const double u=.12+.74*i/std::max(1,count-1);
        const QPointF point=stem.pointAtPercent(u);
        const double tangent=-stem.angleAtPercent(u);
        const double length=14+5*std::sin(i*2.1+x);
        leaf(p,point,tangent+(i%2? -65:65),length,color.lighter(i%3==0?118:105));
    }
}
}

void paintJungle(QPainter &p, const QSizeF &size, double seconds, double speed, double pause) {
    p.save(); p.setRenderHint(QPainter::Antialiasing);
    const double scale=size.height()/480.0;
    if(scale<=0){p.restore();return;}
    p.scale(scale,scale);
    const double width=size.width()/scale;
    const auto pose=junglePose(width,seconds,speed,pause);
    const auto &anchors=pose.anchors;
    const double limbGait=pose.gait-.08;
    const double phase=limbGait*pi;
    const double lag=std::sin(phase-.45);
    const QColor fur("#ad4d18"), furLight("#bd5b21"), skin("#e3a276"), face("#edb68a");
    const auto deployment=[&](double x){return pose.deployment(x);};
    // All vines can be caught; each root stays pinned to the panel.
    for(int i=0;i<anchors.size();++i) {
        const double x=anchors[i];
        const QPointF grip=pose.grips[i];
        const auto offset=grip-QPointF(x,0);
        const QPointF direction=offset/std::hypot(offset.x(),offset.y());
        bool loaded=false;
        loaded=pose.visible && (pose.heldVines[0]==i || pose.heldVines[1]==i);
        const QColor color=loaded?QColor("#c4df50"):QColor("#95b951");
        if(deployment(x)<=0)continue;
        p.save();p.translate(0,-(1-deployment(x))*210);
        vine(p,x,grip,0,color,4.5,5);
        QPainterPath tip;tip.moveTo(grip);
        tip.quadTo(grip+direction*25+QPointF(9,0),grip+direction*(55+12*std::sin(i))+QPointF(3,0));
        stroke(p,tip,color,4.5);leaf(p,grip+direction*30+QPointF(6,0),i%2?150:-25,18,color);
        p.restore();
    }
    // Small leaves at the panel edge, with no painted background strip.
    for(int i=0;i<width/105+1;++i) {
        const double x=22+i*105;
        if(deployment(x)<=0)continue;
        p.save();p.translate(0,-(1-deployment(x))*40);
        leaf(p,QPointF(x,-3),28+10*std::sin(seconds*.4+i),29,QColor("#4c7c42"));
        leaf(p,QPointF(x+17,-5),142,25,QColor("#79a34a"));p.restore();
    }
    if(!pose.visible){p.restore();return;}
    const QPointF body=pose.body;
    p.save();p.translate(body);p.rotate(pose.rotation);
    const QTransform local=p.worldTransform();
    const QTransform artToDevice=QTransform::fromScale(scale,scale);
    const QPointF lh=local.inverted().map(artToDevice.map(pose.hands[0]));
    const QPointF rh=local.inverted().map(artToDevice.map(pose.hands[1]));
    const auto armPath=[&](QPointF shoulder,QPointF hand,int handIndex) {
        QPainterPath path;path.moveTo(shoulder);
        const auto delta=hand-shoulder;
        const double distance=std::max(1.0,std::hypot(delta.x(),delta.y()));
        const QPointF normal(-delta.y()/distance,delta.x()/distance);
        // Elbow curvature follows the circulating arm continuously, flattening
        // at release/catch without snapping when the support hand changes.
        const double bend=std::sqrt(std::max(0.0,95*95-distance*distance*.25))*(.78+.22*pose.armFlex[handIndex]);
        path.quadTo((shoulder+hand)*.5+normal*((5+1.3*bend)*(normal.x()*std::sin(pose.rotation*pi/180)+normal.y()*std::cos(pose.rotation*pi/180))),hand);
        return path;
    };
    QPainterPath tail; tail.moveTo(-51,13); tail.cubicTo(-102,43,-156,18,-139,-14);
    tail.cubicTo(-123,-43,-95,-20,-114,-5);
    stroke(p,tail,QColor("#8d3e17"),10);
    // Legs pass ahead of the hips on the first ascent, then trail on return.
    const double lift=.5-.5*std::cos(pi*limbGait);
    const QPointF backToes(-115+35*lift,-18+105*lift-8*lag);
    // Far leg and arm.
    QPainterPath backLeg; backLeg.moveTo(-38,10); backLeg.cubicTo(-77,18,backToes.x()+23,backToes.y()+13,backToes.x(),backToes.y());
    stroke(p,backLeg,fur,17);
    QPainterPath backFoot; backFoot.moveTo(backToes); backFoot.lineTo(backToes+QPointF(-12,-7)); backFoot.lineTo(backToes+QPointF(-18,5));
    stroke(p,backFoot,skin,13);
    stroke(p,armPath(QPointF(2,-14),lh,0),fur,17);
    oval(p,QRectF(lh.x()-12,lh.y()-17,24,34),skin);
    // Body and a subtle warm belly, keeping the reference's long silhouette.
    oval(p,QRectF(-66,-28,128,57),fur);
    oval(p,QRectF(-34,-9,77,31),furLight);
    const double kick=std::pow(std::sin(pi*(limbGait-.15)),2);
    const QPointF toes(-116+50*lift+15*kick,47+85*lift+10*std::sin(pi*limbGait+.5));
    QPainterPath leg; leg.moveTo(-40,19); leg.cubicTo(-68,51,toes.x()+27,toes.y()+10,toes.x(),toes.y());
    stroke(p,leg,furLight,18);
    QPainterPath foot; foot.moveTo(toes); foot.lineTo(toes+QPointF(-13,-3)); foot.lineTo(toes+QPointF(-15,11));
    stroke(p,foot,skin,14);
    stroke(p,armPath(QPointF(19,-8),rh,1),furLight,19);
    oval(p,QRectF(rh.x()-13,rh.y()-18,26,36),skin);
    // Thumb wraps in front of each bright vine.
    QPainterPath thumb; thumb.moveTo(rh.x()+4,rh.y()-6); thumb.quadTo(rh.x()+12,rh.y()+3,rh.x()+2,rh.y()+12);
    stroke(p,thumb,face,7);
    // Keep his gaze ahead while the torso swings past vertical.
    p.save();p.translate(51,-16);p.rotate(-pose.rotation*.80);p.translate(-51,16);
    oval(p,QRectF(24,-51,67,66),furLight);
    oval(p,QRectF(22,-34,24,27),skin);
    oval(p,QRectF(28,-28,12,15),QColor("#c77e53"));
    oval(p,QRectF(47,-39,29,42),face);
    oval(p,QRectF(58,-22,47,42),face);
    // Eyes blink briefly, without changing the steady swing cadence.
    const double blink=std::fmod(seconds,4.9);
    if (blink>4.70 && blink<4.84) {
        QPainterPath eye; eye.moveTo(63,-26); eye.quadTo(67,-23,70,-27); stroke(p,eye,QColor("#513024"),2.7);
    } else {
        oval(p,QRectF(63,-30,6.5,11),QColor("#513024"));
        oval(p,QRectF(65,-29,2,3),QColor("#fff4d8"));
    }
    oval(p,QRectF(92,-9,3,4),QColor("#8f4a2e"));
    QPainterPath smile; smile.moveTo(75,-5); smile.cubicTo(77,10,88,15,96,6);
    stroke(p,smile,QColor("#9f5637"),2.5);
    // Small forehead tuft.
    QPainterPath tuft; tuft.moveTo(35,-42); tuft.quadTo(54,-64,67,-48); tuft.quadTo(57,-54,53,-42);
    p.setPen(Qt::NoPen);p.setBrush(furLight);p.drawPath(tuft);
    p.restore();
    p.restore();
    p.restore();
}
