// SPDX-License-Identifier: MIT
// Exercise real zoom preset rebuilding and closed-field input with synchronous
// image feedback, including tiny fit scales with identical rounded labels.
#include "lib/zoomcombobox/zoomcombobox.h"
#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QWheelEvent>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

static void check(bool ok, const char *message)
{
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Gwenview::ZoomComboBox combo;
    QAction fit("Fit"), fill("Fill"), actual("100%");
    QActionGroup modes(&combo);
    modes.setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    for (auto action : {&fit, &fill, &actual}) { action->setCheckable(true); modes.addAction(action); }
    combo.setActions(&fit, &fill, &actual);
    QObject::connect(&fit, &QAction::triggered, &combo, [&] { combo.setValue(combo.minimum()); });
    QObject::connect(&fill, &QAction::triggered, &combo, [&] { combo.setValue(1.37); });
    QObject::connect(&actual, &QAction::triggered, &combo, [&] { combo.setValue(1); });
    int changes = 0;
    QObject::connect(&combo, &Gwenview::ZoomComboBox::zoomChanged, &combo, [&](qreal scale) {
        ++changes;
        for (auto action : modes.actions()) action->setChecked(false);
        combo.setValue(scale);
    });
    auto wheel = [&](int delta) {
        QWheelEvent event(QPointF(10, 10), QPointF(10, 10), QPoint(), QPoint(0, delta),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&combo, &event);
    };
    auto near = [](qreal a, qreal b) { return qAbs(a - b) < 1e-7; };
    combo.setMaximum(16);
    for (qreal minimum : {0.001, 0.0031, 0.18, 0.49, 1.0, 0.0027}) {
        const int before = changes;
        combo.setMinimum(minimum);
        check(changes == before, "rebuilding presets must not emit a zoom request");
        QList<qreal> presets{minimum};
        qreal previous = 17;
        for (int i = 0; i < combo.count(); ++i) {
            auto action = combo.itemData(i).value<QAction *>();
            if (action && action != &actual) continue;
            const qreal scale = action ? 1 : combo.itemData(i).toReal();
            check(scale < previous, "presets must stay sorted after changing the fit scale");
            previous = scale;
            if (scale > minimum) presets.append(scale);
        }
        std::sort(presets.begin(), presets.end());
        fit.trigger();
        for (int i = 1; i < presets.size(); ++i) { wheel(120); check(near(combo.value(), presets[i]), "ascending wheel must visit every scale"); }
        wheel(120); check(near(combo.value(), 16), "maximum is stable");
        for (int i = presets.size() - 2; i >= 0; --i) { wheel(-120); check(near(combo.value(), presets[i]), "descending wheel must visit every scale"); }
        wheel(-120); check(near(combo.value(), minimum), "minimum is stable");
        fill.trigger(); wheel(120); check(near(combo.value(), 2), "wheel from fill uses its actual percentage");
        wheel(-120); check(near(combo.value(), 1), "direction reversal reaches actual size");
    }
    combo.setMaximum(8);
    check(near(combo.itemData(0).toReal(), 8), "maximum regeneration removes stale high presets");
    fprintf(stderr, "PASS: preset rebuilding, duplicate rounded labels, full-range wheel steps, Fit/Fill and changed maximum\n");
}
