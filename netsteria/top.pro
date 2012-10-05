
!include(top.pri) {
        error("top.pri not found - please run the configure script.")
}

TEMPLATE = subdirs
SUBDIRS = $$TOPSUBS portaudio speex/libspeex lib gui
#SUBDIRS = sst portaudio speex/libspeex libsamplerate/src lib cons gui

