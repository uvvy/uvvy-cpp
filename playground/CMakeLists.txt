set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

set(udpsender_SOURCES udpsend.cpp main.cpp)
set(udpsender_HEADERS udpsend.h)
qt4_wrap_cpp(udpsender_HEADERS_MOC ${udpsender_HEADERS})

add_executable(udpsender ${udpsender_SOURCES} ${udpsender_HEADERS_MOC})
target_link_libraries(udpsender ${QT_LIBRARIES})
