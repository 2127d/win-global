# Tests for SequoiaEleven KStyle

include(ECMAddTests)

# Use the correct Qt version based on major version
if(QT_MAJOR_VERSION EQUAL 5)
    set(TEST_QT_VERSION ${QT5_MIN_VERSION})
else()
    set(TEST_QT_VERSION ${QT_MIN_VERSION})
endif()

find_package(Qt${QT_MAJOR_VERSION} ${TEST_QT_VERSION} CONFIG REQUIRED COMPONENTS Test Widgets)

# Generate the config data for tests
kconfig_add_kcfg_files(test_config_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../sequoiaelevenstyleconfigdata.kcfgc)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Test for DolphinViewOpacity configuration
ecm_add_test(
    dolphinviewopacity_test.cpp
    ${test_config_SRCS}
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::ConfigGui
    TEST_NAME
        dolphinviewopacity_test${QT_MAJOR_VERSION}
)

# Test for blur helper behavior (config-only tests, no actual blur helper)
ecm_add_test(
    blurhelper_test.cpp
    ${test_config_SRCS}
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::ConfigGui
    TEST_NAME
        blurhelper_test${QT_MAJOR_VERSION}
)

# Exercise the real dialog, including generated Designer controls and persistence.
ki18n_wrap_ui(styleconfig_test_UI ../config/ui/sequoiaelevenstyleconfig.ui)
ecm_add_test(
    styleconfig_test.cpp
    ../config/sequoiaelevenstyleconfig.cpp
    ../config/sequoiaelevenstyleconfig.h
    ${styleconfig_test_UI}
    ${test_config_SRCS}
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::ConfigGui
    TEST_NAME
        styleconfig_test${QT_MAJOR_VERSION}
)
target_include_directories(styleconfig_test${QT_MAJOR_VERSION} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../config)
target_link_libraries(styleconfig_test${QT_MAJOR_VERSION} Qt${QT_MAJOR_VERSION}::DBus KF${QT_MAJOR_VERSION}::ConfigWidgets KF${QT_MAJOR_VERSION}::I18n)
set_tests_properties(styleconfig_test${QT_MAJOR_VERSION} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_STYLE_OVERRIDE=Fusion")
