/*
 * Unit tests for DolphinViewOpacity configuration
 *
 * Tests the new DolphinViewOpacity setting that controls
 * transparency of Dolphin's main file view area.
 */

#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>

#include "sequoiaelevenstyleconfigdata.h"

class DolphinViewOpacityTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Configuration tests
    void testDefaultValue();
    void testSetAndGetValue();
    void testBoundaryValues();
    void testPersistence();
    void testIndependentFromSidebarOpacity();
};

void DolphinViewOpacityTest::initTestCase()
{
    // Use temporary directory for config files
    QVERIFY(m_tempDir.isValid());

}

void DolphinViewOpacityTest::cleanupTestCase()
{

}

void DolphinViewOpacityTest::init()
{
    // Reset to defaults before each test
    SequoiaEleven::StyleConfigData::self()->setDefaults();
}

void DolphinViewOpacityTest::cleanup()
{
    // Nothing to clean up
}

void DolphinViewOpacityTest::testDefaultValue()
{
    // The default value should be 100 (fully opaque)
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 100);
}

void DolphinViewOpacityTest::testSetAndGetValue()
{
    // Test setting various valid values
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(50);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 50);

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(0);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 0);

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(75);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 75);

    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(100);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 100);
}

void DolphinViewOpacityTest::testBoundaryValues()
{
    // Test minimum value (0 = fully transparent)
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(0);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 0);

    // Test maximum value (100 = fully opaque)
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(100);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 100);

    // Test value at 1 (near minimum)
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(1);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 1);

    // Test value at 99 (near maximum)
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(99);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 99);
}

void DolphinViewOpacityTest::testPersistence()
{
    // Set a non-default value
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(42);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 42);

    // Save configuration
    QVERIFY(SequoiaEleven::StyleConfigData::self()->save());

    // Reset to defaults
    SequoiaEleven::StyleConfigData::self()->setDefaults();
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 100);

    // Reload configuration
    SequoiaEleven::StyleConfigData::self()->load();
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 42);
}

void DolphinViewOpacityTest::testIndependentFromSidebarOpacity()
{
    // Set different values for view and sidebar opacity
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(30);
    SequoiaEleven::StyleConfigData::setDolphinSidebarOpacity(70);

    // Verify they are independent
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 30);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinSidebarOpacity(), 70);

    // Change one, verify the other is unaffected
    SequoiaEleven::StyleConfigData::setDolphinViewOpacity(50);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 50);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinSidebarOpacity(), 70);

    SequoiaEleven::StyleConfigData::setDolphinSidebarOpacity(25);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinViewOpacity(), 50);
    QCOMPARE(SequoiaEleven::StyleConfigData::dolphinSidebarOpacity(), 25);
}

int main(int argc, char **argv)
{
    QTemporaryDir config;
    if (!config.isValid()) return 1;
    qputenv("XDG_CONFIG_HOME", config.path().toUtf8());
    QApplication app(argc, argv);
    DolphinViewOpacityTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "dolphinviewopacity_test.moc"
