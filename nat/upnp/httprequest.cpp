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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include <QTimer>
#include <QHostAddress>
#include <QHttpResponseHeader>
#include <QStringList>
#include "httprequest.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QHostInfo>

namespace bt
{

HTTPRequest::HTTPRequest(const QNetworkRequest& rq, const QString& payld, bool verbose)
    : req(rq)
    , payload(payld)
    , verbose(verbose)
    , finished(false)
    , success(false)
    , running(0)
{
    manager = new QNetworkAccessManager(this);

    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());

    payload = payload.replace("$LOCAL_IP", info.addresses()[0].toString());
    req.setHeader(QNetworkRequest::ContentLengthHeader, payload.length());

    qDebug() << req.url();
    foreach (QByteArray h, req.rawHeaderList())
        qDebug() << h << req.rawHeader(h);
    qDebug() << payload;
}

HTTPRequest::~HTTPRequest()
{
    delete running;
    delete manager;
}

void HTTPRequest::start()
{
    success = false;
    finished = false;
    reply.clear();

    connect(manager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(requestFinished(QNetworkReply*)));
    running = manager->post(req, payload.toUtf8());

    QTimer::singleShot(30000,this,SLOT(onTimeout()));
}

void HTTPRequest::cancel()
{
    finished = true;
    delete running; running = 0;
}

void HTTPRequest::requestFinished(QNetworkReply* netreply)
{
    if (finished)
        return;

    if (netreply->error())
    {
        success = false;
        finished = true;
        error = netreply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        reply = netreply->readAll();
        qDebug() << "Received reply:" << reply << ", error:" << error << ", status code" << netreply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        return result(this);
    }

    reply = netreply->readAll();
    qDebug() << "Received reply:" << reply;
    success = true;
    finished = true;
    result(this);
}

void HTTPRequest::onTimeout()
{
    if (finished)
        return;

    qDebug() << "HTTPRequest timeout" << endl;
    success = false;
    finished = true;
    // sock->close();
    error = tr("Operation timed out");
    result(this);
}

}
