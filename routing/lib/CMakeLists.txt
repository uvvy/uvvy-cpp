set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

set(routing_SOURCES reg.cc regcli.cc)

set(routing_HEADERS regcli.h)
qt4_wrap_cpp(routing_HEADERS_MOC ${routing_HEADERS})

add_library(routing ${routing_SOURCES} ${routing_HEADERS_MOC})
