set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../lib ../../sst/lib ../../routing/lib ../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(netsteria_SOURCES main.cc search.cc chat.cc view.cc save.cc settings.cc logarea.cc meter.cc)
set(netsteria_HEADERS chat.h logarea.h main.h meter.h save.h search.h settings.h)
set(netsteria_RESOURCES gui.qrc)

if (APPLE)
        set(netsteria_SOURCES ${netsteria_SOURCES} macsupport.mm)
        set(netsteria_HEADERS ${netsteria_HEADERS} macsupport.h)
endif (APPLE)

qt4_wrap_cpp(netsteria_HEADERS_MOC ${netsteria_HEADERS})
qt4_add_resources(netsteria_RESOURCES_RCC ${netsteria_RESOURCES})

add_executable(Netsteria ${GUI_TYPE} ${netsteria_SOURCES} ${netsteria_HEADERS_MOC} ${netsteria_RESOURCES_RCC})
target_link_libraries(Netsteria sst routing netsteria opus rtaudio ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

if (WIN32)
	target_link_libraries(Netsteria ws2_32 gdi32 winmm)
#	RC_FILE = gui.rc
endif (WIN32)

if (APPLE)
	find_library(COREFOUNDATION_LIB CoreFoundation)
	find_library(COREAUDIO_LIB CoreAudio)
	find_library(AUDIOTOOLBOX_LIB AudioToolbox)
        find_library(COCOA_LIB Cocoa)
        target_link_libraries(Netsteria ${COREFOUNDATION_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${COCOA_LIB})
#	ICON = img/netsteria.icns
endif (APPLE)
