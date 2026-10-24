// SPDX-License-Identifier: GPL-2.0-or-later
#include <effect/effect.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <core/renderviewport.h>
#include <wayland/surface.h>
#include <wayland/clientconnection.h>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QVariantAnimation>
#include <KDecoration3/Decoration>
#include <cmath>

using namespace KWin;

class ModalDimming final : public Effect, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sequoia.ModalDimming1")
public:
    ModalDimming()
        : m_watch(new QDBusServiceWatcher(this))
    {
        auto bus = QDBusConnection::sessionBus();
        m_watch->setConnection(bus);
        m_watch->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(m_watch, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &owner) {
            for (auto it = m_embedded.begin(); it != m_embedded.end();) {
                if (it->owner == owner) {
                    decorationScrim(it.key(), 0);
                    it.key()->addRepaintFull();
                    it = m_embedded.erase(it);
                } else ++it;
            }
            m_watch->removeWatchedService(owner);
        });
        connect(effects, &EffectsHandler::windowClosed, this, [this](EffectWindow *w) {
            decorationScrim(w, 0);
            m_embedded.remove(w);
            if (auto animation = m_native.take(w)) delete animation;
            updateNative();
        });
        connect(effects, &EffectsHandler::windowAdded, this, [this](EffectWindow *w) {
            watch(w);
            updateNative();
        });
        connect(effects, &EffectsHandler::stackingOrderChanged, this, &ModalDimming::updateNative);
        for (auto w : effects->stackingOrder()) watch(w);
        // Register the object first; clients resubmit when the service appears.
        m_registered = bus.registerObject(QStringLiteral("/org/sequoia/ModalDimming"), this, QDBusConnection::ExportAllSlots)
            && bus.registerService(QStringLiteral("org.sequoia.ModalDimming"));
        updateNative();
    }
    ~ModalDimming() override
    {
        for (auto w : m_embedded.keys()) decorationScrim(w, 0);
        if (m_registered) {
            auto bus = QDBusConnection::sessionBus();
            bus.unregisterService(QStringLiteral("org.sequoia.ModalDimming"));
            bus.unregisterObject(QStringLiteral("/org/sequoia/ModalDimming"));
        }
        effects->addRepaintFull();
    }
    bool isActive() const override { return !m_embedded.isEmpty() || !m_native.isEmpty(); }
    int requestedEffectChainPosition() const override { return 99; }

    void drawWindow(const RenderTarget &target, const RenderViewport &viewport, EffectWindow *w,
                    int mask, const Region &region, WindowPaintData &data) override
    {
        if (const auto animation = m_native.value(w); animation && !nativeHandled()) {
            WindowPaintData dimmed(data);
            dimmed.multiplyBrightness(std::pow(1.0 - animation->currentValue().toReal(), 2.2));
            effects->drawWindow(target, viewport, w, mask, region, dimmed);
            return;
        }
        const qreal amount = m_embedded.value(w).amount;
        // Cooperating decorations compose the same source-over black scrim as
        // the app. Brightness alone leaves the backdrop visible through alpha
        // undimmed, so translucent title bars otherwise look too light.
        if (decorationScrim(w, amount)) {
            effects->drawWindow(target, viewport, w, mask, region, data);
            return;
        }
        // Rotation and overview own their composition. Axis-aligned transforms
        // use the same device-space partition as the normal window.
        if (amount <= 0 || effects->hasActiveFullScreenEffect() || data.rotationAngle() != 0) {
            effects->drawWindow(target, viewport, w, mask, region, data);
            return;
        }
        const auto map = [&](const RectF &rect) {
            // Match the renderer's nearest-pixel rasterization. Rounding the
            // client outwards exempts a decoration row when its edge falls
            // between pixels, leaving a bright seam above the dimmed client.
            return viewport.mapToDeviceCoordinates(rect.translated(-w->pos())
                .scaled(data.xScale(), data.yScale()).translated(w->pos() + data.translation().toPointF())).rounded();
        };
        const Region frame(map(w->frameGeometry()));
        const Region client(map(w->clientGeometry()));
        const Region decoration = frame.subtracted(client);
        effects->drawWindow(target, viewport, w, mask, region.subtracted(decoration), data);
        WindowPaintData dimmed(data);
        dimmed.multiplyBrightness(std::pow(1.0 - amount, 2.2));
        effects->drawWindow(target, viewport, w, mask, region.intersected(decoration), dimmed);
    }

