set(QT_DONT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML FALSE)
include(${QT_USE_FILE})

include_directories(../../sst/lib ../../routing/lib ../../ui/lib)

set(sstreams_SOURCES main.cpp)
#set(udpsender_HEADERS udpsend.h)
qt4_wrap_cpp(udpsender_HEADERS_MOC ${udpsender_HEADERS})

add_executable(sstreams ${sstreams_SOURCES} ${sstreams_HEADERS_MOC})
target_link_libraries(sstreams sst routing netsteria ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS sstreams
	RUNTIME DESTINATION tools)
