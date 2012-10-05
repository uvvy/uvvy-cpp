# This qmake include file defines the configuration variables
# that need to be filled in by UIA's top-level configure script.
# We isolate these in an include file so we don't have to
# re-run configure every time a qmake profile changes.

INCLUDEPATH +=  /Users/berkus/Hobby/uia/sst/lib
LIBS += -L/Users/berkus/Hobby/uia/sst -lsst -lcrypto 
SST = /Users/berkus/Hobby/uia/sst
TOPSUBS = 


# We currently have some code that violates the new strict aliasing rules,
# which I really don't feel like fixing.  (Type punning is very handy!)
QMAKE_CFLAGS += -fno-strict-aliasing
QMAKE_CXXFLAGS += -fno-strict-aliasing


