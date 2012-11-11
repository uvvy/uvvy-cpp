#include "multicastsocket.h"

UpnpMulticastSocket::UpnpMulticastSocket()
{
    QObject::connect(this, SIGNAL(readyRead()),
        this, SLOT(onReadyRead()));
    QObject::connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(error(QAbstractSocket::SocketError)));

    for (int i = 0;i < 10;i++)
    {
        if (!bind(1900 + i, QUdpSocket::ShareAddress))
            qWarning() << "Cannot bind to UDP port 1900:" << errorString();
        else
            break;
    }

    joinUPnPMCastGroup(socketDescriptor());
}

UpnpMulticastSocket::~UpnpMulticastSocket()
{
    leaveUPnPMCastGroup(socketDescriptor());
}

void UpnpMulticastSocket::discover()
{
    // send a HTTP M-SEARCH message to 239.255.255.250:1900
    const char* data = "M-SEARCH * HTTP/1.1\r\n" 
            "HOST: 239.255.255.250:1900\r\n"
            "ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
            "MAN:\"ssdp:discover\"\r\n"
            "MX:3\r\n"
            "\r\n\0";
    
    writeDatagram(data,strlen(data),QHostAddress("239.255.255.250"),1900);
}

void UpnpMulticastSocket::joinUPnPMCastGroup(int fd)
{
    struct ip_mreq mreq;
    memset(&mreq,0,sizeof(struct ip_mreq));
    
    inet_aton("239.255.255.250",&mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
#ifndef Q_WS_WIN
    if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
#else
    if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
#endif
    {
        qDebug() << "Failed to join multicast group 239.255.255.250" << endl;
    } 
}

void UpnpMulticastSocket::leaveUPnPMCastGroup(int fd)
{
    struct ip_mreq mreq;
    memset(&mreq,0,sizeof(struct ip_mreq));
    
    inet_aton("239.255.255.250",&mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
#ifndef Q_WS_WIN
    if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
#else
    if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
#endif
    {
        qDebug() << "Failed to leave multicast group 239.255.255.250" << endl;
    } 
}

UpnpRouter* UpnpMulticastSocket::parseResponse(const QByteArray & arr)
{
    QStringList lines = QString::fromAscii(arr).split("\r\n");
    QString server;
    KUrl location;

    qDebug() << "Received:" << arr; 
    
    // first read first line and see if contains a HTTP 200 OK message
    QString line = lines.first();
    if (!line.contains("HTTP"))
    {
        // it is either a 200 OK or a NOTIFY
        if (!line.contains("NOTIFY") && !line.contains("200")) 
            return 0;
    }
    else if (line.contains("M-SEARCH")) // ignore M-SEARCH 
        return 0;
    
    // quick check that the response being parsed is valid 
    bool validDevice = false; 
    for (int idx = 0;idx < lines.count() && !validDevice; idx++) 
    { 
        line = lines[idx]; 
        if ((line.contains("ST:") || line.contains("NT:")) && line.contains("InternetGatewayDevice")) 
        {
            validDevice = true; 
        }
    } 
    if (!validDevice)
    {
        //  Out(SYS_PNP|LOG_IMPORTANT) << "Not a valid Internet Gateway Device" << endl;
        return 0; 
    }
    
    // read all lines and try to find the server and location fields
    for (int i = 1;i < lines.count();i++)
    {
        line = lines[i];
        if (line.toLower().startsWith("location"))
        {
            location = line.mid(line.indexOf(':') + 1).trimmed();
            if (!location.isValid())
                return 0;
        }
        else if (line.toLower().startsWith("server"))
        {
            server = line.mid(line.indexOf(':') + 1).trimmed();
            if (server.length() == 0)
                return 0;
        }
    }
    
    if (findDevice(location))
    {
        return 0;
    }
    else
    {
        qDebug() << "Detected IGD" << server;
        return new UpnpRouter(server, location); 
    }
}

UpnpRouter* UpnpMulticastSocket::findDevice(const QUrl& location)
{
    foreach (UpnpRouter* r, routers)
    {
        if (UrlCompare(r->getLocation(),location))
            return r;
    }
    
    return 0;
}
