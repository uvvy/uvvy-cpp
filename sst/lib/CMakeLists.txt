set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

set(sst_SOURCES sock.cc key.cc dh.cc ident.cc flow.cc seg.cc stream.cc strm/abs.cc strm/base.cc strm/dgram.cc
			  strm/peer.cc strm/sflow.cc strm/proto.cc sign.cc dsa.cc rsa.cc aes.cc
			  sha2.cc hmac.cc chk32.cc xdr.cc util.cc timer.cc host.cc base32.cpp)

set(sst_RPC_SOURCES keyproto.x)
wrap_rpc_sources(sst_SOURCES ${sst_RPC_SOURCES})

set(sst_HEADERS dh.h flow.h key.h seg.h sock.h stream.h timer.h strm/abs.h strm/base.h strm/dgram.h strm/peer.h strm/sflow.h)
qt4_wrap_cpp(sst_HEADERS_MOC ${sst_HEADERS})

if (WIN32)
	list(APPEND sst_SOURCES os-win.cc)
endif (WIN32)

if (UNIX)
	list(APPEND sst_SOURCES os-unix.cc)
endif (UNIX)

# OSX deprecated some openssl compatible declarations in Lion.
if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif (APPLE)

if (BUILD_TESTS)
	add_subdirectory(unittest)
endif (BUILD_TESTS)

add_library(sst ${sst_SOURCES} ${sst_HEADERS_MOC})
