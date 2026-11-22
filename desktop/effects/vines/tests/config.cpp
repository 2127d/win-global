// SPDX-License-Identifier: GPL-2.0-or-later
#include <QApplication>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KCModule>
#include <cstdio>
#include <cstdlib>
static void require(bool value,const char *message) {
    if(!value){fprintf(stderr,"FAIL: %s\n",message);std::exit(2);}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    require(qEnvironmentVariable("XDG_CONFIG_HOME").startsWith("/tmp/gwenview-test-"),"settings probe restricted to private configuration");
    const auto factory=KPluginFactory::loadFactory(KPluginMetaData(QStringLiteral("kwin/effects/configs/kcm_kwin4_genericscripted")));
    require(bool(factory),"load KWin settings factory");
    const auto id=qEnvironmentVariable("VINES_SCRIPT_ID","sequoia-vines-v9");
    auto module=factory.plugin->create<KCModule>(nullptr,{id,QStringLiteral("KWin/Script")});
    require(module,"create real script settings module");
    module->load();
    auto speed=module->widget()->findChild<QSpinBox *>("kcfg_SwingSpeed");
    auto pause=module->widget()->findChild<QDoubleSpinBox *>("kcfg_PauseSeconds");
    require(speed && pause,"settings controls loaded");
    if(argc>1 && QString(argv[1])=="defaults") {
        module->defaults();module->save();
        require(speed->value()==100 && qAbs(pause->value()-2.6)<.001,"restore original timing defaults");
    } else {
        require(speed->value()==100 && qAbs(pause->value()-2.6)<.001,"load default timing values");
        speed->setValue(200);pause->setValue(60);module->save();
        module->load();
        require(speed->value()==200 && pause->value()==60,"settings save and reload");
        module->widget()->resize(480,180);module->widget()->show();app.processEvents();
        module->widget()->grab().save(qEnvironmentVariable("VINES_CAPTURE")+"/settings.png");
    }
    delete module;
    fprintf(stderr,"PASS: real KWin script settings dialog\n");
}
