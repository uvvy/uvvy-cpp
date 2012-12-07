/***************************************************************************
 * Based on KTorrent UPnP code by:                                         *
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   joris.guisson@gmail.com                                               *
 *   Licensed under GPL2+                                                  *
 ***************************************************************************/

#pragma once

#include <QString>
#include <QHostAddress>
#include <QDebug>
#ifdef Q_WS_X11
#include <stdint.h>
#endif
 
class QNetworkReply;
class HTTPRequest;

struct Port
{
    uint16_t number;
    enum Proto { TCP, UDP } protocol;

    Port(uint16_t n, Proto p) : number(n), protocol(p) {}
};

inline bool operator ==(const Port& left, const Port& right)
{
    return left.number == right.number and left.protocol == right.protocol;
}

inline QDebug operator << (QDebug dbg, const Port& p)
{
    dbg.nospace() << p.number << " (" << (p.protocol == Port::TCP ? "TCP" : "UDP") << ")";
    return dbg.space();
}

/**
 * Structure describing a UPnP service found in an xml file.
*/
struct UPnPService  
{ 
    QString serviceid;
    QString servicetype;
    QString controlurl;
    QString eventsuburl;
    QString scpdurl;
    
    UPnPService();
    UPnPService(const UPnPService & s);
    
    /**
     * Set a property of the service.
     * @param name Name of the property (matches to variable names)
     * @param value Value of the property
     */
    void setProperty(const QString & name,const QString & value);
    
    /**
     * Set all strings to empty.
     */
    void clear();
    
    /**
     * Assignment operator
     * @param s The service to copy
     * @return *this
     */
    UPnPService & operator = (const UPnPService & s);
};

/**
 *  Struct to hold the description of a device.
 */
struct UPnPDeviceDescription
{
    QString friendlyName;
    QString manufacturer;
    QString modelDescription;
    QString modelName;
    QString modelNumber;
    
    /**
     * Set a property of the description
     * @param name Name of the property (matches to variable names)
     * @param value Value of the property
     */
    void setProperty(const QString & name,const QString & value);
};

/**
 * @author Joris Guisson
 * 
 * Class representing a UPnP enabled router. This class is also used to communicate
 * with the router.
*/
class UPnPRouter : public QObject
{
    Q_OBJECT
public:
    /**
     * Construct a router.
     * @param server The name of the router
     * @param location The location of it's xml description file
     * @param verbose Print lots of debug info
     */
    UPnPRouter(const QString & server,const QUrl & location,bool verbose = false);  
    virtual ~UPnPRouter();

    /// Disable or enable verbose logging
    void setVerbose(bool v);
    
    /// Get the name  of the server
    QString getServer() const;
    
    /// Get the location of it's xml description
    QUrl getLocation() const;
    
    /// Get the device description
    UPnPDeviceDescription & getDescription();
    
    /// Get the device description (const version)
    const UPnPDeviceDescription & getDescription() const;
    
    /// Get the current error (null string if there is none)
    QString getError() const;
    
    /// Get the router's external IP
    QString getExternalIP() const;
    
    /**
     * Download the XML File of the router.
     */
    void downloadXMLFile();
    
    /**
     * Add a service to the router.
     * @param s The service
     */
    void addService(const UPnPService & s);

    /**
     * See if a port is forwarded
     * @param port The Port
     */
    void isPortForwarded(const Port & port);    
    
    /**
     * Forward a local port
     * @param port The local port to forward
     * @param extPort The external port number to use, automatically assign if zero.
     */
    void forward(const Port & port, int leaseDuration = 0, uint16_t extPort = 0);

    /**
     * Undo forwarding
     * @param port The port
     */
    void undoForward(const Port & port);

    /**
     * Description of an UPnP forwarding.
     */
    struct ForwardingEntry {
        QHostAddress remoteHost;
        QHostAddress internalClient;
        Port port;
        uint16_t extPort;
        QString description;
        int leaseDuration;
    };

    void getPortForwardings(QList<ForwardingEntry>& entries);

    /**
        In order to iterate over all forwardings, the visitor pattern must be used.
    */
    class Visitor
    {
    public:
        virtual ~Visitor() {}
        
        /**
            Called for each forwarding.
            @param port The Port
            @param pending When set to true, the forwarding is not completed yet
            @param service The UPnPService this forwarding is for
        */
        virtual void forwarding(const Port & port, bool pending, const UPnPService* service) = 0;
    };
    
    /**
        Visit all forwardings
        @param visitor The Visitor
    */
    void visit(Visitor* visitor) const;

    
private slots:
    void forwardResult(HTTPRequest* r);
    void undoForwardResult(HTTPRequest* r);
    void getExternalIPResult(HTTPRequest* r);
    void getNumPortForwardingsResult(HTTPRequest* r);
    void downloadFinished(QNetworkReply*);
    
signals:
    /**
     * Internal state has changed, a forwarding succeeded or failed, or an undo forwarding succeeded or failed.
     */
    void stateChanged();
    
    /**
     * Signal which indicates that the XML was downloaded successfully or not.
     * @param r The router which emitted the signal
     * @param success Whether or not it succeeded
     */
    void xmlFileDownloaded(UPnPRouter* r,bool success);

private:
    void getNumPortForwardings(int);
   
private:
    class UPnPRouterPrivate;
    UPnPRouterPrivate* d;
};
