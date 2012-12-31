/***************************************************************************
 * Based on KTorrent UPnP code by:                                         *
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   joris.guisson@gmail.com                                               *
 *   Licensed under GPL2+                                                  *
 ***************************************************************************/

#pragma once

#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * @author Joris Guisson / Berkus
 * Simple HTTP request class using QNetworkAccessManager. 
 */
class HTTPRequest : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor, set the url and the request header.
     * @param req The http request header
     * @param payload The payload
     * @param verbose Print traffic to the log
     */
    HTTPRequest(const QNetworkRequest& req, const QString& payload, bool verbose);
    virtual ~HTTPRequest();

    /**
     * Open a connetion and send the request.
     */
    void start();

    /**
        Cancel the request, no result signal will be emitted.
    */
    void cancel();

    /**
        Get the reply data
    */
    QByteArray replyData() const {return reply;}

    /**
        Did the request succeed
    */
    bool succeeded() const {return success;}

    /**
        In case of failure this function will return an error string
    */
    QString errorString() const {return error;}

signals:
    /**
     * An OK reply was sent.
     * @param r The sender of the request
     */
    void result(HTTPRequest* r);

private slots:
    void requestFinished(QNetworkReply* netreply);
    void onTimeout();

private:
    QNetworkAccessManager* manager;
    QNetworkReply* running;
    QNetworkRequest req;
    QString payload;
    bool verbose;
    bool finished;
    bool success;
    QByteArray reply;
    QString error;
};
