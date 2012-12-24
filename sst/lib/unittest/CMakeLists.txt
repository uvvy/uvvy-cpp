set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_base32 test_base32.cpp)
target_link_libraries(test_base32 sst ${Boost_LIBRARIES} ${QT_LIBRARIES})
install(TARGETS test_base32
    RUNTIME DESTINATION tests/unittests)
