### configuration plugin
add_definitions(-DTRANSLATION_DOMAIN=\"sequoiaeleven_style_config\")
set(kstyle_sequoiaeleven_config_PART_SRCS sequoiaelevenstyleconfig.cpp)

set(kstyle_sequoiaeleven_config_PART_SRCS ${kstyle_sequoiaeleven_config_PART_SRCS} sequoiaelevenstyleconfigmodule.cpp)

kconfig_add_kcfg_files(kstyle_sequoiaeleven_config_PART_SRCS ../sequoiaelevenstyleconfigdata.kcfgc)
set(kstyle_sequoiaeleven_config_PART_FORMS ui/sequoiaelevenstyleconfig.ui)
ki18n_wrap_ui(kstyle_sequoiaeleven_config_PART_FORMS_HEADERS ${kstyle_sequoiaeleven_config_PART_FORMS})

kcoreaddons_add_plugin(sequoiaelevenstyleconfig SOURCES
    ${kstyle_sequoiaeleven_config_PART_SRCS}
    ${kstyle_sequoiaeleven_config_PART_FORMS_HEADERS}
    INSTALL_NAMESPACE "kstyle_config"
)

target_link_libraries(sequoiaelevenstyleconfig Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus)
target_link_libraries(sequoiaelevenstyleconfig KF6::CoreAddons KF6::ConfigCore KF6::ConfigWidgets KF6::I18n)
if(QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(sequoiaelevenstyleconfig KF6::KCMUtils)
endif()
kcmutils_generate_desktop_file(sequoiaelevenstyleconfig)

set(sequoiaeleven_settings_SOURCES main.cpp)
add_executable(sequoiaeleven-settings ${sequoiaeleven_settings_SOURCES} )
target_link_libraries(sequoiaeleven-settings Qt6::Core Qt6::Gui Qt6::Widgets )
target_link_libraries(sequoiaeleven-settings KF6::I18n KF6::KCMUtils KF6::CoreAddons)

set_target_properties(sequoiaeleven-settings PROPERTIES OUTPUT_NAME sequoiaeleven-settings${QT_MAJOR_VERSION})

install(TARGETS sequoiaeleven-settings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# icon
ecm_install_icons(ICONS sc-apps-sequoiaeleven-settings.svgz
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor )


