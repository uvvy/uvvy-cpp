set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11 ../../sst/lib ../../routing/lib)

set(netsteria_SOURCES store.cc share.cc index.cc arch.cc chunk.cc file.cc filesync.cpp opaque.cc
		              action.cc scan.cc update.cc crypt.cc adler32.cc rcsum.cc peer.cc audio.cc voice.cc)
set(netsteria_HEADERS action.h audio.h chunk.h filesync.h opaque.h peer.h scan.h share.h store.h voice.h update.h)

qt4_wrap_cpp(netsteria_HEADERS_MOC ${netsteria_HEADERS})

add_library(netsteria STATIC ${netsteria_SOURCES} ${netsteria_HEADERS_MOC})
