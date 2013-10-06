add_subdirectory(sstreams)

set(QT_DONT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

include_directories(../nat ../ssu/include ../support/include ../ui/3rdparty/opus-1.0.1/include ../ui/3rdparty/rtaudio-4.0.11)

set(udpsender_SOURCES udpsend.cpp main.cpp)
set(udpsender_HEADERS udpsend.h)
qt4_wrap_cpp(udpsender_HEADERS_MOC ${udpsender_HEADERS})

add_executable(udpsender ${udpsender_SOURCES} ${udpsender_HEADERS_MOC})
target_link_libraries(udpsender upnp ${QT_LIBRARIES})
install(TARGETS udpsender
	RUNTIME DESTINATION tools)

add_executable(opus-streaming opus-streaming.cpp)
target_link_libraries(opus-streaming ssu support rtaudio opus ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS opus-streaming
    RUNTIME DESTINATION tools)

if (APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
	find_library(COREFOUNDATION_LIB CoreFoundation)
    target_link_libraries(opus-streaming ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
endif (APPLE)
