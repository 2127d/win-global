
find_package(Qt6 REQUIRED COMPONENTS Svg)
add_executable(systemsettings)
ecm_add_qml_module(systemsettings URI org.kde.systemsettings GENERATE_PLUGIN_SOURCE)

ki18n_wrap_ui(systemsettings externalModule.ui)

target_sources(systemsettings PRIVATE
    DesktopRenderController.cpp
    DesktopRenderController.h
    DesktopPreviewImage.h
    HubGlyph.h
    HubIslandRim.h
    AppearancePreview.cpp
    AppearancePreview.h
    ../../../../desktop/glass-material/glassrim.cpp
    ../../../../desktop/glass-material/glassrim.h
    SettingsNavigation.cpp
    SettingsNavigation.h
    main.cpp
    SettingsBase.cpp
    SettingsBase.h
    SidebarMode.cpp
    SidebarMode.h
    ExternalAppModule.cpp
    ExternalAppModule.h
    kcmmetadatahelpers.h
    MenuItem.cpp
    MenuItem.h
    MenuModel.cpp
    MenuModel.h
    MenuProxyModel.cpp
    MenuProxyModel.h
    ModuleView.cpp
    ModuleView.h
)

ecm_target_qml_sources(systemsettings SOURCES
    qml/HubIcon.qml
    qml/BatteryHero.qml
    qml/HubTile.qml
    qml/CategoryOverview.qml
    qml/HubPanel.qml
    qml/DesktopPreview.qml
    qml/DesktopPreviewSurface.qml
    qml/AppearanceHub.qml
    qml/AppearanceModePill.qml
    qml/AppearanceThemePill.qml
    qml/HubOverview.qml
    qml/HubSidebar.qml
    qml/LegacyIntroPage.qml
    qml/LegacyMain.qml
    qml/CategoriesPage.qml
    qml/CategoryItem.qml
    qml/HamburgerMenuButton.qml
    qml/IntroPage.qml
    qml/Main.qml
    qml/SideBarItem.qml
    qml/SubCategoryPage.qml
    RESOURCES
    images/plasma-logo.svg
    images/hub/home.svg
    images/hub/appearance.svg
    images/hub/display.svg
    images/hub/devices.svg
    images/hub/network.svg
    images/hub/desktop.svg
    images/hub/power.svg
    images/hub/accessibility.svg
    images/hub/system.svg
    images/hub/settings.svg
    images/hub/all.svg
    images/hub/leaf.svg
    images/hub/sleep.svg
    images/hub/gauge.svg
    images/hub/eye.svg
    images/hub/ear.svg
    images/hub/keyboard.svg
    images/hub/mouse.svg
    images/hub/pointer.svg
    images/hub/lock.svg
    images/hub/wallet.svg
    images/hub/login.svg
    images/hub/language.svg
    images/hub/clock.svg
    images/hub/check.svg
    images/hub/refresh.svg
    images/hub/chip.svg
    images/hub/bell.svg
    images/hub/bluetooth.svg
    images/hub/sound.svg
    images/hub/search.svg
    images/hub/sparkles.svg
    images/hub/sliders.svg
    images/hub/folder.svg
    images/hub/screen.svg
    images/hub/info.svg

)

target_compile_definitions(systemsettings PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(systemsettings PRIVATE
    systemsettings_debug
    KF6::Crash
    KF6::ItemViews
    KF6::ItemModels
    KF6::KCMUtils
    KF6::I18n
    KF6::IconThemes
    KF6::KIOWidgets
    KF6::Service
    KF6::WindowSystem
    KF6::XmlGui
    KF6::DBusAddons
    KF6::ConfigGui
    KF6::GuiAddons # UrlHanlder handles help:/ urls
    KF6::AuthCore
    KF6::I18nQml
    KF6::ColorScheme
    Plasma::Activities
    Qt::Svg
    Qt::QuickWidgets
    Qt::Qml
    Qt::Quick
)

kconfig_target_kcfg_file(systemsettings FILE systemsettings-config.kcfg CLASS_NAME Settings MUTATORS)

install(TARGETS systemsettings ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS kdesystemsettings.desktop systemsettings.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(DIRECTORY DESTINATION "${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel")
install(
    CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"${KDE_INSTALL_FULL_APPDIR}/systemsettings.desktop\" \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel/systemsettings.desktop\")"
)
install(FILES org.kde.systemsettings.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES systemsettings.zsh RENAME _systemsettings DESTINATION ${KDE_INSTALL_ZSHAUTOCOMPLETEDIR})
install(FILES systemsettings-config.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

ecm_finalize_qml_module(systemsettings)
find_program(SEQUOIA_QSB NAMES qsb HINTS /usr/lib64/qt6/bin /usr/lib/qt6/bin REQUIRED)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-preview.frag.qsb"
    COMMAND "${SEQUOIA_QSB}" --glsl "100 es,120,150" --hlsl 50 --msl 12
        -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-preview.frag.qsb" "${CMAKE_CURRENT_SOURCE_DIR}/qml/desktop-preview.frag"
    DEPENDS "qml/desktop-preview.frag" VERBATIM)
qt_add_resources(systemsettings hub_preview_shaders
    PREFIX "/settings-hub"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES "${CMAKE_CURRENT_BINARY_DIR}/desktop-preview.frag.qsb"
)
