#pragma once

#include <QString>

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
};

/**
 *  Struct to hold the description of a device
 */
struct UPnPDeviceDescription
{
    QString friendlyName;
    QString manufacturer;
    QString modelDescription;
    QString modelName;
    QString modelNumber;
};

class UpnpRouter : public QObject
{
    Q_OBJECT

public:
    /**
     * Construct a router.
     * @param server The name of the router
     * @param location The location of it's xml description file
     */
    UPnPRouter(const QString & server,const QUrl & location);
    virtual ~UPnPRouter();

    /// Get the router's external IP
    QString getExternalIP() const;

    /**
     * Forward a local port
     * @param port The local port to forward
     */
    void forward(const net::Port & port);
    
    /**
     * Undo forwarding
     * @param port The port
     */
    void undoForward(const net::Port & port);

private slots:
    void forwardResult(HTTPRequest* r);
    void undoForwardResult(HTTPRequest* r);
    void getExternalIPResult(HTTPRequest* r);

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
};
