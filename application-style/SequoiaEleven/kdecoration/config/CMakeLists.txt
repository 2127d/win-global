#####  config classes ææææææ

set(sequoiaelevendecoration_config_SRCS
  ../sequoiaelevenexceptionlist.cpp
    sequoiaelevenconfigwidget.cpp
    sequoiaelevendetectwidget.cpp
    sequoiaelevenexceptiondialog.cpp
    sequoiaelevenexceptionlistwidget.cpp
    sequoiaelevenexceptionmodel.cpp
    sequoiaelevenitemmodel.cpp
)
ki18n_wrap_ui(sequoiaelevendecoration_config_SRCS
    ui/sequoiaelevenconfigurationui.ui
    ui/sequoiaelevendetectwidget.ui
    ui/sequoiaelevenexceptiondialog.ui
    ui/sequoiaelevenexceptionlistwidget.ui
)

kcoreaddons_add_plugin(kcm_sequoiaelevendecoration SOURCES kcm_sequoiaelevendecoration.cpp ${sequoiaelevendecoration_config_SRCS} INSTALL_NAMESPACE "${KDECORATION_KCM_PLUGIN_DIR}")
kconfig_add_kcfg_files(kcm_sequoiaelevendecoration ../sequoiaelevensettings.kcfgc)
target_include_directories(kcm_sequoiaelevendecoration PRIVATE ${CMAKE_SOURCE_DIR}/kdecoration ${CMAKE_BINARY_DIR}/kdecoration/)
target_link_libraries(kcm_sequoiaelevendecoration
     PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
        KDecoration3::KDecoration
     PRIVATE
        KF6::CoreAddons
        KF6::ConfigWidgets
        KF6::GuiAddons
        KF6::I18n
        KF6::IconThemes
        KF6::WindowSystem
        KF6::KCMUtils
)
kcmutils_generate_desktop_file(kcm_sequoiaelevendecoration)
