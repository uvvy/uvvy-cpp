
# First include variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = lib
TARGET = sst
DESTDIR = ..
DEPENDPATH += .
INCLUDEPATH += .
QT = core network
CONFIG += $$LIBTYPE
QMAKE_CXXFLAGS += -fno-strict-aliasing	# XXX

# Input
STRM_HEADERS = strm/abs.h strm/base.h \
    strm/dgram.h strm/peer.h strm/sflow.h strm/proto.h
HEADERS +=	sock.h key.h dh.h ident.h flow.h seg.h \
		stream.h reg.h regcli.h \
		sign.h dsa.h rsa.h aes.h sha2.h hmac.h chk32.h \
		xdr.h util.h timer.h host.h \
		os.h

HEADERS += $$STRM_HEADERS

SOURCES +=	sock.cc key.cc dh.cc ident.cc flow.cc seg.cc \
		stream.cc strm/abs.cc strm/base.cc strm/dgram.cc \
		strm/peer.cc strm/sflow.cc strm/proto.cc \
		reg.cc regcli.cc \
		sign.cc dsa.cc rsa.cc aes.cc sha2.cc hmac.cc chk32.cc\
		xdr.cc util.cc timer.cc host.cc

XFILES = keyproto.x


# How to run rpcgen to build XDR stubs
rpcgen_h.output = ${QMAKE_FILE_BASE}.h
rpcgen_h.input = XFILES
rpcgen_h.commands = ../rpcgen/rpcgen -h ${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
rpcgen_h.name = RPCGEN
rpcgen_h.variable_out = GENERATED_HEADERS

rpcgen_cc.output = ${QMAKE_FILE_BASE}_xdr.cc
rpcgen_cc.input = XFILES
rpcgen_cc.depends = ${QMAKE_FILE_BASE}.h
rpcgen_cc.commands = ../rpcgen/rpcgen -c ${QMAKE_FILE_IN} >${QMAKE_FILE_OUT}
rpcgen_cc.name = RPCGEN
rpcgen_cc.variable_out = GENERATED_SOURCES

QMAKE_EXTRA_COMPILERS += rpcgen_h rpcgen_cc


win32 {
	SOURCES += os-win.cc
}
unix {
	SOURCES += os-unix.cc
}

target.path = $$LIBDIR
headers.path = $$INCDIR/sst
headers.files = $$HEADERS
strm_headers.path = $$INCDIR/sst/strm
strm_headers.files = $$STRM_HEADERS

INSTALLS += target headers strm_headers
