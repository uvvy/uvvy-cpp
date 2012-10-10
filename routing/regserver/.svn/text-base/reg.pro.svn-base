
# First include variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = app
TARGET = sstreg
DESTDIR = ..
DEPENDPATH += . ../lib
INCLUDEPATH += . ../lib
QT = core network
LIBS += -L.. -lsst -lcrypto
POST_TARGETDEPS += ../libsst.*
CONFIG -= app_bundle

# Input
HEADERS += main.h
SOURCES += main.cc

