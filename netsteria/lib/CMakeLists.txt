set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
include_directories(../3rdparty/speex/include)

set(netsteria_SOURCES store.cc share.cc index.cc arch.cc chunk.cc file.cc opaque.cc
		audio.cc voice.cc action.cc scan.cc update.cc crypt.cc adler32.cc rcsum.cc peer.cc)

set(netsteria_HEADERS action.h audio.h chunk.h opaque.h peer.h scan.h share.h store.h voice.h)
qt4_wrap_cpp(netsteria_HEADERS_MOC ${netsteria_HEADERS})

add_library(netsteria ${netsteria_SOURCES} ${netsteria_HEADERS_MOC})
