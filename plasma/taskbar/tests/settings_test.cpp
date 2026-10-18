// SPDX-License-Identifier: MIT
#include "settings.h"
#include <QTest>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QFile>
#include <QJSEngine>
#include <KConfigGroup>

class SettingsTest : public QObject
{
    Q_OBJECT
    QTemporaryDir configDirectory;
private Q_SLOTS:
    void initTestCase() { qputenv("XDG_CONFIG_HOME", configDirectory.path().toUtf8()); }
    void init() {
        KConfigGroup(KSharedConfig::openConfig("sequoiataskbarrc"), "Taskbar").deleteGroup();
        KSharedConfig::openConfig("sequoiataskbarrc")->sync();
    }
    void defaultsApplyAndCancel() {
        TaskbarSettings settings;
        QCOMPARE(settings.values().value("iconSize").toInt(), 32);
        settings.setValue("iconSize", 48);
        QVERIFY(settings.dirty());
        settings.load();
        QCOMPARE(settings.values().value("iconSize").toInt(), 32);
        settings.setValue("iconSize", 48);
        QVERIFY(settings.save());
        TaskbarSettings second;
        QCOMPARE(second.values().value("iconSize").toInt(), 48);
        settings.defaults();
        QVERIFY(settings.dirty());
        QVERIFY(settings.save());
        second.load();
        QCOMPARE(second.values().value("iconSize").toInt(), 32);
    }
    void preserveConcurrentPins() {
        TaskbarSettings editor, panel;
        editor.setValue("spacing", 14);
        const QStringList pins{"applications:org.kde.kate.desktop"};
        panel.setValue("launchers", pins);
        QVERIFY(panel.save());
        QVERIFY(editor.save());
        panel.load();
        QCOMPARE(panel.values().value("launchers").toStringList(), pins);
        QCOMPARE(panel.values().value("spacing").toInt(), 14);
    }
    void rejectInvalidValues() {
        TaskbarSettings settings;
        settings.setValue("iconSize", 999);
        QCOMPARE(settings.values().value("iconSize").toInt(), 128);
        settings.setValue("spacing", -5);
        QCOMPARE(settings.values().value("spacing").toInt(), 0);
        settings.setValue("sizeMode", "invalid");
        QCOMPARE(settings.values().value("sizeMode").toString(), QString("fixed"));
        settings.setValue("surfaceMode", "dock");
        settings.setValue("dockHoverStyle", 1);
        settings.setValue("dockStrength", 18);
        settings.setValue("dockSpeed", 150);
        settings.setValue("dockAnimations", false);
        QVERIFY(settings.save());
        TaskbarSettings second;
        QCOMPARE(second.values().value("surfaceMode").toString(), QString("dock"));
        QCOMPARE(second.values().value("dockHoverStyle").toInt(), 1);
        QCOMPARE(second.values().value("dockStrength").toInt(), 18);
        QCOMPARE(second.values().value("dockSpeed").toInt(), 150);
        QVERIFY(!second.values().value("dockAnimations").toBool());
        settings.setValue("surfaceMode", "invalid");
        QCOMPARE(settings.values().value("surfaceMode").toString(), QString("panel"));
        settings.setValue("dockStrength", 900);
        QCOMPARE(settings.values().value("dockStrength").toInt(), 20);
        settings.setValue("dockSpeed", -1);
        QCOMPARE(settings.values().value("dockSpeed").toInt(), 50);
        settings.setValue("launchers", QStringList{"https://example.com", "applications:org.kde.kate.desktop", "applications:org.kde.kate.desktop"});
        QCOMPARE(settings.values().value("launchers").toStringList(), QStringList{"applications:org.kde.kate.desktop"});
    }
    void sizingAndOverflow() {
        QFile file(QStringLiteral(TASKBAR_SOURCE_DIR "/panel/contents/ui/Metrics.js"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString source = QString::fromUtf8(file.readAll());
        source.remove(".pragma library");
        QJSEngine engine;
        QVERIFY(!engine.evaluate(source).isError());
        QCOMPARE(engine.evaluate("iconSize('fixed', 32, 52, 6)").toInt(), 32);
        QCOMPARE(engine.evaluate("iconSize('fixed', 32, 96, 6)").toInt(), 32);
        QCOMPARE(engine.evaluate("iconSize('panel', 32, 96, 6)").toInt(), 84);
        QCOMPARE(engine.evaluate("iconSize('fixed', 64, 32, 4)").toInt(), 24);
        QCOMPARE(engine.evaluate("buttonLength('fill', 32, 6, 6, 4, 400)").toNumber(), 95.5);
        QCOMPARE(engine.evaluate("buttonLength('fill', 32, 6, 6, 20, 400)").toInt(), 44);
        QCOMPARE(engine.evaluate("offset('center', 400, 200)").toInt(), 100);
        QCOMPARE(engine.evaluate("offset('right', 100, 400)").toInt(), 0);
        QCOMPARE(engine.evaluate("naturalLength(32, 6, 6, 4)").toInt(), 194);
        QCOMPARE(engine.evaluate("naturalLength(32, 6, 6, 0)").toInt(), 0);
        QCOMPARE(engine.evaluate("naturalLength(32, 6, 6, 1)").toInt(), 44);
        QCOMPARE(engine.evaluate("naturalLength(32, 6, 6, 3)").toInt(), 144);
    }
};
QTEST_GUILESS_MAIN(SettingsTest)
#include "settings_test.moc"
