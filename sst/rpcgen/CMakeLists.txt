add_executable(rpcgen main.c parse.c scan.c util.c hout.c cout.c)
install(TARGETS rpcgen
	RUNTIME DESTINATION tools)
