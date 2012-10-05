
# First include variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = app
TARGET = rpcgen
DESTDIR = .
DEPENDPATH += .
INCLUDEPATH += .
QT = core
CONFIG -= app_bundle

# Input
HEADERS +=	parse.h scan.h util.h

SOURCES +=	main.c parse.c scan.c util.c \
		hout.c cout.c

