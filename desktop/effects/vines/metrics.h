// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QMutex>
#include <QQuickItem>
#include <QVector>
#include <atomic>
#include <deque>

// Opt-in, bounded in-memory timing sample; no desktop pixels are captured.
class VinesFrameMetrics9 : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.SequoiaVines.Diagnostics")
public:
    explicit VinesFrameMetrics9(QQuickItem *item);
    ~VinesFrameMetrics9() override;
    void painted(qint64 duration);
    void swapped();
public slots:
    void Start();
    QString Stop();
private:
    QQuickItem *m_item;
    std::atomic<bool> m_active{false};
    std::atomic<quint64> m_serial{0},m_swapped{0};
    QMutex m_mutex;QElapsedTimer m_clock;
    QVector<double> m_paints,m_costs,m_submits,m_presentations;
    std::deque<quint64> m_pending;
    quint64 m_lastPresented=0;
    QMetaObject::Connection m_requested,m_presented;
};
