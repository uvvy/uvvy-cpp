
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>

#include "srv.h"

using namespace SST;


////////// PidWatcher //////////

PidWatcher::PidWatcher(QObject *parent)
:	QThread(parent),
	pid(-1)
{
}

void PidWatcher::watchPid(int pid)
{
	Q_ASSERT(!isRunning() && !isFinished());

	this->pid = pid;
	start();
}

void PidWatcher::run()
{
	Q_ASSERT(pid > 0);
	waitpid(pid, &stat, 0);
}


////////// ShellSession //////////

ShellSession::ShellSession(Stream *strm, QObject *parent)
:	QObject(parent),
	shs(strm),
	ptyfd(-1), ttyfd(-1)
{
	// Make sure the SST stream terminates when we do
	strm->setParent(this);

	connect(&shs, SIGNAL(readyRead()), this, SLOT(inReady()));
	connect(&aftty, SIGNAL(bytesWritten(qint64)), this, SLOT(inReady()));

	connect(&aftty, SIGNAL(readyRead()), this, SLOT(outReady()));
	connect(&shs, SIGNAL(bytesWritten(qint64)), this, SLOT(outReady()));

	connect(&pidw, SIGNAL(finished()), this, SLOT(childDone()));
}

ShellSession::~ShellSession()
{
	qDebug() << this << "~ShellSession";

	aftty.close();

	if (ptyfd >= 0)
		close(ptyfd);
	if (ttyfd >= 0)
		close(ttyfd);
}

void ShellSession::inReady()
{
	bool ttyopen = aftty.isOpen();
	while (true) {
		if (ttyopen && aftty.bytesToWrite() >= shellBufferSize)
			return;	// wait until the write buffer empties a bit

		ShellStream::Packet pkt = shs.receive();
		switch (pkt.type) {
		case ShellStream::Null:
			if (shs.atEnd()) {
				qDebug() << "End of remote input";
				shutdown(aftty.fileDescriptor(), SHUT_WR);
			}
			return;	// nothing more to receive for now

		case ShellStream::Data:
			//qDebug() << this << "input:" << pkt.data;
			if (!ttyopen) {
				error(tr("Received shell data before command "
					"to start shell"));
				break;
			}
			if (aftty.write(pkt.data) < 0)
				error(aftty.errorString());
			break;

		case ShellStream::Control:
			gotControl(pkt.data);
			break;
		}
	}
}

void ShellSession::outReady()
{
	//qDebug() << this << "outReady: openmode" << aftty.openMode();
	Q_ASSERT(aftty.isOpen());

	while (true) {
		// XX if (shs.bytesToWrite() >= shellBufferSize) return;

		char buf[4096];
		int act = aftty.read(buf, sizeof(buf));
		if (act <= 0) {
			if (act < 0)
				error(aftty.errorString());
			return;
		}
		//qDebug() << this << "output:" << QByteArray(buf, act);
		shs.sendData(buf, act);

		// When our child process(es) have no more output to send,
		// just close our end of the pipe and stop forwarding data
		// until the child process dies and we get its exit status.
		// (XX notify client via control message?)
		if (aftty.atEnd()) {
			qDebug() << this << "end-of-file on child pseudo-tty";
			aftty.close();
		}
	}
}

void ShellSession::gotControl(const QByteArray &msg)
{
	Q_ASSERT(msg.size() > 0);

	XdrStream rxs(msg);
	quint32 cmd;
	rxs >> cmd;
	switch ((Command)cmd) {
	case Terminal:	openPty(rxs);	break;
	case Shell:	runShell(rxs);	break;
	case Exec:	doExec(rxs);	break;
	default:
		qDebug() << this << "ignoring unknown control message type"
			<< cmd;
	}
}

