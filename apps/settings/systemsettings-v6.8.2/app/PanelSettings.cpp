// SPDX-License-Identifier: GPL-2.0-or-later
#include "PanelSettings.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <KLocalizedString>
#include <cmath>

PanelSettings::PanelSettings(QObject *parent) : QObject(parent)
{
    setObjectName(QStringLiteral("panelSettings"));
    QFile script(QStringLiteral(":/settings-hub/PanelScript.js"));
    if (script.open(QIODevice::ReadOnly)) m_script = QString::fromUtf8(script.readAll());
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &PanelSettings::refresh);
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PanelSettings::changed);
}
QVariantMap PanelSettings::selected() const
{
    for (const auto &panel : m_panels) if (panel.toMap().value(QStringLiteral("id")).toInt() == m_selectedId) return panel.toMap();
    return {};
}
void PanelSettings::setSelectedId(int id)
{
    for (const auto &panel : m_panels) if (panel.toMap().value(QStringLiteral("id")).toInt() == id && m_selectedId != id) {
        m_selectedId = id; Q_EMIT changed(); return;
    }
}
void PanelSettings::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    if (active) { m_poll.start(); refresh(); } else m_poll.stop();
    Q_EMIT activeChanged();
}
void PanelSettings::refresh() { if (!m_busy) dispatch(); }
void PanelSettings::setSetting(const QString &key, const QVariant &value)
{
    if (selected().isEmpty() || selected().value(QStringLiteral("locked")).toBool()) return;
    const QMap<QString, QStringList> choices{
        {QStringLiteral("location"), {QStringLiteral("top"), QStringLiteral("bottom"), QStringLiteral("left"), QStringLiteral("right")}},
        {QStringLiteral("alignment"), {QStringLiteral("left"), QStringLiteral("center"), QStringLiteral("right")}},
        {QStringLiteral("lengthMode"), {QStringLiteral("fill"), QStringLiteral("fit"), QStringLiteral("custom")}},
        {QStringLiteral("hiding"), {QStringLiteral("none"), QStringLiteral("autohide"), QStringLiteral("dodgewindows"), QStringLiteral("windowsgobelow")}},
        {QStringLiteral("opacity"), {QStringLiteral("adaptive"), QStringLiteral("opaque"), QStringLiteral("translucent")}},
        {QStringLiteral("floatingMode"), {QStringLiteral("disabled"), QStringLiteral("applets"), QStringLiteral("panel")}}
    };
    const QStringList numeric{QStringLiteral("height"), QStringLiteral("screen"), QStringLiteral("minimumLength"), QStringLiteral("maximumLength"), QStringLiteral("offset")};
    if (choices.contains(key)) {
        if (!choices.value(key).contains(value.toString())) return;
        if (key == QStringLiteral("opacity") && !opacitySupported()) return;
    } else if (numeric.contains(key)) {
        bool valid = false;
        const double number = value.toDouble(&valid);
        if (!valid || !std::isfinite(number) || number != std::floor(number) || std::abs(number) > 100000) return;
    } else return;
    enqueue({{QStringLiteral("id"), m_selectedId}, {QStringLiteral("key"), key}, {QStringLiteral("value"), value}, {QStringLiteral("action"), QStringLiteral("set")}});
}
void PanelSettings::editPanel()
{
    if (!selected().isEmpty() && !selected().value(QStringLiteral("locked")).toBool())
        enqueue({{QStringLiteral("id"), m_selectedId}, {QStringLiteral("action"), QStringLiteral("edit")}});
}
void PanelSettings::enqueue(const QVariantMap &command)
{
    // Coalesce only consecutive edits of the same control on the same panel.
    // Keep dependencies (screen/location/length mode) in their original order.
    if (!m_pending.isEmpty() && command.value(QStringLiteral("action")) == QStringLiteral("set")
        && m_pending.last().value(QStringLiteral("id")) == command.value(QStringLiteral("id"))
        && m_pending.last().value(QStringLiteral("key")) == command.value(QStringLiteral("key"))) m_pending.last() = command;
    else m_pending.enqueue(command);
    if (!m_busy) dispatch(m_pending.dequeue());
}
void PanelSettings::dispatch(const QVariantMap &command)
{
    if (m_script.isEmpty()) { m_error = i18n("Panel controls could not be loaded."); Q_EMIT changed(); return; }
    m_busy = true; Q_EMIT busyChanged();
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"), QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("evaluateScript"));
    message << QString(m_script + QStringLiteral("\nsequoiaPanelSettings(%1);").arg(QString::fromUtf8(QJsonDocument::fromVariant(command).toJson(QJsonDocument::Compact))));
    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, 5000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        const auto document = QJsonDocument::fromJson(reply.isError() ? QByteArray() : reply.value().trimmed().toUtf8());
        if (reply.isError() || !document.isObject()) {
            m_error = i18n("Panel controls are unavailable. Check that Plasma is running, then refresh.");
            m_panels.clear(); m_screens.clear(); m_selectedId = -1; m_pending.clear();
        } else {
            const auto state = document.object();
            m_error = state.value(QStringLiteral("error")).toString();
            m_panels = state.value(QStringLiteral("panels")).toArray().toVariantList();
            m_screens = state.value(QStringLiteral("screens")).toArray().toVariantList();
            if (selected().isEmpty()) m_selectedId = m_panels.isEmpty() ? -1 : m_panels.first().toMap().value(QStringLiteral("id")).toInt();
        }
        m_busy = false; Q_EMIT busyChanged();
        if (m_pending.isEmpty()) Q_EMIT changed();
        else dispatch(m_pending.dequeue());
    });
}
