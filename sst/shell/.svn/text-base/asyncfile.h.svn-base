#ifndef SST_ASYNCFILE_H
#define SST_ASYNCFILE_H

#include <QQueue>
#include <QIODevice>

class QSocketNotifier;


/** Generic asynchronous Qt I/O wrapper for using native file descriptors
 * in nonblocking mode on both reads and writes.
 * Provides internal write buffering so that writes always succeed
 * even if the underlying file descriptor isn't ready to accept data.
 * (The caller can use bytesToWrite() to see if output is blocked.)
 */
class AsyncFile : public QIODevice
{
	Q_OBJECT

public:
	enum Status { Ok, Error };

private:
	int fd;
	QSocketNotifier *snin, *snout;
	QQueue<QByteArray> outq;
	qint64 outqd;
	Status st;
	bool endread;

public:
	AsyncFile(QObject *parent = NULL);
	//AsyncFile(int fd, QObject *parent);
	~AsyncFile();

	virtual bool open(int fd, OpenMode mode);
	virtual void close();
	void closeRead();

	inline int fileDescriptor() { return fd; }

	/// Always returns true in class AsyncFile.
	virtual bool isSequential() const;

	/// Returns true if we have reached end-of-file in the input direction.
	virtual bool atEnd() const;

	/// Returns the number of bytes currently queued to write
	/// but not yet written to the underlying file descriptor.
	virtual qint64 bytesToWrite() const;

	inline Status status() { return st; }

protected:
	virtual bool open(OpenMode mode);
	virtual qint64 readData(char *data, qint64 maxSize);
	virtual qint64 writeData(const char *data, qint64 maxSize);

private:
	void setError(const QString &msg);

private slots:
	void readyWrite();
};


#endif	// SST_ASYNCFILE_H