void ShellSession::openPty(XdrStream &rxs)
{
	if (ptyfd >= 0)
		return error(tr("Already have a pseudo-terminal"));
	if (ttyfd >= 0)
		return error(tr("Already have a remote shell I/O stream"));

	// Decode the rest of the Terminal control message
	quint32 width, height, xpixels, ypixels;
	struct termios tios;
	rxs >> termname >> width >> height >> xpixels >> ypixels;
	termunpack(rxs, tios);
	if (rxs.status() != rxs.Ok)
		return error(tr("Invalid Terminal request"));

	qDebug() << "terminal" << termname
		<< "window" << width << "x" << height;

	ptyfd = posix_openpt(O_RDWR | O_NOCTTY);
	if (ptyfd < 0)
		return error(tr("Can't create pseudo-terminal: %0")
				.arg(strerror(errno)));

	// Set the pty's window size
	struct winsize ws;
	ws.ws_col = width;
	ws.ws_row = height;
	ws.ws_xpixel = xpixels;
	ws.ws_ypixel = ypixels;
	if (ioctl(ptyfd, TIOCSWINSZ, &ws) < 0)
		qDebug() << this << "Can't set terminal window size";

	// Set the pty's terminal modes
	if (tcsetattr(ptyfd, TCSANOW, &tios) < 0)
		qDebug() << this << "Can't set terminal parameters";
}

void ShellSession::runShell(XdrStream &rxs)
{
	qDebug() << this << "run shell";
	run();
}

void ShellSession::doExec(XdrStream &rxs)
{
	QString cmd;
	rxs >> cmd;
	if (rxs.status() != rxs.Ok)
		return error(tr("Invalid Exec request"));

	qDebug() << this << "run command" << cmd;

	run(cmd);
}

void ShellSession::run(const QString &/*cmd XXX*/)
{
	if (ttyfd >= 0)
		return error(tr("Already have a remote shell running"));

	// If we don't have a pseudo-terminal,
	// create a socket pair for the shell's (non-terminal) stdio.
	int childfd = -1;
	if (ptyfd < 0) {
		int fds[2];
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
			return error(tr("Can't create socket pair: %0")
					.arg(strerror(errno)));
		ttyfd = fds[0];
		childfd = fds[1];
	}
	Q_ASSERT((ptyfd >= 0) ^ (ttyfd >= 0));	// one or the other, not both

	// Fork off the child process.
	pid_t childpid = fork();
	if (childpid < 0) {
		if (childfd >= 0)
			close(childfd);
		return error(tr("Can't create child process: %0")
				.arg(strerror(errno)));
	}
	if (childpid == 0) {
		// We're the child process.

		// XXX authenticate user, setuid appropriately

		if (ptyfd >= 0) {
			qDebug() << "setup child ptys";

			// Set up the pseudo-tty for the child
			signal(SIGCHLD, SIG_DFL);
			if (grantpt(ptyfd) < 0)
				{ perror("Remote shell: grantpt"); exit(1); }
			if (unlockpt(ptyfd) < 0)
				{ perror("Remote shell: unlockpt"); exit(1); }

			// Establish a new session
			if (setsid() < 0)
				{ perror("Remote shell: setsid"); exit(1); }

			// Open the child end of the terminal
			Q_ASSERT(childfd < 0);
			char *ttyname = ptsname(ptyfd);
			qDebug() << "child ptsname" << ttyname;
			if (ttyname == NULL)
				{ perror("Remote shell: ptsname"); exit(1); }
			childfd = open(ttyname, O_RDWR);
			if (childfd < 0)
				{ perror("Remote shell: open tty"); exit(1); }

			// Set the TERM environment variable appropriately
			setenv("TERM", termname.toLocal8Bit().data(), 1);
		}
		Q_ASSERT(childfd >= 0);

		// Set up our stdio handles
		if (dup2(childfd, STDIN_FILENO) < 0 ||
				dup2(childfd, STDOUT_FILENO) < 0 ||
				dup2(childfd, STDERR_FILENO) < 0)
			{ perror("Remote shell: dup2"); exit(1); }

		// Run the shell/command
		qDebug() << "child: exec";
		execlp("login", "login", NULL);	// XXX
		perror("Remote shell: exec");
		exit(1);
	}

	// We're the parent process.
	// First close the child's file descriptor, if any.
	if (childfd >= 0)
		close(childfd);

	// Set up for I/O forwarding.
	aftty.open(ptyfd >= 0 ? ptyfd : ttyfd, QIODevice::ReadWrite);

	// Watch for the child process's termination.
	pidw.watchPid(childpid);

	qDebug() << this << "started shell";
}

