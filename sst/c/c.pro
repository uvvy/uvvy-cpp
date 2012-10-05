
# First include variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = lib
TARGET = sst_c
DESTDIR = ..
DEPENDPATH += .
INCLUDEPATH += . ../inc
QT = 
CONFIG += staticlib
QMAKE_CXXFLAGS += -fno-strict-aliasing	# XXX

# Input
HEADERS +=	../inc/sst/id.h ../inc/sst/base64.h ../inc/sst/decls.h
SOURCES +=	id.cc base64.c


win32 {
	#SOURCES += os-win.cc
}
unix {
	#SOURCES += os-unix.cc
}


# Installation
libsst_c.path = $$LIBDIR
libsst_c.commands = install -c ../libsst_c.a $$INSTALL_ROOT$$LIBDIR/
INSTALLS += libsst_c

