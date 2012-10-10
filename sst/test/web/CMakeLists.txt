set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib ../lib)

set(web_HEADERS cli.h srv.h)
qt4_wrap_cpp(web_HEADERS_MOC ${web_HEADERS})

add_executable(webtest main.cc cli.cc srv.cc ${web_HEADERS_MOC})
target_link_libraries(webtest sst routing sst_test ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