public Q_SLOTS:
    bool SetDecorationDim(uint surfaceId, double amount)
    {
        if (!calledFromDBus() || !std::isfinite(amount) || amount < 0 || amount > .6 || !surfaceId) return false;
        const QString owner = message().service();
        const auto reply = connection().interface()->servicePid(owner);
        if (!reply.isValid()) return false;
        EffectWindow *matched = nullptr;
        for (auto w : effects->stackingOrder()) {
            const auto surface = w->surface();
            if (!w->isDeleted() && surface && surface->id() == surfaceId
                && surface->client()->processId() == pid_t(reply.value())) {
                // Multiple Wayland connections in one process can reuse IDs.
                // Refuse ambiguity instead of choosing an unrelated window.
                if (matched) return false;
                matched = w;
            }
        }
        if (!matched) return false;
        if (amount == 0) m_embedded.remove(matched);
        else {
            m_embedded[matched] = {owner, amount};
            m_watch->addWatchedService(owner);
        }
        decorationScrim(matched, amount);
        matched->addRepaintFull();
        return true;
    }
private:
    static bool decorationScrim(EffectWindow *w, qreal amount)
    {
        auto decoration = w->decoration();
        if (!decoration || decoration->metaObject()->indexOfProperty("sequoiaModalAmount") < 0) return false;
        decoration->setProperty("sequoiaModalAmount", amount);
        return true;
    }
    struct Embedded { QString owner; qreal amount = 0; };
    QHash<EffectWindow *, Embedded> m_embedded;
    QHash<EffectWindow *, QVariantAnimation *> m_native;
    QDBusServiceWatcher *m_watch;
    bool m_registered = false;
    bool nativeHandled() const
    {
        return effects->isEffectLoaded(QStringLiteral("dialogparent"))
            || effects->isEffectLoaded(QStringLiteral("dimparent"));
    }
    void watch(EffectWindow *w)
    {
        connect(w, &EffectWindow::windowModalityChanged, this, &ModalDimming::updateNative);
        connect(w, &EffectWindow::minimizedChanged, this, &ModalDimming::updateNative);
    }
    void updateNative()
    {
        // Respect KDE's existing dim-parent effect instead of applying it twice.
        const bool handled = nativeHandled();
        for (auto w : effects->stackingOrder()) {
            if (w->isDeleted()) continue;
            auto modal = w->findModal();
            const qreal goal = !handled && modal && !modal->isDeleted() && !modal->isMinimized() ? .3 : 0;
            auto animation = m_native.value(w);
            if (!animation && goal == 0) continue;
            if (!animation) {
                animation = new QVariantAnimation(this);
                animation->setStartValue(0.0);
                animation->setEndValue(0.0);
                m_native.insert(w, animation);
                connect(animation, &QVariantAnimation::valueChanged, this, [w] { w->addRepaintFull(); });
                connect(animation, &QVariantAnimation::finished, this, [this, w, animation] {
                    if (animation->endValue().toReal() == 0) {
                        m_native.remove(w);
                        animation->deleteLater();
                    }
                });
            }
            if (animation->endValue().toReal() == goal) continue;
            const qreal current = animation->currentValue().toReal();
            animation->stop();
            animation->setStartValue(current);
            animation->setEndValue(goal);
            animation->setDuration(qRound(160 * effects->animationTimeFactor()));
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            animation->start();
        }
    }
};
KWIN_EFFECT_FACTORY(ModalDimming, "metadata.json")
#include "modaldimming.moc"
