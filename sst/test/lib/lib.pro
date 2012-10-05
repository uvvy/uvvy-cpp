
# First include variables filled in by the configure script
!include(../../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = lib
TARGET = sst_test
DESTDIR = ../..
DEPENDPATH += . ../../lib
INCLUDEPATH += . ../../lib
QT = core network
CONFIG += staticlib
#QMAKE_CXXFLAGS += -fno-strict-aliasing	# XXX

# Input
HEADERS +=	tcp.h sim.h
SOURCES +=	tcp.cc sim.cc

