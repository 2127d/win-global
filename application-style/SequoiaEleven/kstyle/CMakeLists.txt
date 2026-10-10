################# includes #################
include_directories(
    animations
    debug
    ${CMAKE_SOURCE_DIR}/libsequoiaelevencommon
    ${CMAKE_BINARY_DIR}/libsequoiaelevencommon
)

################# configuration #################
configure_file(config-sequoiaeleven.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-sequoiaeleven.h)

########### next target ###############
set(sequoiaeleven_PART_SRCS
    animations/sequoiaelevenanimation.cpp
    animations/sequoiaelevenanimations.cpp
    animations/sequoiaelevenanimationdata.cpp
    animations/sequoiaelevenbaseengine.cpp
    animations/sequoiaelevenbusyindicatordata.cpp
    animations/sequoiaelevenbusyindicatorengine.cpp
    animations/sequoiaelevendialdata.cpp
    animations/sequoiaelevendialengine.cpp
    animations/sequoiaelevenenabledata.cpp
    animations/sequoiaelevengenericdata.cpp
    animations/sequoiaelevenheaderviewdata.cpp
    animations/sequoiaelevenheaderviewengine.cpp
    animations/sequoiaelevenscrollbardata.cpp
    animations/sequoiaelevenscrollbarengine.cpp
    animations/sequoiaelevenspinboxengine.cpp
    animations/sequoiaelevenspinboxdata.cpp
    animations/sequoiaelevenstackedwidgetdata.cpp
    animations/sequoiaelevenstackedwidgetengine.cpp
    animations/sequoiaeleventabbarengine.cpp
    animations/sequoiaeleventabbardata.cpp
    animations/sequoiaeleventoolboxengine.cpp
    animations/sequoiaeleventransitiondata.cpp
    animations/sequoiaeleventransitionwidget.cpp
    animations/sequoiaelevenwidgetstateengine.cpp
    animations/sequoiaelevenwidgetstatedata.cpp
    debug/sequoiaelevenwidgetexplorer.cpp
    sequoiaelevenaddeventfilter.cpp
    sequoiaelevenblurhelper.cpp
    sequoiaelevenframeshadow.cpp
    sequoiaelevenhelper.cpp
    sequoiaelevenmdiwindowshadow.cpp
    sequoiaelevenmnemonics.cpp
    sequoiaelevenpropertynames.cpp
    sequoiaelevenshadowhelper.cpp
    sequoiaelevensplitterproxy.cpp
    sequoiaelevenstyle.cpp
    sequoiaelevenstyleplugin.cpp
    sequoiaeleventileset.cpp
    sequoiaelevenwindowmanager.cpp
    sequoiaeleventoolsareamanager.cpp
)

kconfig_add_kcfg_files(sequoiaeleven_PART_SRCS ../kdecoration/sequoiaelevensettings.kcfgc)
kconfig_add_kcfg_files(sequoiaeleven_PART_SRCS sequoiaelevenstyleconfigdata.kcfgc)

add_library(sequoiaeleven${QT_MAJOR_VERSION} MODULE ${sequoiaeleven_PART_SRCS})

################# find Qt modules #################
if(QT_MAJOR_VERSION EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickWidgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Quick QuickWidgets)
endif()

################# link Qt libraries #################
target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION}
    PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Quick
)

if( SEQUOIAELEVEN_HAVE_QTQUICK )
    target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION}
        PRIVATE
            Qt${QT_MAJOR_VERSION}::QuickWidgets
            KF${QT_MAJOR_VERSION}::CoreAddons
    )
endif()

################# link KDE Frameworks #################
target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION}
    PRIVATE
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigCore
    KF${QT_MAJOR_VERSION}::ConfigGui
    KF${QT_MAJOR_VERSION}::GuiAddons
    KF${QT_MAJOR_VERSION}::IconThemes
    KF${QT_MAJOR_VERSION}::WindowSystem
)

if(QT_MAJOR_VERSION STREQUAL "5")
    target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE KF5::ConfigWidgets)
    if(SEQUOIAELEVEN_HAVE_QTQUICK)
        target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE KF5::Kirigami2)
    endif()
else()
    target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE KF6::ColorScheme)
    if(SEQUOIAELEVEN_HAVE_QTQUICK)
        target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE KF6::KirigamiPlatform)
    endif()
endif()

################# link internal library #################
target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE sequoiaelevencommon${QT_MAJOR_VERSION})

################# KDE style integration #################
if(KF${QT_MAJOR_VERSION}FrameworkIntegration_FOUND)
    target_link_libraries(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE KF${QT_MAJOR_VERSION}::Style)
endif()

if (WIN32)
    # As stated in https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants M_PI only gets defined
    # when if _USE_MATH_DEFINES is defined
    target_compile_definitions(sequoiaeleven${QT_MAJOR_VERSION} PRIVATE _USE_MATH_DEFINES _BSD_SOURCE)
endif()


########### install files ###############
if (FOR_FLATPAK)
    set(LIB_TARGET ${QT_PLUGINS_DIR}/styles/)
else()
    set(LIB_TARGET ${KDE_INSTALL_QTPLUGINDIR}/styles/)
endif()
install(TARGETS sequoiaeleven${QT_MAJOR_VERSION} DESTINATION ${LIB_TARGET})

if (NOT FOR_FLATPAK)
    install(FILES sequoiaeleven.themerc  DESTINATION  ${KDE_INSTALL_DATADIR}/kstyle/themes)
endif()

########### subdirectories ###############

if (QT_MAJOR_VERSION EQUAL "6" AND TARGET "KF6::KCMUtils" AND NOT FOR_FLATPAK)
    add_subdirectory(config)
endif()

########### tests ###############
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
