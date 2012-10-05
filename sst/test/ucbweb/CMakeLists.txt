set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib ../lib)

set(ucbweb_HEADERS cli.h srv.h)
qt4_wrap_cpp(ucbweb_HEADERS_MOC ${ucbweb_HEADERS})

if (WIN32)
	set(ucbweb_EXTRA_LIBS ws2_32 gdi32)
endif (WIN32)

add_executable(ucbwebtest main.cc cli.cc srv.cc ${ucbweb_HEADERS_MOC})
target_link_libraries(ucbwebtest sst sst_test ${QT_LIBRARIES} ${OPENSSL_LIBRARIES} ${ucbweb_EXTRA_LIBS})
