
TEMPLATE = app
TARGET = webtest
DESTDIR = .
LIBS += -L../.. -lsst_test -lsst
DEPENDPATH += . ../../lib ../lib
INCLUDEPATH += . ../../lib ../lib
QT = core network gui
POST_TARGETDEPS += ../../libsst.* ../../libsst_test.*

# Include variables filled in by the configure script
!include(../../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

# Input sources
HEADERS += main.h cli.h srv.h
SOURCES += main.cc cli.cc srv.cc

