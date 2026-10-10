add_library(sequoiaelevencommon${QT_MAJOR_VERSION} OBJECT sequoiaelevenboxshadowrenderer.cpp)

set_target_properties(sequoiaelevencommon${QT_MAJOR_VERSION} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(sequoiaelevencommon${QT_MAJOR_VERSION}
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui)
