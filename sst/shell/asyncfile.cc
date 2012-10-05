
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <QSocketNotifier>
#include <QtDebug>

#include "asyncfile.h"


AsyncFile::AsyncFile(QObject *parent)
:	QIODevice(parent),
	fd(-1),
	snin(NULL), snout(NULL),
	outqd(0),
	st(Ok), endread(false)
{
}

AsyncFile::~AsyncFile()
{
	close();
}

bool AsyncFile::open(OpenMode)
{
	qFatal("Do not call AsyncFile::open(OpenMode).");
	return false;
}

bool AsyncFile::open(int fd, OpenMode mode)
{
	//qDebug() << this << "open fd" << fd << "mode" << mode;
	Q_ASSERT(mode & ReadWrite);

	if (this->fd >= 0) {
		setError(tr("AsyncFile already open"));
		return false;
	}

	// Place the file descriptor in nonblocking mode
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	this->fd = fd;
	if (mode & ReadOnly) {
		snin = new QSocketNotifier(fd, QSocketNotifier::Read, this);
		connect(snin, SIGNAL(activated(int)),
			this, SIGNAL(readyRead()));
	}
	if (mode & WriteOnly) {
		snout = new QSocketNotifier(fd, QSocketNotifier::Write, this);
		connect(snout, SIGNAL(activated(int)),
			this, SLOT(readyWrite()));
	}

	// XX could we coexist with QIODevice's buffering?
	//QIODevice::setOpenMode(mode | Unbuffered);
	QIODevice::setOpenMode(mode);
	return true;
}

void AsyncFile::closeRead()
{
	if (snin) {
		delete snin;
		snin = NULL;
	}
	setOpenMode(openMode() & ~ReadOnly);
}

bool AsyncFile::isSequential() const
{
	return true;
}

qint64 AsyncFile::readData(char *data, qint64 maxSize)
{
	Q_ASSERT(fd >= 0);
	Q_ASSERT(openMode() & ReadOnly);

	qint64 act = ::read(fd, data, maxSize);
	if (act < 0) {
		setError(strerror(errno));
		return -1;
	}
	if (act == 0)
		endread = true;
	return act;
}

bool AsyncFile::atEnd() const
{
	return endread;
}

qint64 AsyncFile::writeData(const char *data, qint64 maxSize)
{
	Q_ASSERT(fd >= 0);
	Q_ASSERT(openMode() & WriteOnly);

	// Write the data immediately if possible
	qint64 act = 0;
	if (outq.isEmpty()) {
		act = ::write(fd, data, maxSize);
		if (act < 0) {
			if (errno != EINTR && errno != EAGAIN
					&& errno != EWOULDBLOCK) {
				setError(strerror(errno));
				return -1;	// a real error occurred
			}
			act = 0;	// nothing serious
		}
		data += act;
		maxSize -= act;
	}

	// Buffer whatever we can't write.
	if (maxSize > 0) {
		outq.enqueue(QByteArray(data, maxSize));
		outqd += maxSize;
		act += maxSize;
		snout->setEnabled(true);
	}

	return act;
}

qint64 AsyncFile::bytesToWrite() const
{
	return outqd;
}

void AsyncFile::readyWrite()
{
	while (!outq.isEmpty()) {
		QByteArray &buf = outq.head();
		qint64 act = ::write(fd, buf.data(), buf.size());
		if (act < 0) {
			if (errno != EINTR && errno != EAGAIN
					&& errno != EWOULDBLOCK) {
				// A real error: empty the output buffer
				setError(strerror(errno));
				outq.clear();
				return;
			}
			act = 0;	// nothing serious
		}

		if (act < buf.size()) {
			// Partial write: leave the rest buffered
			buf = buf.mid(act);
			return;
		}

		// Full write: proceed to the next buffer
		outq.removeFirst();
	}

	// Emptied the output queue - don't need write notifications for now.
	snout->setEnabled(false);
}

void AsyncFile::setError(const QString &msg)
{
	st = Error;
	setErrorString(msg);
}

void AsyncFile::close()
{
	QIODevice::close();

	if (fd >= 0) {
		setOpenMode(NotOpen);
		fd = -1;
	}
	if (snin) {
		delete snin;
		snin = NULL;
	}
	if (snout) {
		delete snout;
		snout = NULL;
	}
}

