add_executable(nsh cli.h ../proto.h ../asyncfile.h main.cc cli.cc ../proto.cc ../asyncfile.cc)
target_link_libraries(nsh sst)
