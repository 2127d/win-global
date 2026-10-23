// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick

Canvas {
    id: glyph
    property string condition: ""
    implicitWidth: 170; implicitHeight: 150
    onConditionChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c=getContext("2d"); c.reset(); c.scale(width/170,height/150);
        const night=condition.indexOf("night")>=0;
        const wet=/rain|showers|storm/.test(condition), snow=/snow|hail/.test(condition);
        const cloudy=/cloud|overcast|fog|mist/.test(condition)||wet||snow;
        if(!wet && !snow) {
            const g=c.createRadialGradient(107,49,5,107,49,40);
            g.addColorStop(0,night ? "#f2f2de" : "#fff1b5");g.addColorStop(1,night ? "#bdd4e6" : "#ffbd64");
            c.fillStyle=g; c.beginPath();c.arc(107,49,32,0,Math.PI*2);c.fill();
            if(!night) { c.strokeStyle="#ffdf91";c.lineWidth=2;c.lineCap="round";
                for(let a=0;a<8;a++) {c.save();c.translate(107,49);c.rotate(a*Math.PI/4);c.beginPath();c.moveTo(0,-40);c.lineTo(0,-45);c.stroke();c.restore();}}
        }
        if(cloudy) {
            const g=c.createLinearGradient(0,43,0,108);g.addColorStop(0,"#f7fbff");g.addColorStop(1,"#aac9df");
            c.fillStyle=g;c.beginPath();c.moveTo(38,108);c.bezierCurveTo(8,108,8,67,38,66);
            c.bezierCurveTo(40,24,98,23,107,60);c.bezierCurveTo(146,48,165,108,126,108);c.closePath();c.fill();
        }
        if(wet||snow) {c.strokeStyle="#8ddcff";c.lineWidth=snow?3:4;c.lineCap="round";
            for(let i=0;i<3;i++){const x=53+i*29;c.beginPath();c.moveTo(x,120);c.lineTo(x-5,132);c.stroke();
                if(snow){c.beginPath();c.moveTo(x-6,123);c.lineTo(x+3,129);c.stroke();}}}
        if(/fog|mist/.test(condition)){c.strokeStyle="#c0d9e8";c.lineWidth=3;c.lineCap="round";
            for(let i=0;i<2;i++){c.beginPath();c.moveTo(40+i*10,121+i*11);c.lineTo(129-i*10,121+i*11);c.stroke();}}
    }
}
