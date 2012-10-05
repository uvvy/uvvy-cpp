
TEMPLATE = app
TARGET = sstregress
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
HEADERS += main.h srv.h cli.h dgram.h migrate.h seg.h
SOURCES += main.cc srv.cc cli.cc dgram.cc migrate.cc seg.cc

