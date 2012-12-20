set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib ..)
add_definitions(-DETCDIR="/usr/local/etc")

set(nshd_HEADERS srv.h ../proto.h ../asyncfile.h)
qt4_wrap_cpp(nshd_HEADERS_MOC ${nshd_HEADERS})

add_executable(nshd main.cc srv.cc ../proto.cc ../asyncfile.cc ${nshd_HEADERS_MOC})
target_link_libraries(nshd sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS nshd
	RUNTIME DESTINATION tools)
