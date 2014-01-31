add_subdirectory(3rdparty/opus-1.0.1)
add_subdirectory(3rdparty/rtaudio-4.0.11)
add_subdirectory(3rdparty/metakit-2.4.9.7)

include_directories(. ../nat/include)
add_library(natclient STATIC traverse_nat.cpp)
add_library(rclient STATIC client_utils.cpp)

#add_subdirectory(lib)
add_subdirectory(qt5)

