set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE) # upnp stuff :(
include(${QT_USE_FILE})
include_directories(../lib ../../sst/lib ../../routing/lib ../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11)
include_directories(../3rdparty/metakit-2.4.9.7/include ../../nat)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(mettanode_SOURCES main.cc search.cc chat.cc chathistory.cpp view.cc save.cc settings.cc logarea.cc logwindow.cpp meter.cc)
set(mettanode_HEADERS chat.h chathistory.h logarea.h logwindow.h main.h meter.h save.h search.h settings.h)
set(mettanode_RESOURCES gui.qrc)

if (APPLE)
    list(APPEND mettanode_SOURCES macsupport.mm)
    list(APPEND mettanode_HEADERS macsupport.h)
	set(MACOSX_BUNDLE_ICON_FILE "mettanode.icns")
	set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

qt4_wrap_cpp(mettanode_HEADERS_MOC ${mettanode_HEADERS})
qt4_add_resources(mettanode_RESOURCES_RCC ${mettanode_RESOURCES})

add_executable(MettaNode ${GUI_TYPE} ${mettanode_SOURCES} ${mettanode_HEADERS_MOC} ${mettanode_RESOURCES_RCC} ${APPLE_BUNDLE_SRC})
target_link_libraries(MettaNode sst routing netsteria opus rtaudio mk4 upnp ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

if (WIN32)
    target_link_libraries(MettaNode ws2_32 gdi32 winmm)
#   RC_FILE = gui.rc
endif (WIN32)

if (APPLE)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(COCOA_LIB Cocoa)
    target_link_libraries(MettaNode ${COREFOUNDATION_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${COCOA_LIB})
    set_target_properties(MettaNode PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MettaNode-Info.plist)
endif (APPLE)

if (UNIX AND NOT APPLE)
	target_link_libraries(MettaNode asound rt)
endif (UNIX AND NOT APPLE)

install(TARGETS MettaNode
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
