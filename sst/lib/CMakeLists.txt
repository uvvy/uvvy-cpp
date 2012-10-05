set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(SST_SOURCES strm/abs.h strm/base.h strm/dgram.h strm/peer.h strm/sflow.h strm/proto.h
              sock.h key.h dh.h ident.h flow.h seg.h stream.h reg.h regcli.h
			  sign.h dsa.h rsa.h aes.h sha2.h hmac.h chk32.h xdr.h util.h
			  timer.h host.h os.h
			  sock.cc key.cc dh.cc ident.cc flow.cc seg.cc stream.cc strm/abs.cc strm/base.cc strm/dgram.cc
			  strm/peer.cc strm/sflow.cc strm/proto.cc reg.cc regcli.cc sign.cc dsa.cc rsa.cc aes.cc
			  sha2.cc hmac.cc chk32.cc xdr.cc util.cc timer.cc host.cc)

if (WIN32)
	set(SOURCES "$SOURCES os-win.cc")
endif (WIN32)

if (UNIX)
	set(SOURCES "$SOURCES os-unix.cc")
endif (UNIX)

add_library(sst ${SST_SOURCES})
