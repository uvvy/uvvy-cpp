include_directories(../include .)

# Enable to build with system STL
#add_definitions(-Dq4_STD)

set(metakit_SOURCES column.cpp custom.cpp derived.cpp fileio.cpp field.cpp format.cpp handler.cpp persist.cpp remap.cpp std.cpp store.cpp string.cpp table.cpp univ.cpp view.cpp viewx.cpp)

add_library(mk4 STATIC ${metakit_SOURCES})
