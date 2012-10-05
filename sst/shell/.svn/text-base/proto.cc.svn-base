
#include <termios.h>

#include "stream.h"
#include "proto.h"

using namespace SST;


////////// ShellProtocol //////////

const QString ShellProtocol::serviceName = "Shell";
const QString ShellProtocol::protocolName = "NstShell";

const char ShellProtocol::ControlMarker;

int ShellProtocol::termpackspeed(speed_t speed)
{
	switch (speed) {
	case B0:		return 0;
	case B50:		return 50;
	case B75:		return 75;
	case B110:		return 110;
	case B134:		return 134;
	case B150:		return 150;
	case B200:		return 200;
	case B300:		return 300;
	case B600:		return 600;
	case B1200:		return 1200;
	case B1800:		return 1800;
	case B2400:		return 2400;
	case B4800:		return 4800;
	case B9600:		return 9600;
	case B19200:		return 19200;
	case B38400:		return 38400;
	default:
		qDebug() << "unknown termios speed" << speed;
		return 9600;
	}
}

speed_t ShellProtocol::termunpackspeed(quint32 speed)
{
	switch (speed) {
	case 0:			return B0;
	case 50:		return B50;
	case 75:		return B75;
	case 110:		return B110;
	case 134:		return B134;
	case 150:		return B150;
	case 200:		return B200;
	case 300:		return B300;
	case 600:		return B600;
	case 1200:		return B1200;
	case 1800:		return B1800;
	case 2400:		return B2400;
	case 4800:		return B4800;
	case 9600:		return B9600;
	case 19200:		return B19200;
	case 38400:		return B38400;
	default:
		qDebug() << "unknown termios speed" << speed;
		return B9600;
	}
}

// Encode the terminal mode settings from a termios struct into an XdrStream.
void ShellProtocol::termpack(XdrStream &xs, const struct termios &tios)
{
	// Encode input flags.
	InputFlags iflag = 0;
	if (tios.c_iflag & BRKINT)		iflag |= tBRKINT;
	if (tios.c_iflag & ICRNL)		iflag |= tICRNL;
	if (tios.c_iflag & IGNBRK)		iflag |= tIGNBRK;
	if (tios.c_iflag & IGNCR)		iflag |= tIGNCR;
	if (tios.c_iflag & IGNPAR)		iflag |= tIGNPAR;
	if (tios.c_iflag & INLCR)		iflag |= tINLCR;
	if (tios.c_iflag & INPCK)		iflag |= tINPCK;
	if (tios.c_iflag & ISTRIP)		iflag |= tISTRIP;
	if (tios.c_iflag & IXANY)		iflag |= tIXANY;
	if (tios.c_iflag & IXOFF)		iflag |= tIXOFF;
	if (tios.c_iflag & IXON)		iflag |= tIXON;
	if (tios.c_iflag & PARMRK)		iflag |= tPARMRK;
#ifdef IUCLC
	if (tios.c_iflag & IUCLC)		iflag |= tIUCLC;
#endif

	// Encode output flags.
	OutputFlags oflag = 0;
	if (tios.c_oflag & OPOST)		oflag |= tOPOST;
#ifdef OLCUC
	if (tios.c_oflag & OLCUC)		oflag |= tOLCUC;
#endif
	if (tios.c_oflag & ONLCR)		oflag |= tONLCR;
	if (tios.c_oflag & OCRNL)		oflag |= tOCRNL;
	if (tios.c_oflag & ONOCR)		oflag |= tONOCR;
	if (tios.c_oflag & ONLRET)		oflag |= tONLRET;

	// Encode control flags.
	ControlFlags cflag = 0;
	if ((tios.c_cflag & CSIZE) == CS8)	cflag |= tCS8;
	if (tios.c_cflag & CSTOPB)		cflag |= tCSTOPB;
	if (tios.c_cflag & PARENB)		cflag |= tPARENB;
	if (tios.c_cflag & PARODD)		cflag |= tPARODD;
	if (tios.c_cflag & HUPCL)		cflag |= tHUPCL;
	if (tios.c_cflag & CLOCAL)		cflag |= tCLOCAL;

	// Encode local flags.
	LocalFlags lflag = 0;
	if (tios.c_lflag & ECHO)		lflag |= tECHO;
	if (tios.c_lflag & ECHOE)		lflag |= tECHOE;
	if (tios.c_lflag & ECHOK)		lflag |= tECHOK;
	if (tios.c_lflag & ECHONL)		lflag |= tECHONL;
	if (tios.c_lflag & ICANON)		lflag |= tICANON;
	if (tios.c_lflag & IEXTEN)		lflag |= tIEXTEN;
	if (tios.c_lflag & ISIG)		lflag |= tISIG;
	if (tios.c_lflag & NOFLSH)		lflag |= tNOFLSH;
	if (tios.c_lflag & TOSTOP)		lflag |= tTOSTOP;

	// Input and output speeds
	quint32 ispeed = termpackspeed(cfgetispeed(&tios));
	quint32 ospeed = termpackspeed(cfgetospeed(&tios));

	// Encode special characters
	QByteArray cc(tNCCS, (char)0);
	cc[tVEOF]	= tios.c_cc[VEOF];
	cc[tVEOL]	= tios.c_cc[VEOL];
	cc[tVERASE]	= tios.c_cc[VERASE];
	cc[tVINTR]	= tios.c_cc[VINTR];
	cc[tVKILL]	= tios.c_cc[VKILL];
	cc[tVQUIT]	= tios.c_cc[VQUIT];
	cc[tVSTART]	= tios.c_cc[VSTART];
	cc[tVSTOP]	= tios.c_cc[VSTOP];
	cc[tVSUSP]	= tios.c_cc[VSUSP];

	// Encode the mode structure
	xs << (quint32)iflag << (quint32)oflag
		<< (quint32)cflag << (quint32)lflag
		<< ispeed << ospeed << cc;
}

