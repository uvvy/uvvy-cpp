set(QT_USE_QTNETWORK TRUE)
set(QT_USE_QTXML TRUE)
include(${QT_USE_FILE})

#set(upnp_SOURCES router.cpp multicastsocket.cpp)
#set(upnp_HEADERS router.h multicastsocket.h)

set(upnp_SOURCES httprequest.cpp soap.cpp upnpdescriptionparser.cpp upnpmcastsocket.cpp upnprouter.cpp)
set(upnp_HEADERS httprequest.h upnpmcastsocket.h upnprouter.h)
qt4_wrap_cpp(upnp_HEADERS_MOC ${upnp_HEADERS})

add_library(upnp STATIC ${upnp_SOURCES} ${upnp_HEADERS_MOC})
