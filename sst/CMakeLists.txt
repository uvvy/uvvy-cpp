
# SST currently has some code that violates the new strict aliasing rules,
# which I really don't feel like fixing.  (Type punning is very handy!)
set(CMAKE_C_CFLAGS "${CMAKE_C_CFLAGS} -fno-strict-aliasing")
set(CMAKE_CXX_CFLAGS "${CMAKE_CXX_CFLAGS} -fno-strict-aliasing")

add_subdirectory(rpcgen)
add_subdirectory(lib)
add_subdirectory(reg)
add_subdirectory(test)

if (UNIX)
    add_subdirectory(shell)
endif (UNIX)

#add_library(sst ${SST_SOURCES})

