// SPDX-License-Identifier: GPL-2.0-or-later
#include "metrics.h"
#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QMutexLocker>
#include <effect/effecthandler.h>
#include <core/output.h>
#include <core/backendoutput.h>
#include <core/renderloop.h>
#include <algorithm>
namespace {
constexpr auto path="/SequoiaVines/Diagnostics";
QJsonObject stats(const QVector<double> &times) {
    if(times.size()<2)return {{"samples",times.size()}};
    QVector<double> intervals;
    for(int i=1;i<times.size();++i)intervals.append(times[i]-times[i-1]);
    std::sort(intervals.begin(),intervals.end());
    return {{"samples",times.size()},{"hz",1000.0*(times.size()-1)/(times.last()-times.first())},
        {"median_ms",intervals[intervals.size()/2]}, {"p95_ms",intervals[int((intervals.size()-1)*.95)]},
        {"max_ms",intervals.last()}};
}
}
VinesFrameMetrics7::VinesFrameMetrics7(QQuickItem *item):QObject(item),m_item(item) {
    QDBusConnection::sessionBus().registerObject(path,this,QDBusConnection::ExportAllSlots);
}
VinesFrameMetrics7::~VinesFrameMetrics7() {
    m_active=false;
    disconnect(m_requested);disconnect(m_presented);
    QDBusConnection::sessionBus().unregisterObject(path);
}
void VinesFrameMetrics7::Start() {
    m_active=false;disconnect(m_requested);disconnect(m_presented);
    {
        QMutexLocker lock(&m_mutex);
        m_paints.clear();m_costs.clear();m_submits.clear();m_presentations.clear();m_pending.clear();
        m_lastPresented=0;m_clock.start();
    }
    if(auto window=m_item->window()) {
        if(auto output=KWin::effects->screenAt(window->geometry().center())) {
            auto loop=output->backendOutput()->renderLoop();
            m_requested=connect(loop,&KWin::RenderLoop::frameRequested,this,[this] {
                if(!m_active)return;
                QMutexLocker lock(&m_mutex);
                m_pending.push_back(m_swapped.load());
                if(m_pending.size()>8)m_pending.pop_front();
            });
            m_presented=connect(loop,&KWin::RenderLoop::framePresented,this,[this](auto *,auto timestamp,auto) {
                if(!m_active)return;
                QMutexLocker lock(&m_mutex);
                if(m_pending.empty())return;
                const auto serial=m_pending.front();m_pending.pop_front();
                if(serial && serial!=m_lastPresented && m_presentations.size()<10000) {
                    m_presentations.append(timestamp.count()/1e6);m_lastPresented=serial;
                }
            });
        }
    }
    m_active=true;
}
QString VinesFrameMetrics7::Stop() {
    m_active=false;disconnect(m_requested);disconnect(m_presented);
    QMutexLocker lock(&m_mutex);
    std::sort(m_costs.begin(),m_costs.end());
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {"paint",stats(m_paints)},{"submitted",stats(m_submits)},{"presented",stats(m_presentations)},
        {"paint_cpu_median_ms",m_costs.isEmpty()?0:m_costs[m_costs.size()/2]},
        {"paint_cpu_p95_ms",m_costs.isEmpty()?0:m_costs[int((m_costs.size()-1)*.95)]}
    }).toJson(QJsonDocument::Compact));
}
void VinesFrameMetrics7::painted(qint64 duration) {
    ++m_serial;
    if(!m_active)return;
    QMutexLocker lock(&m_mutex);
    if(m_paints.size()<10000) {m_paints.append(m_clock.nsecsElapsed()/1e6);m_costs.append(duration/1e6);}
}
void VinesFrameMetrics7::swapped() {
    m_swapped=m_serial.load();
    if(!m_active)return;
    QMutexLocker lock(&m_mutex);
    if(m_submits.size()<10000)m_submits.append(m_clock.nsecsElapsed()/1e6);
}
