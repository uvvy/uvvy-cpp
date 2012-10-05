#ifndef SHELL_PROTO_H
#define SHELL_PROTO_H

#include <termios.h>

#include <QObject>
#include <QFlags>
#include <QQueue>

namespace SST {
	class Stream;
	class XdrStream;
};

class ShellProtocol
{
public:
	// Standard service and protocol names for this protocol.
	static const QString serviceName;
	static const QString protocolName;


	enum Command {
		Invalid = 0,
		Terminal,	// Request pseudo-terminal
		Shell,		// Start a shell process
		Exec,		// Execute a specific command
		ExitStatus,	// Indicate remote process's exit status
		ExitSignal,	// Indicate remote process killed by signal
	};

	static const char ControlMarker = 1;	// ASCII 'SOH'


	// Terminal input flags
	enum InputFlag {
		tBRKINT		= 0x0001,
		tICRNL		= 0x0002,
		tIGNBRK		= 0x0004,
		tIGNCR		= 0x0008,
		tIGNPAR		= 0x0010,
		tINLCR		= 0x0020,
		tINPCK		= 0x0040,
		tISTRIP		= 0x0080,
		tIXANY		= 0x0100,
		tIXOFF		= 0x0200,
		tIXON		= 0x0400,
		tPARMRK		= 0x0800,
		tIUCLC		= 0x1000,
	};
	Q_DECLARE_FLAGS(InputFlags, InputFlag)

	// Terminal output flags
	enum OutputFlag {
		tOPOST		= 0x0001,
		tOLCUC		= 0x0002,
		tONLCR		= 0x0004,
		tOCRNL		= 0x0008,
		tONOCR		= 0x0010,
		tONLRET		= 0x0020,
	};
	Q_DECLARE_FLAGS(OutputFlags, OutputFlag)

	// Terminal control flags
	enum ControlFlag {
		tCS8		= 0x0001,
		tCSTOPB		= 0x0002,
		tPARENB		= 0x0004,
		tPARODD		= 0x0008,
		tHUPCL		= 0x0010,
		tCLOCAL		= 0x0020,
	};
	Q_DECLARE_FLAGS(ControlFlags, ControlFlag)

	// Terminal local flags
	enum LocalFlag {
		tECHO		= 0x0001,
		tECHOE		= 0x0002,
		tECHOK		= 0x0004,
		tECHONL		= 0x0008,
		tICANON		= 0x0010,
		tIEXTEN		= 0x0020,
		tISIG		= 0x0040,
		tNOFLSH		= 0x0080,
		tTOSTOP		= 0x0100,
	};
	Q_DECLARE_FLAGS(LocalFlags, LocalFlag)

	// Special character indexes
	static const int tVEOF		= 0;
	static const int tVEOL		= 1;
	static const int tVERASE	= 2;
	static const int tVINTR		= 3;
	static const int tVKILL		= 4;
	static const int tVQUIT		= 5;
	static const int tVSTART	= 6;
	static const int tVSTOP		= 7;
	static const int tVSUSP		= 8;
	static const int tNCCS		= 9;

	// Size of input/output buffers for shell I/O forwarding
	static const int shellBufferSize = 16384;


	static void termpack(SST::XdrStream &xs, const struct termios &tios);
	static void termunpack(SST::XdrStream &xs, struct termios &tios);


private:
	static int termpackspeed(speed_t speed);
	static speed_t termunpackspeed(quint32 speed);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(ShellProtocol::InputFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(ShellProtocol::OutputFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(ShellProtocol::ControlFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(ShellProtocol::LocalFlags);


// Common base class for managing both client- and server-side shell streams.
// Handles encoding and decoding control messages embedded within the stream.
class ShellStream : public QObject, public ShellProtocol
{
	Q_OBJECT

public:
	enum PacketType { Null, Data, Control };
	struct Packet {
		PacketType type;
		QByteArray data;

		inline Packet(PacketType type = Null,
				QByteArray data = QByteArray())
			: type(type), data(data) { }
		inline bool isNull() { return type == Null; }
	};

private:
	static const int maxControlMessage = 1<<24;

	SST::Stream *strm;

	// Receive state:
	//	0: normal character transmission
	//	1: received SOH, expecting control message length byte(s)
	//	2: received control message length, expecting message byte(s)
	enum {
		RecvNormal,	// normal character transmission
		RecvLength,	// got SOH, expecting control message length
		RecvMessage,	// Got SOH and length, expecting message data
	} rstate;

	QByteArray rbuf;	// Raw stream data receive buffer
	char *rdat;
	int ramt;

	QByteArray cbuf;	// Control message buffer
	int clen, cgot;

	QQueue<Packet> rq;	// packet receive queue

public:
	ShellStream(SST::Stream *strm = NULL, QObject *parent = NULL);

	inline SST::Stream *stream() { return strm; }
	void setStream(SST::Stream *strm);

	Packet receive();
	bool atEnd() const;

	void sendData(const char *data, int size);
	inline void sendData(const QByteArray &buf)
		{ sendData(buf.data(), buf.size()); }

	void sendControl(const QByteArray &msg);
	void sendEndOfFile();

signals:
	void readyRead();
	void bytesWritten(qint64 bytes);

	// Emitted when a protocol error is detected.
	void error(const QString &msg);
};

#endif	// SHELL_PROTO_H
