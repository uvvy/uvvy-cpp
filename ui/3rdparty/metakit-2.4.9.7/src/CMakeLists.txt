include_directories(../include .)

set(metakit_SOURCES column.cpp custom.cpp derived.cpp fileio.cpp field.cpp format.cpp handler.cpp persist.cpp remap.cpp store.cpp string.cpp table.cpp univ.cpp view.cpp viewx.cpp)

# Enable to build with system STL
if (USE_SYSTEM_STL)
	add_definitions(-Dq4_STD)
	list(APPEND metakit_SOURCES std.cpp)
endif (USE_SYSTEM_STL)

add_library(mk4 STATIC ${metakit_SOURCES})
