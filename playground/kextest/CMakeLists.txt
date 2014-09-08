#add_executable(cryptest cryptest.cpp)
#target_link_libraries(cryptest sodiumpp arsenal)

add_definitions("-Wno-deprecated-register") # Damnit, boost!

add_executable(kextest kextest.cpp)
target_link_libraries(kextest sodiumpp arsenal ${Boost_LIBRARIES})
