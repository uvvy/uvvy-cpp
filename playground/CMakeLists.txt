set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

include_directories(../nat/include ../ssu/include ../support/include)
include_directories(../ui/3rdparty/opus-1.0.1/include) # For unlogger

#set(udpsender_SOURCES udpsend.cpp main.cpp)
#set(udpsender_HEADERS udpsend.h)
#qt4_wrap_cpp(udpsender_HEADERS_MOC ${udpsender_HEADERS})

#add_executable(udpsender ${udpsender_SOURCES} ${udpsender_HEADERS_MOC})
#target_link_libraries(udpsender upnp ${QT_LIBRARIES})
#install(TARGETS udpsender
#	RUNTIME DESTINATION tools)

include_directories(.)
add_library(natclient STATIC traverse_nat.cpp)
add_library(rclient STATIC client_utils.cpp)

add_executable(opus-streaming opus-streaming.cpp)
target_link_libraries(opus-streaming ssu support ${VOICEBOX_LIBS} opus plist rclient routing
    natclient nat upnpc krypto ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS opus-streaming
    RUNTIME DESTINATION tools)

add_executable(unlogger unlogger.cpp)
target_link_libraries(unlogger support opus ${Boost_LIBRARIES})
install(TARGETS unlogger
    RUNTIME DESTINATION tools)

if (APPLE)
    add_executable(screencaster screencaster.cpp macosx.mm)
    target_link_libraries(screencaster ${QT_LIBRARIES} objc)
    install(TARGETS screencaster
        RUNTIME DESTINATION tools)
endif (APPLE)

#add_subdirectory(sstreams)
add_subdirectory(PeerPicker)
