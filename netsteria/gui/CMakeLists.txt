set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../lib ../../sst/lib ../../routing/lib ../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11)
include_directories(../3rdparty/metakit-2.4.9.7/include)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(netsteria_SOURCES main.cc search.cc chat.cc chathistory.cpp view.cc save.cc settings.cc logarea.cc meter.cc)
set(netsteria_HEADERS chat.h chathistory.h logarea.h main.h meter.h save.h search.h settings.h)
set(netsteria_RESOURCES gui.qrc)

if (APPLE)
    list(APPEND netsteria_SOURCES macsupport.mm)
    list(APPEND netsteria_HEADERS macsupport.h)
endif (APPLE)

qt4_wrap_cpp(netsteria_HEADERS_MOC ${netsteria_HEADERS})
qt4_add_resources(netsteria_RESOURCES_RCC ${netsteria_RESOURCES})

add_executable(Netsteria ${GUI_TYPE} ${netsteria_SOURCES} ${netsteria_HEADERS_MOC} ${netsteria_RESOURCES_RCC})
target_link_libraries(Netsteria sst routing netsteria opus rtaudio mk4 ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

if (WIN32)
    target_link_libraries(Netsteria ws2_32 gdi32 winmm)
#   RC_FILE = gui.rc
endif (WIN32)

if (APPLE)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(COCOA_LIB Cocoa)
    target_link_libraries(Netsteria ${COREFOUNDATION_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${COCOA_LIB})
#   ICON = img/netsteria.icns
endif (APPLE)

if (UNIX AND NOT APPLE)
	target_link_libraries(Netsteria asound rt)
endif (UNIX AND NOT APPLE)
