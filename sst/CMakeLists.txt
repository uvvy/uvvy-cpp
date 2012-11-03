# This is transitional for extracting pieces of routing-related functionality from SST
include_directories(../routing/lib)

# SST currently has some code that violates the new strict aliasing rules,
# which I really don't feel like fixing.  (Type punning is very handy!)
set(CMAKE_C_CFLAGS "${CMAKE_C_CFLAGS} -fno-strict-aliasing")
set(CMAKE_CXX_CFLAGS "${CMAKE_CXX_CFLAGS} -fno-strict-aliasing")

add_subdirectory(rpcgen)
add_subdirectory(lib)

if (BUILD_TESTS)
	add_subdirectory(test)
endif (BUILD_TESTS)

if (UNIX AND BUILD_TESTS)
    add_subdirectory(shell)
endif (UNIX AND BUILD_TESTS)
