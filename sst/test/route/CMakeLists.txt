# @todo Move this to routing

set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib ../lib)

set(route_HEADERS route.h view.h)
qt4_wrap_cpp(route_HEADERS_MOC ${route_HEADERS})

add_executable(route main.cc route.cc stats.cc view.cc ${route_HEADERS_MOC})
target_link_libraries(route sst_test sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS route
	RUNTIME DESTINATION tests)

