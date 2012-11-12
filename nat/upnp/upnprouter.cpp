/***************************************************************************
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include <stdlib.h>
#include <QDir>
#include <QtDebug>
#include <QStringList>
#include <QDomElement>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "upnprouter.h"
#include "upnpdescriptionparser.h"
#include "soap.h"
#include "httprequest.h"

// using namespace net;

namespace bt
{
    struct Forwarding 
    {
        uint16_t port;
        HTTPRequest* pending_req;
        const UPnPService* service;
    };
    
    class UPnPRouter::UPnPRouterPrivate
    {
    public:
        UPnPRouterPrivate(const QString & server,const QUrl & location,bool verbose,UPnPRouter* parent); 
        ~UPnPRouterPrivate();
        
        HTTPRequest* sendSoapQuery(const QString& query, const QString& soapact, const QString& controlurl, bool at_exit = false);
        void forward(const UPnPService* srv,const uint16_t & port);
        void undoForward(const UPnPService* srv,const uint16_t & port);
        void httpRequestDone(HTTPRequest* r,bool erase_fwd);
        void getExternalIP();
        
    public:
        QString server;
        QUrl location;
        UPnPDeviceDescription desc;
        QList<UPnPService> services;
        QList<Forwarding> fwds;
        QList<HTTPRequest*> active_reqs;
        QString error;
        bool verbose;
        UPnPRouter* parent;
        QString external_ip;
    };
    
    ////////////////////////////////////
    
    UPnPService::UPnPService()
    {
    }
    
    UPnPService::UPnPService(const UPnPService & s)
    {
        this->servicetype = s.servicetype;
        this->controlurl = s.controlurl;
        this->eventsuburl = s.eventsuburl;
        this->serviceid = s.serviceid;
        this->scpdurl = s.scpdurl;
    }

    void UPnPService::setProperty(const QString & name,const QString & value)
    {
        if (name == "serviceType")
            servicetype = value;
        else if (name == "controlURL")
            controlurl = value;
        else if (name == "eventSubURL")
            eventsuburl = value;
        else if (name == "SCPDURL")
            scpdurl = value;
        else if (name == "serviceId")
            serviceid = value;
    }
    
    void UPnPService::clear()
    {
        servicetype = controlurl = eventsuburl = scpdurl = serviceid = "";
    }

    UPnPService & UPnPService::operator = (const UPnPService & s)
    {
        this->servicetype = s.servicetype;
        this->controlurl = s.controlurl;
        this->eventsuburl = s.eventsuburl;
        this->serviceid = s.serviceid;
        this->scpdurl = s.scpdurl;
        return *this;
    }
    
    ///////////////////////////////////////
    
    void UPnPDeviceDescription::setProperty(const QString & name,const QString & value)
    {
        if (name == "friendlyName")
            friendlyName = value;
        else if (name == "manufacturer")
            manufacturer = value;
        else if (name == "modelDescription")
            modelDescription = value;
        else if (name == "modelName")
            modelName = value;
        else if (name == "modelNumber")
            modelNumber = value;
    }
    
    ///////////////////////////////////////
    
    
    
    UPnPRouter::UPnPRouter(const QString & server,const QUrl & location,bool verbose) 
        : d(new UPnPRouterPrivate(server,location,verbose,this))
    {
        qDebug() << __PRETTY_FUNCTION__ << server << location;
    }
    
    
    UPnPRouter::~UPnPRouter()
    {
        qDebug() << __PRETTY_FUNCTION__;
        delete d;
    }
    
    void UPnPRouter::addService(const UPnPService & s)
    {
        qDebug() << __PRETTY_FUNCTION__;
        foreach (const UPnPService & os,d->services)
        {
            if (s.servicetype == os.servicetype)
                return;
        }
        d->services.append(s);
    }
    
    void UPnPRouter::downloadFinished(QNetworkReply* j)
    {
        qDebug() << __PRETTY_FUNCTION__;
        if (j->error())
        {
            d->error = QString("Failed to download %1: %2").arg(d->location.toString()).arg(j->error());
            qCritical() << d->error << endl;
            return;
        }

        UPnPDescriptionParser desc_parse;
        bool ret = desc_parse.parse(j->readAll(),this);
        if (!ret)
        {
            d->error = ("Error parsing router description.");
        }
    
        xmlFileDownloaded(this,ret);
        d->getExternalIP();
        j->deleteLater();
    }
    
    void UPnPRouter::downloadXMLFile()
    {
        qDebug() << __PRETTY_FUNCTION__;
        d->error = QString();
        // downlaod XML description into a temporary file in /tmp
        qDebug() << "Downloading XML file" << d->location;
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(downloadFinished(QNetworkReply*)));
        manager->get(QNetworkRequest(d->location));
    }

    void UPnPRouter::forward(const uint16_t & port)
    {
        qDebug() << __PRETTY_FUNCTION__ << port;
        if (!d->error.isEmpty())
        {
            d->error = QString();
            stateChanged();
        }
        
        bool found = false;
        qDebug() << "Forwarding port " << port << " (UDP)";// << (port.proto == UDP ? "UDP" : "TCP") << ")" << endl;
        // first find the right service
        foreach (const UPnPService & s,d->services)
        {
            if (s.servicetype.contains("WANIPConnection") || s.servicetype.contains("WANPPPConnection"))
            {
                d->forward(&s,port);
                found = true;
            }
        }
        
        if (!found)
        {
            d->error = ("Forwarding failed:\nDevice does not have a WANIPConnection or WANPPPConnection.");
            qCritical() << d->error << endl;
            stateChanged();
        }
    }
    

    
    
    void UPnPRouter::undoForward(const uint16_t & port)
    {
        qDebug() << "Undoing forward of port " << port 
                << " (UDP)";// << (port.proto == UDP ? "UDP" : "TCP") << ")" << endl;
        
        QList<Forwarding>::iterator itr = d->fwds.begin();
        while (itr != d->fwds.end())
        {
            Forwarding & wd = *itr;
            if (wd.port == port)
            {
                d->undoForward(wd.service,wd.port);
                itr = d->fwds.erase(itr);
            }
            else
            {
                itr++;
            }
        }
        
        stateChanged();
    }
    
    void UPnPRouter::forwardResult(HTTPRequest* r)
    {
        qDebug() << __PRETTY_FUNCTION__ << r->succeeded();
        if (r->succeeded())
        {
            d->httpRequestDone(r,false);
            stateChanged();
        }
        else
        {
            d->httpRequestDone(r,true);
            if (d->fwds.count() == 0)
            {
                d->error = r->errorString();
                stateChanged();
            }
        }
    }
    
    void UPnPRouter::undoForwardResult(HTTPRequest* r)
    {
        qDebug() << __PRETTY_FUNCTION__;
        d->active_reqs.removeAll(r);
        r->deleteLater();
    }
    
    void UPnPRouter::getExternalIPResult(HTTPRequest* r)
    {
        qDebug() << __PRETTY_FUNCTION__;
        d->active_reqs.removeAll(r);
        if (r->succeeded())
        {
            QDomDocument doc;
            if (!doc.setContent(r->replyData()))
            {
                qDebug() << "UPnP: GetExternalIP failed: invalid reply";
            }
            else
            {
                QDomNodeList nodes = doc.elementsByTagName("NewExternalIPAddress");
                if (nodes.count() > 0)
                {
                    d->external_ip = nodes.item(0).firstChild().nodeValue();
                    qDebug() << "UPnP: External IP: " << d->external_ip;
                }
                else
                    qDebug() << "UPnP: GetExternalIP failed: no IP address returned";
            }
        }
        else
        {
            qDebug() << "UPnP: GetExternalIP failed: " << r->errorString();
        }
        
        r->deleteLater();
    }

    QString UPnPRouter::getExternalIP() const
    {
        return d->external_ip;
    }

    
    void UPnPRouter::isPortForwarded(const uint16_t & port)
    {
        qDebug() << __PRETTY_FUNCTION__;

        // first find the right service
        const UPnPService* service = 0;
        foreach (const UPnPService& s, d->services)
        {
            if (s.servicetype.contains("WANIPConnection") || s.servicetype.contains("WANPPPConnection"))
            {
                service = &s;
                break;
            }
        }

        if (!service)
        {
            d->error = tr("Cannot find port forwarding service in the device's description.");
            qWarning() << d->error;
            stateChanged();
        }
        
        // add all the arguments for the command
        QList<SOAP::Arg> args;
        SOAP::Arg a;
        a.element = "NewRemoteHost";
        args.append(a);
        
        // the external port
        a.element = "NewExternalPort";
        a.value = QString::number(port);
        args.append(a);
        
        // the protocol
        a.element = "NewProtocol";
        a.value = /*port.proto == TCP ? "TCP" :*/ "UDP";
        args.append(a);

        QString action = "GetSpecificPortMappingEntry";
        QString comm = SOAP::createCommand(action,service->servicetype,args);
        HTTPRequest* r = d->sendSoapQuery(comm,service->servicetype + "#" + action,service->controlurl);
    }

    
    void UPnPRouter::setVerbose(bool v) 
    {
        d->verbose = v;
    }

    
    QString UPnPRouter::getServer() const 
    {
        return d->server;
    }

    QUrl UPnPRouter::getLocation() const 
    {
        return d->location;
    }
    
    UPnPDeviceDescription & UPnPRouter::getDescription()
    {
        return d->desc;
    }

    const UPnPDeviceDescription & UPnPRouter::getDescription() const 
    {
        return d->desc;
    }
    
    QString UPnPRouter::getError() const
    {
        return d->error;
    }

    void UPnPRouter::visit(UPnPRouter::Visitor* visitor) const
    {
        foreach (const Forwarding & fwd,d->fwds)
        {
            visitor->forwarding(fwd.port,fwd.pending_req != 0,fwd.service);
        }
    }


    ////////////////////////////////////

    UPnPRouter::UPnPRouterPrivate::UPnPRouterPrivate(const QString& server, const QUrl& location,bool verbose,UPnPRouter* parent)
        : server(server),location(location),verbose(verbose),parent(parent)
    {
        qDebug() << __PRETTY_FUNCTION__;
    }
    
    UPnPRouter::UPnPRouterPrivate::~UPnPRouterPrivate()
    {
        qDebug() << __PRETTY_FUNCTION__;
        foreach (HTTPRequest* r,active_reqs)
        {
            r->cancel();
            r->deleteLater();
        }
    }

    HTTPRequest* UPnPRouter::UPnPRouterPrivate::sendSoapQuery(const QString & query,const QString & soapact,const QString & controlurl,bool at_exit)
    {
        qDebug() << __PRETTY_FUNCTION__;
        // if port is not set, 0 will be returned 
        // thanks to Diego R. Brogna for spotting this bug
        if (location.port()<=0)
            location.setPort(80);
        

        QUrl ctrlurl(controlurl);
        QString host = !ctrlurl.host().isEmpty() ? ctrlurl.host() : location.host();
        uint16_t port = ctrlurl.port() != -1 ? ctrlurl.port() : location.port(80);

        QUrl fullurl(ctrlurl);
        fullurl.setHost(host);
        fullurl.setPort(port);
        fullurl.setScheme("http");

        // QString http_hdr;
        // QTextStream out(&http_hdr);
        // QByteArray encoded_query = ctrlurl.encodedQuery();
        // if (encoded_query.isEmpty())
        //     out << "POST " << ctrlurl.encodedPath() << " HTTP/1.1\r\n";
        // else
        //     out << "POST " << ctrlurl.encodedPath() << "?" << encoded_query << " HTTP/1.1\r\n";

        QByteArray s(soapact.toAscii());
        s.prepend("\"");
        s.append("\"");

        QNetworkRequest req(fullurl);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
        req.setRawHeader("Host", QString("%1:%2").arg(host).arg(port).toAscii());
        req.setRawHeader("User-Agent", "MettaNode/Test");
        req.setRawHeader("SOAPAction", s);
        
        HTTPRequest* r = new HTTPRequest(req, query, verbose);
        if (!at_exit)
        {
            // Only listen for results when we are not exiting
            active_reqs.append(r);
        }
        
        r->start();
        return r;
    }
    
    void UPnPRouter::UPnPRouterPrivate::forward(const UPnPService* srv,const uint16_t & port)
    {
        qDebug() << __PRETTY_FUNCTION__;
        // add all the arguments for the command
        QList<SOAP::Arg> args;
        SOAP::Arg a;
        a.element = "NewRemoteHost";
        args.append(a);
        
        // the external port
        a.element = "NewExternalPort";
        a.value = QString::number(0); // Automatic port selection ///port);
        args.append(a);
        
        // the protocol
        a.element = "NewProtocol";
        a.value = /*port.proto == net::TCP ? "TCP" :*/ "UDP";
        args.append(a);
        
        // the local port
        a.element = "NewInternalPort";
        a.value = QString::number(port);
        args.append(a);
        
        // the local IP address
        a.element = "NewInternalClient";
        a.value = "$LOCAL_IP";// will be replaced by our local ip in HTTPRequest
        args.append(a);
        
        a.element = "NewEnabled";
        a.value = "1";
        args.append(a);
        
        a.element = "NewPortMappingDescription";
        static uint32_t cnt = 0;
        a.value = QString("MettaNode UPnP %1").arg(cnt++);  // TODO: change this
        args.append(a);
        
        a.element = "NewLeaseDuration";
        a.value = "0";
        args.append(a);
        
        QString action = "AddPortMapping";
        QString comm = SOAP::createCommand(action,srv->servicetype,args);
        
        Forwarding fw = {port,0,srv};
        // erase old forwarding if one exists
        QList<Forwarding>::iterator itr = fwds.begin();
        while (itr != fwds.end())
        {
            Forwarding & fwo = *itr;
            if (fwo.port == port && fwo.service == srv)
                itr = fwds.erase(itr);
            else
                itr++;
        }

        fw.pending_req = sendSoapQuery(comm,srv->servicetype + "#" + action,srv->controlurl);
        connect(fw.pending_req,SIGNAL(result(HTTPRequest*)),parent,SLOT(forwardResult(HTTPRequest*)));
        fwds.append(fw);
    }

    void UPnPRouter::UPnPRouterPrivate::undoForward(const UPnPService* srv,const uint16_t & port)
    {
        qDebug() << __PRETTY_FUNCTION__;
        // add all the arguments for the command
        QList<SOAP::Arg> args;
        SOAP::Arg a;
        a.element = "NewRemoteHost";
        args.append(a);
        
        // the external port
        a.element = "NewExternalPort";
        a.value = QString::number(port);
        args.append(a);
        
        // the protocol
        a.element = "NewProtocol";
        a.value = /*port.proto == net::TCP ? "TCP" :*/ "UDP";
        args.append(a);
        
        
        QString action = "DeletePortMapping";
        QString comm = SOAP::createCommand(action,srv->servicetype,args);
        HTTPRequest* r = sendSoapQuery(comm,srv->servicetype + "#" + action,srv->controlurl,1);
        
        // if (waitjob)
        //  waitjob->addExitOperation(r);
        // else
            connect(r,SIGNAL(result(HTTPRequest*)),parent,SLOT(undoForwardResult(HTTPRequest*)));
    }
    
    void UPnPRouter::UPnPRouterPrivate::getExternalIP()
    {
        qDebug() << __PRETTY_FUNCTION__;
        foreach (const UPnPService & s,services)
        {
            if (s.servicetype.contains("WANIPConnection") || s.servicetype.contains("WANPPPConnection"))
            {
                QString action = "GetExternalIPAddress";
                QString comm = SOAP::createCommand(action,s.servicetype);
                HTTPRequest* r = sendSoapQuery(comm,s.servicetype + "#" + action,s.controlurl);
                connect(r,SIGNAL(result(HTTPRequest*)),parent,SLOT(getExternalIPResult(HTTPRequest*)));
                break;
            }
        }
    }
    
    void UPnPRouter::UPnPRouterPrivate::httpRequestDone(HTTPRequest* r,bool erase_fwd)
    {
        qDebug() << __PRETTY_FUNCTION__;
        QList<Forwarding>::iterator i = fwds.begin();
        while (i != fwds.end())
        {
            Forwarding & fw = *i;
            if (fw.pending_req == r)
            {
                fw.pending_req = 0;
                if (erase_fwd)
                    fwds.erase(i);
                break;
            }
            i++;
        }
        
        active_reqs.removeAll(r);
        r->deleteLater();
    }
}