// Unpack a terminal mode string into a termios structure.
void ShellProtocol::termunpack(XdrStream &xs, struct termios &tios)
{
	// Clear any system-specific parts of the termios struct
	memset(&tios, 0, sizeof(tios));

	// Decode the various fields
	quint32 iflag32, oflag32, cflag32, lflag32, ispeed, ospeed;
	QByteArray cc;
	xs >> iflag32 >> oflag32 >> cflag32 >> lflag32
		>> ispeed >> ospeed >> cc;

	// Decode input flags.
	InputFlags iflag = (InputFlags)iflag32;
	tios.c_iflag = 0;
	if (iflag & tBRKINT)		tios.c_iflag |= BRKINT;
	if (iflag & tICRNL)		tios.c_iflag |= ICRNL;
	if (iflag & tIGNBRK)		tios.c_iflag |= IGNBRK;
	if (iflag & tIGNCR)		tios.c_iflag |= IGNCR;
	if (iflag & tIGNPAR)		tios.c_iflag |= IGNPAR;
	if (iflag & tINLCR)		tios.c_iflag |= INLCR;
	if (iflag & tINPCK)		tios.c_iflag |= INPCK;
	if (iflag & tISTRIP)		tios.c_iflag |= ISTRIP;
	if (iflag & tIXANY)		tios.c_iflag |= IXANY;
	if (iflag & tIXOFF)		tios.c_iflag |= IXOFF;
	if (iflag & tIXON)		tios.c_iflag |= IXON;
	if (iflag & tPARMRK)		tios.c_iflag |= PARMRK;
#ifdef IUCLC
	if (iflag & tIUCLC)		tios.c_iflag |= IUCLC;
#endif

	// Decode output flags.
	OutputFlags oflag = (OutputFlags)oflag32;
	tios.c_oflag = 0;
	if (oflag & tOPOST)		tios.c_oflag |= OPOST;
#ifdef OLCUC
	if (oflag & tOLCUC)		tios.c_oflag |= OLCUC;
#endif
	if (oflag & tONLCR)		tios.c_oflag |= ONLCR;
	if (oflag & tOCRNL)		tios.c_oflag |= OCRNL;
	if (oflag & tONOCR)		tios.c_oflag |= ONOCR;
	if (oflag & tONLRET)		tios.c_oflag |= ONLRET;

	// Decode control flags.
	ControlFlags cflag = (ControlFlags)cflag32;
	tios.c_cflag = 0;
	if (cflag & tCS8)		tios.c_cflag |= CS8;
		else			tios.c_cflag |= CS7;
	if (cflag & tCSTOPB)		tios.c_cflag |= CSTOPB;
	if (cflag & tPARENB)		tios.c_cflag |= PARENB;
	if (cflag & tPARODD)		tios.c_cflag |= PARODD;
	if (cflag & tHUPCL)		tios.c_cflag |= HUPCL;
	if (cflag & tCLOCAL)		tios.c_cflag |= CLOCAL;

	// Decode local flags.
	LocalFlags lflag = (LocalFlags)lflag32;
	tios.c_lflag = 0;
	if (lflag & tECHO)		tios.c_lflag |= ECHO;
	if (lflag & tECHOE)		tios.c_lflag |= ECHOE;
	if (lflag & tECHOK)		tios.c_lflag |= ECHOK;
	if (lflag & tECHONL)		tios.c_lflag |= ECHONL;
	if (lflag & tICANON)		tios.c_lflag |= ICANON;
	if (lflag & tIEXTEN)		tios.c_lflag |= IEXTEN;
	if (lflag & tISIG)		tios.c_lflag |= ISIG;
	if (lflag & tNOFLSH)		tios.c_lflag |= NOFLSH;
	if (lflag & tTOSTOP)		tios.c_lflag |= TOSTOP;

	// Input and output speeds
	cfsetispeed(&tios, termunpackspeed(ispeed));
	cfsetospeed(&tios, termunpackspeed(ospeed));

	// Encode special characters
	while (cc.size() < tNCCS)
		cc.append((char)0);
	tios.c_cc[VEOF]		= cc[tVEOF];
	tios.c_cc[VEOL]		= cc[tVEOL];
	tios.c_cc[VERASE]	= cc[tVERASE];
	tios.c_cc[VINTR]	= cc[tVINTR];
	tios.c_cc[VKILL]	= cc[tVKILL];
	tios.c_cc[VQUIT]	= cc[tVQUIT];
	tios.c_cc[VSTART]	= cc[tVSTART];
	tios.c_cc[VSTOP]	= cc[tVSTOP];
	tios.c_cc[VSUSP]	= cc[tVSUSP];
}


