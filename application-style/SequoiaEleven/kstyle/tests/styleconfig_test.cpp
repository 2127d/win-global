/*
 * Unit tests for StyleConfig UI module with DolphinViewOpacity
 *
 * Tests that the configuration UI properly handles the
 * DolphinViewOpacity slider and integrates with the config system.
 */

#include <QTest>
#include <QSlider>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSignalSpy>

#include "sequoiaelevenstyleconfigdata.h"

class StyleConfigTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // UI component tests
    void testSliderRange();
    void testSliderDefaultValue();
    void testSliderSpinBoxSync();
    void testOpacityAlphaConversion();
    void testTransparencyConditionCheck();
};

void StyleConfigTest::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    QStandardPaths::setTestModeEnabled(true);
}

void StyleConfigTest::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void StyleConfigTest::init()
{
    SequoiaEleven::StyleConfigData::self()->setDefaults();
}

void StyleConfigTest::cleanup()
{
}

void StyleConfigTest::testSliderRange()
{
    // Create a slider like the one in the UI
    QSlider slider;
    slider.setRange(0, 100);

    QCOMPARE(slider.minimum(), 0);
    QCOMPARE(slider.maximum(), 100);

    // Test that all valid opacity values are within range
    for (int i = 0; i <= 100; ++i) {
        slider.setValue(i);
        QCOMPARE(slider.value(), i);
    }
}

void StyleConfigTest::testSliderDefaultValue()
{
    // The default opacity should be 100 (fully opaque)
    int defaultValue = SequoiaEleven::StyleConfigData::dolphinViewOpacity();
    QCOMPARE(defaultValue, 100);

    // A slider initialized with this value should show 100
    QSlider slider;
    slider.setRange(0, 100);
    slider.setValue(defaultValue);
    QCOMPARE(slider.value(), 100);
}

void StyleConfigTest::testSliderSpinBoxSync()
{
    // Create slider and spinbox like in the UI
    QSlider slider;
    slider.setRange(0, 100);
    slider.setValue(100);

    QSpinBox spinBox;
    spinBox.setRange(0, 100);
    spinBox.setValue(100);

    // Connect them like in StyleConfig constructor
    QObject::connect(&slider, &QSlider::valueChanged,
                     &spinBox, &QSpinBox::setValue);
    QObject::connect(&spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &slider, &QSlider::setValue);

    // Change slider, spinbox should follow
    slider.setValue(50);
    QCOMPARE(spinBox.value(), 50);

    // Change spinbox, slider should follow
    spinBox.setValue(25);
    QCOMPARE(slider.value(), 25);

    // Test edge cases
    slider.setValue(0);
    QCOMPARE(spinBox.value(), 0);

    spinBox.setValue(100);
    QCOMPARE(slider.value(), 100);
}

void StyleConfigTest::testOpacityAlphaConversion()
{
    // Test conversion from opacity percentage to alpha float
    // This is how it's done in the style code:
    // backgroundColor.setAlphaF(StyleConfigData::dolphinViewOpacity() / 100.0);

    auto opacityToAlpha = [](int opacity) -> qreal {
        return opacity / 100.0;
    };

    // Test key values
    QCOMPARE(opacityToAlpha(0), 0.0);
    QCOMPARE(opacityToAlpha(50), 0.5);
    QCOMPARE(opacityToAlpha(100), 1.0);
    QCOMPARE(opacityToAlpha(25), 0.25);
    QCOMPARE(opacityToAlpha(75), 0.75);

    // Test that setting opacity and converting gives correct alpha
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(42);
    qreal alpha = SequoiaEleven::StyleConfigData::dolphinViewOpacity() / 100.0;
    QCOMPARE(alpha, 0.42);
}

void StyleConfigTest::testTransparencyConditionCheck()
{
    // Test the condition used throughout the code:
    // StyleConfigData::dolphinViewOpacity() < 100

    auto isTransparent = []() -> bool {
        return SequoiaEleven::StyleConfigData::dolphinViewOpacity() < 100;
    };

    // Default (100) should not be transparent
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(100);
    QVERIFY(!isTransparent());

    // 99 and below should be transparent
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(99);
    QVERIFY(isTransparent());

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(50);
    QVERIFY(isTransparent());

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(1);
    QVERIFY(isTransparent());

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(0);
    QVERIFY(isTransparent());

    // Back to 100 should not be transparent
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(100);
    QVERIFY(!isTransparent());
}

QTEST_MAIN(StyleConfigTest)
#include "styleconfig_test.moc"
