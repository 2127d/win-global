kcmutils_add_qml_kcm(kcm_sequoia_taskbar SOURCES kcm.cpp)
set_target_properties(kcm_sequoia_taskbar PROPERTIES PREFIX "")
target_link_libraries(kcm_sequoia_taskbar PRIVATE taskbarsettings KF6::KCMUtilsQuick KF6::I18n)
