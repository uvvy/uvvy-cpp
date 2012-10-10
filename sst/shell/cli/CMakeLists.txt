set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib)

set(nsh_HEADERS cli.h ../proto.h ../asyncfile.h)
qt4_wrap_cpp(nsh_HEADERS_MOC ${nsh_HEADERS})

add_executable(nsh main.cc cli.cc ../proto.cc ../asyncfile.cc ${nsh_HEADERS_MOC})
target_link_libraries(nsh sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
