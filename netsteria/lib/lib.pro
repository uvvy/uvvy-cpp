
TEMPLATE = lib
TARGET = netsteria
DESTDIR = ..
DEPENDPATH += .
INCLUDEPATH += . ../portaudio/pa_common ../speex/include
QT = core network
#LIBS += -lsst
#CONFIG += create_prl
CONFIG += staticlib

# Variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

# Input sources
HEADERS +=	store.h share.h index.h arch.h chunk.h file.h opaque.h \
		action.h scan.h update.h \
		audio.h voice.h \
		crypt.h adler32.h rcsum.h \
		peer.h

SOURCES +=	store.cc share.cc index.cc arch.cc chunk.cc file.cc opaque.cc \
		audio.cc voice.cc \
		action.cc scan.cc update.cc \
		crypt.cc adler32.cc rcsum.cc \
		peer.cc

