
TEMPLATE = app
TARGET = ucbwebtest
DESTDIR = .
LIBS += -L../.. -lsst_test -lsst
DEPENDPATH += . ../../lib ../lib
INCLUDEPATH += . ../../lib ../lib
QT = core network
POST_TARGETDEPS += ../../libsst.* ../../libsst_test.*
CONFIG -= app_bundle

# Include variables filled in by the configure script
!include(../../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

# Input sources
HEADERS += main.h srv.h cli.h
SOURCES += main.cc srv.cc cli.cc

win32 {
	LIBS += -lws2_32 -lgdi32
}

