set(QT_DONT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../lib)

SET(regserver_HEADERS main.h)
QT4_WRAP_CPP(regserver_HEADERS_MOC ${regserver_HEADERS})

add_executable(regserver main.cc ${regserver_HEADERS_MOC})
target_link_libraries(regserver sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS regserver
	RUNTIME DESTINATION tools)
