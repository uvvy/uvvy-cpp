# Variables filled in by the configure script
!include(../top.pri) {
        error("top.pri not found - please run configure at top level.")
}

TEMPLATE = app
TARGET = Netsteria
DESTDIR = ..
DEPENDPATH += . ../lib $$SST/lib
INCLUDEPATH += . ../lib ../portaudio/pa_common ../speex/include
QT = core gui network
LIBS += -L.. -lnetsteria -lspeex -lportaudio -lcrypto
POST_TARGETDEPS += $$SST/libsst.a ../libnetsteria.a


# Input
HEADERS += main.h search.h chat.h view.h save.h settings.h \
	logarea.h meter.h
SOURCES += main.cc search.cc chat.cc view.cc save.cc settings.cc \
	logarea.cc meter.cc
RESOURCES = gui.qrc

win32 {
	LIBS += -lws2_32 -lgdi32 -lwinmm
	RC_FILE = gui.rc
}
macx {
	LIBS += -framework CoreAudio -framework AudioToolbox
	ICON = img/netsteria.icns
}

