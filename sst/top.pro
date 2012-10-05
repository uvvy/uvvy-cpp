
TEMPLATE = subdirs
SUBDIRS = rpcgen lib reg test

unix {
	SUBDIRS += shell
}

