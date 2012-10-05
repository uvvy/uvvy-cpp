#ifndef SHELL_H
#define SHELL_H

#include <QThread>

#include "stream.h"
#include "../proto.h"
#include "../asyncfile.h"


class PidWatcher : public QThread
{
	Q_OBJECT

private:
	int pid;
	int stat;

public:
	PidWatcher(QObject *parent = NULL);

	/// Start the PidWatcher thread and make it wait on process 'pid'.
	/// The thread terminates when the designated child process does,
	/// emitting the QThread::finished() signal.
	void watchPid(int pid);

	/// After the PidWatcher thread has terminated and emitted finished(),
	/// the child process's exit status can be obtained from this method.
	inline int exitStatus() { return stat; }

private:
	void run();
};

class ShellSession : public QObject, ShellProtocol
{
	Q_OBJECT

private:
	ShellStream shs;
	int ptyfd, ttyfd;
	AsyncFile aftty;
	QString termname;	// Name for TERM environment variable 
	PidWatcher pidw;	// To watch for the child process's death

public:
	ShellSession(SST::Stream *strm, QObject *parent = NULL);
	~ShellSession();

private:
	void gotControl(const QByteArray &msg);

	void openPty(SST::XdrStream &rxs);
	void runShell(SST::XdrStream &rxs);
	void doExec(SST::XdrStream &rxs);

	void run(const QString &cmd = QString());

	// Send an error message and reset the stream
	void error(const QString &str);

private slots:
	void inReady();
	void outReady();
	void childDone();
};

class ShellServer : public QObject, public ShellProtocol
{
	Q_OBJECT

private:
	SST::StreamServer srv;

public:
	ShellServer(SST::Host *host, QObject *parent = NULL);

private slots:
	void gotConnection();
};

#endif	// SHELL_H