void ShellSession::error(const QString &str)
{
	qDebug() << this << "error:" << str;

	// XXX send error control message

	this->deleteLater();
}

void ShellSession::childDone()
{
	int st = pidw.exitStatus();
	qDebug() << "child terminated with status" << st;

	QByteArray cmsg;
	XdrStream wxs(&cmsg, QIODevice::WriteOnly);
	if (WIFEXITED(st)) {
		// Regular process termination
		wxs << (qint32)ExitStatus << (qint32)WEXITSTATUS(st);

	} else if (WIFSIGNALED(st)) {

		// Process terminated by signal
		QString signame;
		switch (WTERMSIG(st)) {
		case SIGABRT:	signame = "SIGABRT"; break;
		case SIGALRM:	signame = "SIGALRM"; break;
		case SIGBUS:	signame = "SIGBUS"; break;
		case SIGCHLD:	signame = "SIGCHLD"; break;
		case SIGCONT:	signame = "SIGCONT"; break;
		case SIGFPE:	signame = "SIGFPE"; break;
		case SIGHUP:	signame = "SIGHUP"; break;
		case SIGILL:	signame = "SIGILL"; break;
		case SIGINT:	signame = "SIGINT"; break;
		case SIGKILL:	signame = "SIGKILL"; break;
		case SIGPIPE:	signame = "SIGPIPE"; break;
		case SIGQUIT:	signame = "SIGQUIT"; break;
		case SIGSEGV:	signame = "SIGSEGV"; break;
		case SIGSTOP:	signame = "SIGSTOP"; break;
		case SIGTERM:	signame = "SIGTERM"; break;
		case SIGTSTP:	signame = "SIGTSTP"; break;
		case SIGTTIN:	signame = "SIGTTIN"; break;
		case SIGTTOU:	signame = "SIGTTOU"; break;
		case SIGUSR1:	signame = "SIGUSR1"; break;
		case SIGUSR2:	signame = "SIGUSR2"; break;
#ifdef SIGPOLL
		case SIGPOLL:	signame = "SIGPOLL"; break;
#endif
		case SIGPROF:	signame = "SIGPROF"; break;
		case SIGSYS:	signame = "SIGSYS"; break;
		case SIGTRAP:	signame = "SIGTRAP"; break;
		case SIGURG:	signame = "SIGURG"; break;
		case SIGVTALRM:	signame = "SIGVTALRM"; break;
		case SIGXCPU:	signame = "SIGXCPU"; break;
		case SIGXFSZ:	signame = "SIGXFSZ"; break;
		default:
				signame = QString::number(WTERMSIG(st));
				// XX append '@machine-type', like ssh?
				break;
		}
		qint32 flags = 0;
		if (WCOREDUMP(st))
			flags |= 1;
		QString errmsg;
		QString langtag;		// XXX RFC3066 lang tag?
		wxs << (qint32)ExitSignal << flags << signame
			<< errmsg << langtag;
	}
	if (!cmsg.isEmpty())
		shs.sendControl(cmsg);

	// Terminate this shell session
	this->deleteLater();
}


////////// ShellServer //////////

ShellServer::ShellServer(Host *host, QObject *parent)
:	QObject(parent),
	srv(host)
{
	connect(&srv, SIGNAL(newConnection()),
		this, SLOT(gotConnection()));

	if (!srv.listen(serviceName, tr("Secure Remote Shell"),
			protocolName, tr("Netsteria Remote Shell Protocol")))
		qFatal("Can't register Shell service with SST");
}

void ShellServer::gotConnection()
{
	// Accept substreams representing shell connections.
	while (true) {
		Stream *strm = srv.accept();
		if (!strm)
			return;

		(void)new ShellSession(strm, this);
	}
}

