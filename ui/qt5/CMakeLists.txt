# Qt5 - http://qt-project.org/doc/qt-5.0/qtdoc/cmake-manual.html
set(CMAKE_PREFIX_PATH /usr/local/opt/qt5/)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
find_package(Qt5Widgets REQUIRED)

set(mettanode_SOURCES
    main.cpp
    ProfileEditor.cpp
    ContactModel.cpp
    CallService.cpp
    XcpApplication.cpp
    Meter.cpp)
set(mettanode_FORMS
    ProfileEditor.ui
    SingleCallWidget.ui)

set(mettanode_RESOURCES ui.qrc)

if (APPLE)
    list(APPEND mettanode_SOURCES macosx.mm)
    qt5_wrap_cpp(mettanode_HEADERS_MOC macsupport.h)
    set(MACOSX_BUNDLE_ICON_FILE "mettanode.icns")
    set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

set(mettanode_RESOURCES ui.qrc)
qt5_add_resources(mettanode_RESOURCES_RCC ${mettanode_RESOURCES})

add_executable(MettaNode ${GUI_TYPE}
    ${mettanode_SOURCES}
    ${mettanode_HEADERS_MOC}
    ${mettanode_RESOURCES}
    ${mettanode_RESOURCES_RCC}
    ${mettanode_FORMS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(MettaNode krypto rclient routing ssu arsenal natclient nat upnpc
    ${VOICEBOX_LIBS} opus
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

qt5_use_modules(MettaNode Widgets Quick Network)
qt5_wrap_ui(mettanode_FORMS_HEADERS ${mettanode_FORMS})

if (WIN32)
    target_link_libraries(MettaNode ws2_32 gdi32 winmm)
#   RC_FILE = gui.rc
endif (WIN32)

if (APPLE)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(COCOA_LIB Cocoa)
    target_link_libraries(MettaNode
        ${COREFOUNDATION_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${COCOA_LIB})
    set_target_properties(MettaNode PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/OSXInfo.plist)
endif (APPLE)

install(TARGETS MettaNode
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

