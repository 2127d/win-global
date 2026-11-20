// SPDX-License-Identifier: GPL-2.0-or-later
#include "jungle.h"
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
    QPainterPath stem; stem.moveTo(x,-12); stem.cubicTo(x+sway,75,end.x()-sway,end.y()-65,end.x(),end.y());
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

void paintJungle(QPainter &p, const QSizeF &size, double seconds) {
    p.save(); p.setRenderHint(QPainter::Antialiasing);
    const double scale=size.height()/396.0;
    if(scale<=0){p.restore();return;}
    p.scale(scale,scale);
    const double width=size.width()/scale;
    constexpr double spacing=172, speed=90;
    const double journey=(width+460)/speed;
    const double travel=std::fmod(std::max(0.0,seconds-2.0),journey+4.0);
    const bool monkeyVisible=seconds>=2.0 && travel<journey;
    const double bodyX=-230+travel*speed;
    const double phase=bodyX*2*pi/spacing;
    const double lag=std::sin(phase-.45);
    const QColor fur("#ad4d18"), furLight("#bd5b21"), skin("#e3a276"), face("#edb68a");
    const auto deployment=[&](double x) {
        const double u=std::clamp((seconds-std::max(0.0,x/width)*.65)/1.15,0.0,1.0);
        return 1-std::pow(1-u,3);
    };
    struct Hand {QPointF position; int vine; bool holding;};
    const auto hand=[&](int side) {
        const double cycles=(bodyX-side*spacing+spacing*.5)/(2*spacing);
        const int turn=std::floor(cycles);
        const double fraction=cycles-turn;
        const int index=turn*2+side;
        const double anchor=index*spacing;
        if(fraction<.5) {
            const double relative=(bodyX-anchor)/(spacing*.5);
            const double weight=1-relative*relative;
            return Hand{QPointF(anchor+(bodyX-anchor)*.35*weight,125+27*weight),index,true};
        }
        const double reach=(fraction-.5)*2;
        const double smooth=reach*reach*(3-2*reach);
        return Hand{QPointF(anchor+2*spacing*smooth,125-48*std::sin(pi*reach)),index,false};
    };
    const Hand leftHand=hand(0),rightHand=hand(1);
    for(int i=-1;i<=std::ceil(width/spacing)+1;++i) {
        const double x=i*spacing+spacing*.5;
        const double length=180+85*(.5+.5*std::sin(i*2.8));
        const double sway=9*std::sin(seconds*.7+i);
        p.save();p.scale(1,deployment(x));
        vine(p,x,QPointF(x+sway,length),12*std::sin(seconds*.5+i),QColor("#568c48"),2.8,7);
        p.restore();
    }
    // Every fixed anchor belongs to the canopy; hands alternately hold and
    // release these vines, so the scenery never scrolls with the monkey.
    for(int i=-1;i<=std::ceil(width/spacing)+1;++i) {
        const double x=i*spacing;
        QPointF grip(x,125);
        bool loaded=false;
        if(monkeyVisible && leftHand.holding && leftHand.vine==i){grip=leftHand.position;loaded=true;}
        if(monkeyVisible && rightHand.holding && rightHand.vine==i){grip=rightHand.position;loaded=true;}
        const QColor color=loaded?QColor("#c4df50"):QColor("#95b951");
        p.save();p.scale(1,deployment(x));
        vine(p,x,grip,10*std::sin(seconds*.55+i),color,4.5,5);
        QPainterPath tip;tip.moveTo(grip);
        tip.quadTo(grip+QPointF(9,25),grip+QPointF(3,55+12*std::sin(i)));
        stroke(p,tip,color,4.5);leaf(p,grip+QPointF(6,30),i%2?150:-25,18,color);
        p.restore();
    }
    // Small leaves at the panel edge, with no painted background strip.
    for(int i=0;i<width/105+1;++i) {
        const double x=22+i*105;
        p.save();p.scale(1,deployment(x));
        leaf(p,QPointF(x,-3),28+10*std::sin(seconds*.4+i),29,QColor("#4c7c42"));
        leaf(p,QPointF(x+17,-5),142,25,QColor("#79a34a"));p.restore();
    }
    if(!monkeyVisible){p.restore();return;}
    const QPointF body(bodyX,232+22*std::cos(phase));
    p.save();p.translate(body);p.rotate(-7+10*std::sin(phase));
    const QTransform local=p.worldTransform();
    const QTransform artToDevice=QTransform::fromScale(scale,scale);
    const QPointF lh=local.inverted().map(artToDevice.map(leftHand.position));
    const QPointF rh=local.inverted().map(artToDevice.map(rightHand.position));
    QPainterPath tail; tail.moveTo(-51,13); tail.cubicTo(-102,43,-156,18,-139,-14);
    tail.cubicTo(-123,-43,-95,-20,-114,-5);
    stroke(p,tail,QColor("#8d3e17"),10);
    // Far leg and arm.
    QPainterPath backLeg; backLeg.moveTo(-38,10); backLeg.cubicTo(-77,18,-92,-5,-115,-18-8*lag);
    stroke(p,backLeg,fur,17);
    QPainterPath backFoot; backFoot.moveTo(-114,-18-8*lag); backFoot.lineTo(-126,-25-8*lag); backFoot.lineTo(-132,-13-8*lag);
    stroke(p,backFoot,skin,13);
    QPainterPath farArm; farArm.moveTo(2,-14); farArm.cubicTo(-29,-33,lh.x()+14,lh.y()+43,lh.x(),lh.y());
    stroke(p,farArm,fur,17);
    oval(p,QRectF(lh.x()-12,lh.y()-17,24,34),skin);
    // Body and a subtle warm belly, keeping the reference's long silhouette.
    oval(p,QRectF(-66,-28,128,57),fur);
    oval(p,QRectF(-34,-9,77,31),furLight);
    QPainterPath leg; leg.moveTo(-40,19); leg.cubicTo(-68,51,-89,57,-116,47+6*lag);
    stroke(p,leg,furLight,18);
    QPainterPath foot; foot.moveTo(-116,47+6*lag); foot.lineTo(-129,44+6*lag); foot.lineTo(-131,58+6*lag);
    stroke(p,foot,skin,14);
    QPainterPath arm; arm.moveTo(19,-8); arm.cubicTo(65,-25,rh.x()-6,rh.y()+48,rh.x(),rh.y());
    stroke(p,arm,furLight,19);
    oval(p,QRectF(rh.x()-13,rh.y()-18,26,36),skin);
    // Thumb wraps in front of each bright vine.
    QPainterPath thumb; thumb.moveTo(rh.x()+4,rh.y()-6); thumb.quadTo(rh.x()+12,rh.y()+3,rh.x()+2,rh.y()+12);
    stroke(p,thumb,face,7);
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
}
