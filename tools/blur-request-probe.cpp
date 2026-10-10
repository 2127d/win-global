// SPDX-License-Identifier: MIT
// Test-only interposition: observe real style requests without a compositor.
#include <QWindow>
#include <QVariant>
#include <QRegion>

namespace KWindowEffects {
void enableBlurBehind(QWindow *window, bool enabled, const QRegion &region)
{
    window->setProperty("testBlurEnabled", enabled);
    window->setProperty("testBlurRegion", QVariant::fromValue(region));
    window->setProperty("testBlurRequests", window->property("testBlurRequests").toInt() + 1);
}
}
