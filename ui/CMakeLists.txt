include_directories(. ../nat/include)
add_library(natclient STATIC traverse_nat.cpp)
add_library(rclient STATIC client_utils.cpp)

#add_subdirectory(lib)
add_subdirectory(qt5)

