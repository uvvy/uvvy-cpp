# How to run rpcgen to build XDR stubs
macro(WRAP_RPC_SOURCES name sources)

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
GET_TARGET_PROPERTY(RPCGEN_EXE rpcgen LOCATION) # should not be needed past cmake 2.4 if this file knows about rpcgen target...
foreach (src ${sources})
	string(REGEX REPLACE ".x$" "" tgt ${src})
	ADD_CUSTOM_COMMAND(OUTPUT
	    ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp
	    COMMAND ${RPCGEN_EXE} -h -o ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h    ${CMAKE_CURRENT_SOURCE_DIR}/${src}
	    COMMAND ${RPCGEN_EXE} -c -o ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/${src}
	    DEPENDS ${RPCGEN_EXE}
	    MAIN_DEPENDENCY ${src})
	list(APPEND ${name} ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h)
endforeach()
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

endmacro(WRAP_RPC_SOURCES)


set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

set(SST_SOURCES strm/abs.h strm/base.h strm/dgram.h strm/peer.h strm/sflow.h strm/proto.h
              sock.h key.h dh.h ident.h flow.h seg.h stream.h reg.h regcli.h
			  sign.h dsa.h rsa.h aes.h sha2.h hmac.h chk32.h xdr.h util.h
			  timer.h host.h os.h
			  sock.cc key.cc dh.cc ident.cc flow.cc seg.cc stream.cc strm/abs.cc strm/base.cc strm/dgram.cc
			  strm/peer.cc strm/sflow.cc strm/proto.cc reg.cc regcli.cc sign.cc dsa.cc rsa.cc aes.cc
			  sha2.cc hmac.cc chk32.cc xdr.cc util.cc timer.cc host.cc)

set(RPC_SOURCES keyproto.x)
wrap_rpc_sources(SST_SOURCES ${RPC_SOURCES})

if (WIN32)
	set(SOURCES "$SOURCES os-win.cc")
endif (WIN32)

if (UNIX)
	set(SOURCES "$SOURCES os-unix.cc")
endif (UNIX)


message(STATUS "Sources ${SST_SOURCES}")


add_library(sst ${SST_SOURCES})
