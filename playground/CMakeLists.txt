set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

include_directories(../nat/include ../ssu/include ../support/include ../ui/3rdparty/opus-1.0.1/include)
include_directories(../ui/3rdparty/rtaudio-4.0.11)

#set(udpsender_SOURCES udpsend.cpp main.cpp)
#set(udpsender_HEADERS udpsend.h)
#qt4_wrap_cpp(udpsender_HEADERS_MOC ${udpsender_HEADERS})

#add_executable(udpsender ${udpsender_SOURCES} ${udpsender_HEADERS_MOC})
#target_link_libraries(udpsender upnp ${QT_LIBRARIES})
#install(TARGETS udpsender
#	RUNTIME DESTINATION tools)

add_executable(opus-streaming opus-streaming.cpp)
target_link_libraries(opus-streaming ssu support rtaudio opus plist routing nat upnpc krypto
    ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS opus-streaming
    RUNTIME DESTINATION tools)

add_executable(unlogger unlogger.cpp)
target_link_libraries(unlogger support opus ${Boost_LIBRARIES})
install(TARGETS unlogger
    RUNTIME DESTINATION tools)

add_executable(screencaster screencaster.cpp macosx.mm)
target_link_libraries(screencaster ${QT_LIBRARIES} objc)
install(TARGETS screencaster
    RUNTIME DESTINATION tools)

if (APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    target_link_libraries(opus-streaming ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
endif (APPLE)

if (UNIX AND NOT APPLE)
    find_library(PULSE_LIB pulse)
    find_library(PULSESIMPLE_LIB pulse-simple)
    target_link_libraries(opus-streaming ${PULSE_LIB} ${PULSESIMPLE_LIB})
endif (UNIX AND NOT APPLE)

#add_subdirectory(sstreams)
add_subdirectory(PeerPicker)
