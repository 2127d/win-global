add_definitions(-DTRANSLATION_DOMAIN="sequoiaeleven_kwin_deco")

################# configuration #################
configure_file(config-sequoiaeleven.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-sequoiaeleven.h )

################# includes #################
include_directories(${CMAKE_SOURCE_DIR}/libsequoiaelevencommon)
include_directories(${CMAKE_BINARY_DIR}/libsequoiaelevencommon)

################# newt target #################
### plugin classes
set(sequoiaelevendecoration_SRCS
    sequoiaelevenbutton.cpp
    sequoiaelevendecoration.cpp
    sequoiaelevenexceptionlist.cpp
    sequoiaelevensettingsprovider.cpp
)
### build library
kconfig_add_kcfg_files(sequoiaelevendecoration_SRCS sequoiaelevensettings.kcfgc)
foreach(material IN ITEMS legacy solid transparent)
    set(decoration_target sequoiaelevendecoration)
    set(decoration_id org.kde.sequoiaeleven)
    if(NOT material STREQUAL "legacy")
        string(APPEND decoration_target "_${material}")
        string(APPEND decoration_id ".${material}")
    endif()
    add_library(${decoration_target} MODULE
        ${sequoiaelevendecoration_SRCS}
        ${sequoiaelevendecoration_config_SRCS}
        ${sequoiaelevendecoration_config_PART_FORMS_HEADERS})

    set_target_properties(${decoration_target} PROPERTIES
        OUTPUT_NAME ${decoration_id}
    )

    target_link_libraries(${decoration_target}
         PRIVATE
            sequoiaelevencommon6
            KF6::CoreAddons
            KF6::ConfigWidgets
            KF6::GuiAddons
            KF6::I18n
            KF6::IconThemes
            KF6::WindowSystem
            KF6::KCMUtils
            Qt6::DBus
            KDecoration3::KDecoration
    )
    install(TARGETS ${decoration_target} DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})

    if(NOT material STREQUAL "legacy")
        string(TOUPPER "${material}" material_define)
        target_compile_definitions(${decoration_target} PRIVATE "SEQUOIA_${material_define}_DECORATION")
    endif()
endforeach()
install(FILES config/sequoiaelevendecorationconfig.desktop DESTINATION  ${KDE_INSTALL_DATADIR}/kservices6)
add_subdirectory(config)
