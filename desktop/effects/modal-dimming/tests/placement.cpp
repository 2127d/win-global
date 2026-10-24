// SPDX-License-Identifier: GPL-2.0-or-later
// Loaded only into the private test compositor, never installed with the effect.
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <window.h>
#include <QCoreApplication>
#include <QTimer>
#include <QSet>
#include <memory>
static void start()
{
    auto timer = new QTimer(qApp);
    auto placed = std::make_shared<QSet<QUuid>>();
    QObject::connect(timer, &QTimer::timeout, [placed] {
        if (!KWin::effects) return;
        for (auto effectWindow : KWin::effects->stackingOrder()) {
            auto window = effectWindow->window();
            if (!window->caption().contains(QStringLiteral("Modal dimming test")) || placed->contains(window->internalId())) continue;
            placed->insert(window->internalId());
            QTimer::singleShot(300, window, [window] {
                const auto geometry = window->frameGeometry();
                window->moveResize(geometry.translated(QPointF(111.25, 111.75) - geometry.topLeft()));
                qInfo() << "Seam fixture positioned:" << window->frameGeometry();
            });
        }
    });
    timer->start(50);
}
Q_COREAPP_STARTUP_FUNCTION(start)
