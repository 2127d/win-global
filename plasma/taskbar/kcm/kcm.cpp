// SPDX-License-Identifier: MIT
#include <KQuickConfigModule>
#include <qqml.h>
#include "settings.h"
class TaskbarKCM : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(TaskbarSettings *settings READ settings CONSTANT)
public:
    TaskbarKCM(QObject *parent, const KPluginMetaData &data) : KQuickConfigModule(parent, data), m_settings(this)
    {
        qmlRegisterAnonymousType<TaskbarSettings>("org.sequoia.taskbar.kcm", 1);
        setButtons(Apply | Default | Help);
        connect(&m_settings, &TaskbarSettings::dirtyChanged, this, [this] {
            setNeedsSave(m_settings.dirty());
            setRepresentsDefaults(m_settings.values() == TaskbarSettings::defaultValues());
        });
        connect(&m_settings, &TaskbarSettings::saveError, this, &TaskbarKCM::errorOccurred);
    }
    TaskbarSettings *settings() { return &m_settings; }
    void load() override { m_settings.load(); }
    void save() override { m_settings.save(); }
    void defaults() override { m_settings.defaults(); }
Q_SIGNALS:
    void errorOccurred(const QString &message);
private:
    TaskbarSettings m_settings;
};
K_PLUGIN_CLASS_WITH_JSON(TaskbarKCM, "kcm_sequoia_taskbar.json")
#include "kcm.moc"
