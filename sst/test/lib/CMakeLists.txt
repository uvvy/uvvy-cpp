set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../../lib)

set(sst_test_HEADERS sim.h tcp.h)
qt4_wrap_cpp(sst_test_HEADERS_MOC ${sst_test_HEADERS})

add_library(sst_test STATIC tcp.cc sim.cc ${sst_test_HEADERS_MOC})
