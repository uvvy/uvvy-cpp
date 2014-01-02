set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib ../lib)

set(sstregress_HEADERS srv.h cli.h migrate.h seg.h)
qt4_wrap_cpp(sstregress_HEADERS_MOC ${sstregress_HEADERS})

add_executable(sstregress  main.cc srv.cc cli.cc migrate.cc seg.cc ${sstregress_HEADERS_MOC})
target_link_libraries(sstregress sst_test sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS sstregress
	RUNTIME DESTINATION tests)

add_test(sstregress_basic sstregress basic)
add_test(sstregress_migrate sstregress migrate)