////////// ShellStream //////////

ShellStream::ShellStream(Stream *strm, QObject *parent)
:	QObject(parent),
	strm(NULL),
	rstate(RecvNormal),
	ramt(0)
{
	if (strm)
		setStream(strm);
}

void ShellStream::setStream(Stream *strm)
{
	Q_ASSERT(this->strm == NULL);
	this->strm = strm;

	connect(strm, SIGNAL(readyRead()),
		this, SIGNAL(readyRead()));
	connect(strm, SIGNAL(bytesWritten(qint64)),
		this, SIGNAL(bytesWritten(qint64)));
}

#if 0
void ShellStream::setInputEnabled(bool enable)
{
	Q_ASSERT(strm != NULL);

	if (enable) {
		connect(strm, SIGNAL(readyRead()), this, SLOT(readyRead()));
		readyRead();
	} else
		disconnect(strm, SIGNAL(readyRead()), this, SLOT(readyRead()));
}
#endif

void ShellStream::sendData(const char *data, int size)
{
	// Escape any SOH characters in the stream
	const char *mark = (const char*)memchr(data, ControlMarker, size);
	while (mark != NULL) {
		static const char lenbyte = 0x80;
		int amt = mark+1-data;
		strm->writeData(data, amt);
		strm->writeData(&lenbyte, 1);
		data += amt;
		size -= amt;
	}

	// Remainder contains no control markers
	strm->writeData(data, size);
}

// Insert a control message into the outgoing character data stream.
void ShellStream::sendControl(const QByteArray &msg)
{
	int len = msg.size();
	Q_ASSERT(len > 0);

	// Build and send the control message header
	char hdr[10];
	int j = sizeof(hdr);
	int i = j;
	do {
		hdr[--i] = len & 0x7f;
		len >>= 7;
	} while (len > 0);
	hdr[j-1] |= 0x80;	// High bit indicates last length byte
	hdr[--i] = ControlMarker;
	strm->writeData(hdr+i, j-i);

	// Send the control message body
	strm->writeData(msg.data(), msg.size());
}

// Process a control message received as a substream of this shell stream.
ShellStream::Packet ShellStream::receive()
{
	while (true) {
		// Fill the receive buffer if it's empty
		if (ramt == 0) {
			rbuf = strm->readData();
			rdat = rbuf.data();
			ramt = rbuf.size();
			if (ramt == 0) {
				// nothing to read at the moment
				return Packet();
			}
		}

		// Process the received data
		switch (rstate) {
		case RecvNormal: {
			if (rdat[0] == ControlMarker) {
				qDebug() << "got control marker";
				rstate = RecvLength;
				clen = 0;
				rdat++, ramt--;
				break;
			}

			// Receive normal character data
			char *p = (char*)memchr(rdat, ControlMarker, ramt);
			int act = p ? p - rdat : ramt;
			QByteArray ret = (act == rbuf.size())
						? rbuf
						: QByteArray(rdat, act);
			rdat += act, ramt -= act;
			return Packet(Data, ret); }

		case RecvLength: {
			if (clen >= maxControlMessage) {
				error(tr("Control message too large"));
				strm->shutdown(strm->Reset);
				return Packet(Null);
			}
			char ch = rdat[0];
			rdat++, ramt--;
			clen = (clen << 7) | (ch & 0x7f);
			if (ch & 0x80) {
				if (clen == 0) {
					// Just an escaped control marker.
					return Packet(Data,
						QByteArray(1, ControlMarker));
				}
				//qDebug() << "control msg size" << clen;
				rstate = RecvMessage;
				cbuf.resize(clen);
				cgot = 0;
			}
			break; }

		case RecvMessage: {
			// Receive control message data.
			int act = qMin(ramt, clen-cgot);
			memcpy(cbuf.data()+cgot, rdat, act);
			rdat += act, ramt -= act;
			cgot += act;
			if (cgot == clen) {
				// Got a complete control message.
				Q_ASSERT(cbuf.size() == clen);
				rstate = RecvNormal;
				Packet p(Control, cbuf);
				cbuf.clear();
				return p;
			}
			break; }
		}
	}
}

bool ShellStream::atEnd() const
{
	return strm->atEnd();
}

