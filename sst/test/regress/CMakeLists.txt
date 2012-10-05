add_executable(sstregress main.h srv.h cli.h dgram.h migrate.h seg.h main.cc srv.cc cli.cc dgram.cc migrate.cc seg.cc)
target_link_libraries(sstregress sst sst_test)
