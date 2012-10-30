include_directories(../include .)

# Build with system STL
#add_definitions(-Dq4_STD)

set(metakit_SOURCES column.cpp custom.cpp derived.cpp fileio.cpp field.cpp format.cpp handler.cpp persist.cpp remap.cpp std.cpp store.cpp string.cpp table.cpp univ.cpp view.cpp viewx.cpp)

#g++ -dynamiclib -flat_namespace -undefined suppress -o libmk4.dylib column.o custom.o derived.o fileio.o field.o format.o handler.o persist.o remap.o std.o store.o string.o table.o univ.o view.o viewx.o -lpthread 

add_library(mk4 SHARED ${metakit_SOURCES})
#find_package(Thread)
#target_link_libraries(mk4 pthread)
