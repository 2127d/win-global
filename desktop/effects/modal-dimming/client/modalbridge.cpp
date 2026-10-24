// SPDX-License-Identifier: GPL-2.0-or-later
#include "modalbridge.h"
#include <QWindow>
#include <QPlatformSurfaceEvent>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <qpa/qplatformwindow_p.h>
#include <wayland-client-core.h>
#include <cmath>

static const QString service = QStringLiteral("org.sequoia.ModalDimming");
SequoiaModalBridge::SequoiaModalBridge(QWindow *window) : QObject(window), m_window(window)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &SequoiaModalBridge::publish);
    auto bus = QDBusConnection::sessionBus();
    auto watcher = new QDBusServiceWatcher(service, bus, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &owner) {
        m_available = !owner.isEmpty();
        schedule();
    });
    m_available = bus.interface() && bus.interface()->isServiceRegistered(service).value();
    window->installEventFilter(this);
    connect(window, SIGNAL(sequoiaModalAmountChanged()), this, SLOT(schedule()));
    connect(window, &QWindow::visibleChanged, this, &SequoiaModalBridge::schedule);
    schedule();
}
void SequoiaModalBridge::schedule()
{
    m_retries = 0;
    if (!m_timer.isActive()) m_timer.start(16);
}
bool SequoiaModalBridge::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::PlatformSurface) {
        auto surface = static_cast<QPlatformSurfaceEvent *>(event);
        if (surface->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            if (m_available && m_surface) send(m_surface, 0);
            m_surface = 0;
        } else schedule();
    }
    return false;
}
void SequoiaModalBridge::publish()
{
    if (!m_available || !m_window) return;
    if (m_inFlight) { m_pending = true; return; }
    auto native = m_window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
    if (!native || !native->surface()) return;
    m_surface = wl_proxy_get_id(reinterpret_cast<wl_proxy *>(native->surface()));
    const double amount = m_window->isVisible() ? m_window->property("sequoiaModalAmount").toDouble() : 0;
    send(m_surface, std::isfinite(amount) ? qBound(0.0, amount, .6) : 0);
}
void SequoiaModalBridge::send(uint surface, double amount)
{
    auto message = QDBusMessage::createMethodCall(service, QStringLiteral("/org/sequoia/ModalDimming"),
                                                QStringLiteral("org.sequoia.ModalDimming1"), QStringLiteral("SetDecorationDim"));
    message << surface << amount;
    m_inFlight = true;
    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, 1000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, amount](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<bool> reply = *call;
        call->deleteLater();
        m_inFlight = false;
        // A newly created wl_surface may not be mapped when the first report
        // arrives. Retry briefly; never build an unbounded queue if KWin stalls.
        const bool retry = !reply.isError() && !reply.value() && amount > 0 && m_retries++ < 3;
        if (m_pending || retry) {
            m_pending = false;
            m_timer.start(retry ? 100 : 16);
        }
    });
}
