add_executable(nshd srv.h ../proto.h ../asyncfile.h main.cc srv.cc ../proto.cc ../asyncfile.cc)
target_link_libraries(nshd sst)
