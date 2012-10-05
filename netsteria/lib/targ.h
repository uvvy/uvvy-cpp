#ifndef NET_TARG_H
#define NET_TARG_H

#include <QObject>

class QByteArray;
class QDataStream;
class UdpEndpoint;

// 32-bit dispatch target label
typedef quint32 TargetLabel;

// Abstract base class representing local packet dispatch target
class Target : public QObject
{
public:
	inline Target(QObject *parent) : QObject(parent) { }

	virtual void udpReceive(QByteArray &msg, QDataStream &ds,
				UdpEndpoint &src) = 0;
};

#endif	// NET_TARG_H
