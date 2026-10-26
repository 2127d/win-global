/* Regression tests exercise the real application-style settings widget. */
#include <QTest>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDir>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>

#include "../config/sequoiaelevenstyleconfig.h"
#include "sequoiaelevenstyleconfigdata.h"

using SequoiaEleven::StyleConfig;
using SequoiaEleven::StyleConfigData;

class StyleConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init()
    {
        StyleConfigData::self()->setDefaults();
        QVERIFY(StyleConfigData::self()->save());
    }

    void cleanLoadAndReset()
    {
        StyleConfig widget(nullptr);
        QSignalSpy changed(&widget, &StyleConfig::changed);
        for (int i = 0; i < 3; ++i) {
            widget.reset();
            QVERIFY(!changed.last().at(0).toBool());
            QCOMPARE(widget.findChild<QComboBox *>("_shadowSize")->count(), 5);
        }
        auto rounded = widget.findChild<QCheckBox *>("_roundedRubberBandFrame");
        rounded->toggle();
        QVERIFY(changed.last().at(0).toBool());
        rounded->toggle();
        QVERIFY(!changed.last().at(0).toBool());
        auto strength = widget.findChild<QSpinBox *>("_shadowStrength");
        strength->setValue(180);
        QVERIFY(changed.last().at(0).toBool());
        widget.save();
        QVERIFY(!changed.last().at(0).toBool());
        QCOMPARE(StyleConfigData::shadowStrength(), 180);
    }

    void defaultsArePendingUntilSaved()
    {
        StyleConfigData::setMenuOpacity(45);
        StyleConfigData::setOldTabbar(true);
        StyleConfigData::setViewDrawTreeBranchLines(true);
        QVERIFY(StyleConfigData::self()->save());
        StyleConfig widget(nullptr);
        QSignalSpy changed(&widget, &StyleConfig::changed);
        widget.defaults();
        QCOMPARE(widget.findChild<QSlider *>("_menuOpacity")->value(), 86);
        QVERIFY(widget.findChild<QRadioButton *>("_defaultTabbar")->isChecked());
        QVERIFY(changed.last().at(0).toBool());
        QCOMPARE(StyleConfigData::menuOpacity(), 45);
        KConfig disk(QStringLiteral("sequoiaelevenrc"), KConfig::SimpleConfig);
        QCOMPARE(KConfigGroup(&disk, "Style").readEntry("MenuOpacity", 0), 45);
        widget.reset();
        QCOMPARE(widget.findChild<QSlider *>("_menuOpacity")->value(), 45);
        QVERIFY(widget.findChild<QRadioButton *>("_oldTabbar")->isChecked());
        QVERIFY(!changed.last().at(0).toBool());
        widget.defaults();
        widget.save();
        widget.reset();
        QCOMPARE(widget.findChild<QSlider *>("_menuOpacity")->value(), 86);
        QVERIFY(!widget.findChild<QCheckBox *>("_viewDrawTreeBranchLines")->isChecked());
        QVERIFY(!changed.last().at(0).toBool());
    }

    void supportedOptionsRoundTrip_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<QVariant>("value");
        for (const char *key : {"ViewDrawTreeBranchLines", "ViewInvertSortIndicator", "TabBarTabExpandFullWidth", "StackedWidgetTransitionsEnabled"})
            QTest::newRow(key) << QString::fromLatin1(key) << QVariant(true);
        for (const char *key : {"ProgressBarAnimated", "SplitterProxyEnabled"})
            QTest::newRow(key) << QString::fromLatin1(key) << QVariant(false);
        QTest::newRow("busy duration") << QStringLiteral("ProgressBarBusyStepDuration") << QVariant(1200);
        QTest::newRow("splitter width") << QStringLiteral("SplitterProxyWidth") << QVariant(18);
    }

    void supportedOptionsRoundTrip()
    {
        QFETCH(QString, key);
        QFETCH(QVariant, value);
        StyleConfig widget(nullptr);
        QSignalSpy changed(&widget, &StyleConfig::changed);
        QString name = key;
        name[0] = name[0].toLower();
        auto control = widget.findChild<QWidget *>(QLatin1Char('_') + name);
        QVERIFY(control);
        if (auto check = qobject_cast<QCheckBox *>(control)) check->setChecked(value.toBool());
        else qobject_cast<QSpinBox *>(control)->setValue(value.toInt());
        QVERIFY(changed.last().at(0).toBool());
        widget.save();
        widget.reset();
        QCOMPARE(StyleConfigData::self()->findItem(key)->property(), value);
        QVERIFY(!changed.last().at(0).toBool());
    }

    void dependentControls()
    {
        StyleConfig widget(nullptr);
        auto check = [&](const char *name) { return widget.findChild<QCheckBox *>(name); };
        auto enabled = [&](const char *name) { return widget.findChild<QWidget *>(name)->isEnabled(); };
        check("_animationsEnabled")->setChecked(false);
        QVERIFY(!enabled("_animationsDuration"));
        QVERIFY(!enabled("_stackedWidgetTransitionsEnabled"));
        QVERIFY(enabled("_progressBarBusyStepDuration")); // Busy indicator is independent.
        check("_progressBarAnimated")->setChecked(false);
        QVERIFY(!enabled("_progressBarBusyStepDuration"));
        check("_scrollBarTransient")->setChecked(false);
        QVERIFY(!enabled("_scrollBarTransientAlwaysShowSlim"));
        check("_splitterProxyEnabled")->setChecked(false);
        QVERIFY(!enabled("_splitterProxyWidth"));
        QVERIFY(!enabled("_tabBarTabExpandFullWidth"));
        check("_documentModeTabs")->setChecked(true);
        QVERIFY(enabled("_tabBarTabExpandFullWidth"));
        check("_noOutline")->setChecked(true);
        QVERIFY(!enabled("_fullOutline"));
        widget.findChild<QComboBox *>("_shadowSize")->setCurrentIndex(0);
        QVERIFY(!enabled("_shadowStrength"));
        widget.reset();
        QVERIFY(enabled("_animationsDuration"));
        QVERIFY(enabled("_shadowStrength"));
        QVERIFY(enabled("_fullOutline"));
        QVERIFY(!widget.findChild<QWidget *>("_widgetToolBarShadow"));
    }

    void opacityControlsStaySynchronized()
    {
        StyleConfig widget(nullptr);
        for (const char *name : {"_menuOpacity", "_sidebarOpacity", "_viewOpacity", "_menuBarOpacity", "_toolBarOpacity", "_tabBarOpacity"}) {
            auto slider = widget.findChild<QSlider *>(name);
            auto spin = widget.findChild<QSpinBox *>(QString::fromLatin1(name) + "SpinBox");
            slider->setValue(42);
            QCOMPARE(spin->value(), 42);
            spin->setValue(67);
            QCOMPARE(slider->value(), 67);
        }
        widget.save();
        widget.reset();
        QCOMPARE(StyleConfigData::menuOpacity(), 67);
        QCOMPARE(StyleConfigData::dolphinViewOpacity(), 67);
    }

    void previewPages()
    {
        const QString output = qEnvironmentVariable("SEQUOIA_SETTINGS_PREVIEW_DIR");
        if (output.isEmpty()) QSKIP("Optional visual preview");
        QVERIFY(QDir().mkpath(output));
        StyleConfig widget(nullptr);
        widget.resize(780, 620);
        widget.show();
        auto tabs = widget.findChild<QTabWidget *>("tabWidget");
        for (int i = 0; i < tabs->count(); ++i) {
            tabs->setCurrentIndex(i);
            QTest::qWait(40);
            QVERIFY(widget.grab().save(output + '/' + tabs->widget(i)->objectName() + ".png"));
        }
    }
};

int main(int argc, char **argv)
{
    // Isolate configuration before Qt or KConfig initializes; never touch user preferences.
    QTemporaryDir config;
    if (!config.isValid()) return 1;
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("sequoiaeleven_style_config");
    StyleConfigTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "styleconfig_test.moc"
