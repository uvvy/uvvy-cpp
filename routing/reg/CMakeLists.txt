set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../lib)

SET(sstreg_HEADERS main.h)
QT4_WRAP_CPP(sstreg_HEADERS_MOC ${sstreg_HEADERS})

add_executable(sstreg main.cc ${sstreg_HEADERS_MOC})
target_link_libraries(sstreg sst routing ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
