# Qt5 - http://qt-project.org/doc/qt-5.0/qtdoc/cmake-manual.html
# Tell CMake to run moc when necessary:
#set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
#find_package(Qt5Widgets REQUIRED)

#add_executable(PeerPicker main.cpp PeerPickerWindow.ui)

#qt5_use_modules(PeerPicker Widgets Declarative)

# Qt4
set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# Old UI includes:
#include_directories(../lib ../../sst/lib ../../routing/lib ../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11)
#include_directories(../3rdparty/metakit-2.4.9.7/include ../../nat)

set(mettanode_SOURCES main.cpp PeerTableModel.cpp PeerPicker.cpp)
set(mettanode_HEADERS PeerTableModel.h PeerPicker.h)
set(mettanode_FORMS PeerPickerWindow.ui)
set(mettanode_RESOURCES ui.qrc)

if (APPLE)
    list(APPEND mettanode_SOURCES macosx.mm)
    list(APPEND mettanode_HEADERS macsupport.h)
    set(MACOSX_BUNDLE_ICON_FILE "mettanode.icns")
    set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

qt4_wrap_cpp(mettanode_HEADERS_MOC ${mettanode_HEADERS})
qt4_wrap_ui(mettanode_FORMS_HEADERS ${mettanode_FORMS})
qt4_add_resources(mettanode_RESOURCES_RCC ${mettanode_RESOURCES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(MettaNode ${GUI_TYPE}
    ${mettanode_SOURCES}
    ${mettanode_HEADERS_MOC}
    ${mettanode_RESOURCES_RCC}
    ${mettanode_FORMS_HEADERS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(MettaNode krypto rclient routing ssu support natclient nat upnpc
    ${VOICEBOX_LIBS} opus
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

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

