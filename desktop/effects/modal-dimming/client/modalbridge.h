// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QTimer>
class QWindow;
// The host exposes sequoiaModalAmount (0..0.6), including its overlay animation.
// The bridge sends at most one update per frame and is inert without the effect.
class SequoiaModalBridge final : public QObject
{
    Q_OBJECT
public:
    explicit SequoiaModalBridge(QWindow *window);
public Q_SLOTS:
    void schedule();
private:
    bool eventFilter(QObject *, QEvent *) override;
    void publish();
    void send(uint surface, double amount);
    QPointer<QWindow> m_window;
    QTimer m_timer;
    uint m_surface = 0;
    bool m_available = false;
    bool m_inFlight = false;
    bool m_pending = false;
    int m_retries = 0;
};
