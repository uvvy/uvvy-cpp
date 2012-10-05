
#include <QtDebug>

#include "host.h"
#include "abs.h"

using namespace SST;


////////// AbstractStream //////////

AbstractStream::AbstractStream(Host *h)
:	QObject(static_cast<StreamHostState*>(h)),
	h(h),
	strm(NULL),
	pri(0)
{
}

QByteArray AbstractStream::localHostId()
{
	return h->hostIdent().id();
}

QByteArray AbstractStream::remoteHostId()
{
	return peerid;
}

void AbstractStream::setPriority(int newpri)
{
	pri = newpri;
}

